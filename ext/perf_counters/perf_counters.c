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
      rb_raise(rb_eArgError, "ioctl call failed");                             \
    }                                                                          \
  } while (0);

struct read_format {
  uint64_t nr;
  struct {
    uint64_t value;
    uint64_t id;
  } values[];
};

int started = 0;
struct perf_event_attr pe;
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

  for (int i = 0; i < rb_events_len; i++) {
    VALUE rb_current_array_element = rb_ary_entry(rb_events, i);
    // extract type, value
    VALUE type = rb_const_get_at(rb_current_array_element, rb_intern("TYPE"));
    VALUE config =
        rb_const_get_at(rb_current_array_element, rb_intern("VALUE"));

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
      rb_raise(rb_eArgError, "perf_event_open failed type=%d, config=%d", type,
               config);
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

  for (int i = 0; i < rb_events_len; i++) {
    close(fds[i]);
  }
  xfree(fds);
  xfree(ids);

  if (read_bytes == -1) {
    rb_raise(rb_eArgError, "reading the performance counters failed");
    return Qnil;
  }

  uint64_t results[rb_events_len];
  VALUE rb_hash_results = rb_hash_new();

  // Assuming here that the events are in the same order they are requested
  for (int i = 0; i < rb_events_len; i++) {
    VALUE rb_current_array_element = rb_ary_entry(rb_events, i);

    // Event::INSTRUCTIONS.name.split('::').last.downcase
    // TODO: would it make sense to have this in a ruby function?
    VALUE rb_name = rb_funcall(rb_current_array_element, rb_intern("name"), 0);
    VALUE rb_split =
        rb_funcall(rb_name, rb_intern("split"), 1, rb_str_new_cstr("::"));
    VALUE rb_last = rb_funcall(rb_split, rb_intern("last"), 0);
    VALUE rb_downcase = rb_funcall(rb_last, rb_intern("downcase"), 0);
    VALUE rb_symbol = rb_funcall(rb_downcase, rb_intern("to_sym"), 0);

    rb_hash_aset(rb_hash_results, rb_symbol, INT2NUM(rf->values[i].value));
  }

  started = 1;
  return rb_hash_results;
}

void Init_perf_counters(void) {
  VALUE rb_mPerfCounters = rb_define_module("PerfCounters");

  VALUE rb_Measurement =
      rb_define_class_under(rb_mPerfCounters, "Measurement", rb_cObject);
  rb_define_method(rb_Measurement, "start", measurement_start, 0);
  rb_define_method(rb_Measurement, "stop", measurement_stop, 0);
}
