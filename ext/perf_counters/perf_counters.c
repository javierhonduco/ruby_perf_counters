#include "perf_counters.h"
#include <asm/unistd.h>
#include <errno.h>
#include <linux/perf_event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define LEADER(array) (array[0])
#define RAISE_ON_ERROR(function_call)                                          \
  do {                                                                         \
    if (function_call == -1) {                                                 \
      state->started = 0;                                                      \
      xfree(state->fds);                                                       \
      xfree(state->ids);                                                       \
      rb_raise(rb_eArgError, "ioctl call failed in line %d with '%s'",         \
               __LINE__, strerror(errno));                                     \
    }                                                                          \
  } while (0);

struct read_format {
  uint64_t nr;
  struct {
    uint64_t value;
    uint64_t id;
  } values[];
};
struct perf_event_attr pe;
struct perf_state {
  int started;
  int *fds;
  uint64_t *ids;
};

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags) {
  return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

void dealloc_state(struct perf_state *state) { xfree(state); }

VALUE
alloc_state(VALUE klass) {
  struct perf_state *state = xmalloc(sizeof(struct perf_state));
  state->started = 0;
  return Data_Wrap_Struct(klass, NULL, dealloc_state, state);
}

VALUE
measurement_start(VALUE self) {
  VALUE rb_events = rb_iv_get(self, "@events");
  size_t rb_events_len = RARRAY_LEN(rb_events);

  struct perf_state *state;
  Data_Get_Struct(self, struct perf_state, state);
  state->started = 1;
  state->fds = xmalloc(sizeof(int) * rb_events_len);
  state->ids = xmalloc(sizeof(uint64_t) * rb_events_len);

  for (unsigned int i = 0; i < rb_events_len; i++) {
    VALUE rb__events = rb_iv_get(self, "@__events");
    // extract type, value
    VALUE type = rb_ary_entry(rb__events, (i * 3) + 1);
    VALUE config = rb_ary_entry(rb__events, (i * 3) + 2);

    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = NUM2INT(type);
    pe.config = NUM2INT(config);
    pe.size = sizeof(struct perf_event_attr);
    pe.disabled = 1;
    pe.exclude_kernel = rb_iv_get(self, "@exclude_kernel") == Qtrue ? 1 : 0;
    pe.exclude_hv = 1;
    pe.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;

    int current_fd;
    if (i == 0) {
      current_fd = perf_event_open(&pe, 0, -1, -1, 0);
    } else {
      current_fd = perf_event_open(&pe, 0, -1, LEADER(state->fds), 0);
    }

    if (current_fd == -1) {
      state->started = 0;
      xfree(state->fds);
      xfree(state->ids);

      rb_raise(rb_eArgError, "perf_event_open failed type=%d, config=%d. Check "
                             "your Linux kernel's version source code to see "
                             "if this event exists in "
                             "'include/uapi/linux/perf_event.h'",
               NUM2INT(type), NUM2INT(config));
    }

    state->fds[i] = current_fd;
    RAISE_ON_ERROR(ioctl(current_fd, PERF_EVENT_IOC_ID, &state->ids[i]));
  }

  RAISE_ON_ERROR(
      ioctl(LEADER(state->fds), PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP));
  RAISE_ON_ERROR(
      ioctl(LEADER(state->fds), PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP));

  return Qtrue;
}

VALUE
measurement_stop(VALUE self) {
  struct perf_state *state;
  Data_Get_Struct(self, struct perf_state, state);

  if (!state->started) {
    return Qnil;
  }

  VALUE rb_events = rb_iv_get(self, "@events");
  size_t rb_events_len = RARRAY_LEN(rb_events);
  // TODO: check the buffer size is ok
  size_t buffer_size =
      (sizeof(uint64_t) + (sizeof(uint64_t) * 2 * rb_events_len));
  char buffer[buffer_size];
  memset(buffer, 0, buffer_size);
  struct read_format *rf = (struct read_format *)buffer;

  RAISE_ON_ERROR(
      ioctl(LEADER(state->fds), PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP));
  ssize_t read_bytes = read(LEADER(state->fds), buffer, sizeof(buffer));

  for (unsigned int i = 0; i < rb_events_len; i++) {
    close(state->fds[i]);
  }
  xfree(state->fds);
  xfree(state->ids);

  if (read_bytes == -1) {
    rb_raise(rb_eArgError, "read of the performance counters failed");
    return Qnil;
  }

  VALUE rb_array_result = rb_ary_new();

  // Assuming here that the events are in the same order they are requested
  for (unsigned int i = 0; i < rb_events_len; i++) {
    rb_ary_push(rb_array_result, INT2NUM(rf->values[i].value));
  }

  state->started = 0;
  return rb_array_result;
}

void Init_perf_counters(void) {
  VALUE rb_mPerfCounters = rb_define_module("PerfCounters");

  VALUE rb_Measurement =
      rb_define_class_under(rb_mPerfCounters, "Measurement", rb_cObject);
  rb_define_alloc_func(rb_Measurement, alloc_state);
  rb_define_method(rb_Measurement, "__start", measurement_start, 0);
  rb_define_method(rb_Measurement, "__stop", measurement_stop, 0);
}
