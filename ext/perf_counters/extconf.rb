require 'mkmf'

$CFLAGS << ' -O0 -ggdb'
create_makefile('perf_counters/perf_counters')
