#include "perf_counters.h"
#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define leader(array) (array[0])
#define raise_on_error(function_call)                                          \
  do {                                                                         \
    if (function_call) {                                                       \
      xfree(fds);                                                              \
      xfree(ids);                                                              \
      started = 0;                                                             \
      rb_raise(rb_eArgError, "ioctl call failed in line %d", __LINE__);        \
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
int started = 0;
int *fds;
uint64_t *ids;

// static?
static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags) {
  return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

VALUE
measurement_start(VALUE self) {
  started = 1;
  VALUE rb_events = rb_iv_get(self, "@events");
  size_t rb_events_len = RARRAY_LEN(rb_events);
  // TODO:
  // - figure out what happens if somewhere in the stack a exception is raised
  // and what impact has in a native extension (e.g: not freeing resources?)
  // - can we statically allocate this? Would it make sense
  // performance-wise?
  fds = xmalloc(sizeof(int) * rb_events_len);
  ids = xmalloc(sizeof(uint64_t) * rb_events_len);

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
      current_fd = perf_event_open(&pe, 0, -1, leader(fds), 0);
    }

    if (current_fd == -1) {
      xfree(fds);
      xfree(ids);
      started = 0;

      rb_raise(rb_eArgError, "perf_event_open failed type=%d, config=%d. Check "
                             "your Linux kernel's version source code to see "
                             "if this event exists in "
                             "'include/uapi/linux/perf_event.h'",
               NUM2INT(type), NUM2INT(config));
    }

    fds[i] = current_fd;
    raise_on_error(ioctl(current_fd, PERF_EVENT_IOC_ID, &ids[i]));
  }

  raise_on_error(ioctl(leader(fds), PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP));
  raise_on_error(
      ioctl(leader(fds), PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP));

  return Qtrue;
}

VALUE
measurement_stop(VALUE self) {
  if (!started) {
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

  raise_on_error(
      ioctl(leader(fds), PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP));
  ssize_t read_bytes = read(leader(fds), buffer, sizeof(buffer));

  for (unsigned int i = 0; i < rb_events_len; i++) {
    close(fds[i]);
  }
  xfree(fds);
  xfree(ids);

  if (read_bytes == -1) {
    rb_raise(rb_eArgError, "read of the performance counters failed");
    return Qnil;
  }

  VALUE rb_hash_results = rb_hash_new();

  // Assuming here that the events are in the same order they are requested
  for (unsigned int i = 0; i < rb_events_len; i++) {
    VALUE rb__events = rb_iv_get(self, "@__events");
    VALUE rb_symbol = rb_ary_entry(rb__events, (i * 3) + 0);

    rb_hash_aset(rb_hash_results, rb_symbol, INT2NUM(rf->values[i].value));
  }

  started = 0;
  return rb_hash_results;
}

void Init_perf_counters(void) {
  VALUE rb_mPerfCounters = rb_define_module("PerfCounters");

  VALUE rb_Measurement =
      rb_define_class_under(rb_mPerfCounters, "Measurement", rb_cObject);
  rb_define_method(rb_Measurement, "start", measurement_start, 0);
  rb_define_method(rb_Measurement, "stop", measurement_stop, 0);
}
