# PerfCounters

Experimental, do not use in production, could blow up your Rubies.

### Usage
```ruby
require 'perf_counters'
events = [
  Event::INSTRUCTIONS,
  Event::CPU_CYCLES,
  Event::CACHE_MISSES,
]
perf = PerfCounters::Measurement.new(
  exclude_kernel: true,
  events: events,
)
perf.start
# do something here
perf.stop
=> {:instructions=>3276, :cpu_cycles=>18651, :cache_misses=>24}
```

### Contributing

#### Requirements
A modern Linux machine (unfortunately, most VMs don't virtualize perf counters)
with `perf` installed.

#### Install
```shell
$ bundle install
```

#### Compile
```shell
$ rake compile
```

#### Run tests
```shell
$ rake [test]
```
