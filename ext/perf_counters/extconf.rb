require 'mkmf'

$CFLAGS << ' -Wall -Werror -Wextra -Wshadow -Wpedantic'

$CFLAGS << if ENV['OPTIMIZE']
             ' -O3'
           else
             ' -O0 -ggdb'
end

# RbConfig::MAKEFILE_CONFIG['CC'] = 'clang' if ENV['USE_CLANG']
create_makefile('perf_counters/perf_counters')
