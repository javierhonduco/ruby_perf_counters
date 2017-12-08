# PerfCounters

Experimental, do not use in production, could blow up your Rubies.

### What
Read the CPU's performance counters from Ruby using `perf_event_open(2)`.

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

### Sample Rack middleware in a Rails app
```ruby
class PerformanceCountersRackMiddleware
  def initialize app
    @app = app
  end

  def call env
    events = [
      Event::INSTRUCTIONS,
      Event::CPU_CYCLES,
      Event::CACHE_MISSES,
      Event::CONTEXT_SWITCHES,
      Event::BUS_CYCLES,
      Event::PAGE_FAULTS_MIN,
    ]
    perf_data = PerfCounters.measure(exclude_kernel: true, events: events) do
      @status, @headers, @response = @app.call(env)
    end

    Rails.logger.info "#{perf_data}"

    [@status, @headers, @response]
  end
end
```

For 2 request it outputs something like:
```ruby
{:instructions=>93930165, :cpu_cycles=>69263494, :cache_misses=>101678, :context_switches=>0, :bus_cycles=>2663999, :page_faults_min=>216}
[...]
Completed 200 OK in 71ms (Views: 54.7ms)


{:instructions=>93984918, :cpu_cycles=>74644389, :cache_misses=>116050, :context_switches=>0, :bus_cycles=>2870920, :page_faults_min=>246}
[...]
Completed 200 OK in 50ms (Views: 38.2ms)
```
### Contributing

#### Requirements
A modern Linux machine (unfortunately, most VMs don't virtualize perf counters)
with `perf` installed.

#### Install
```shell
$ bin/setup
```

#### Compile
```shell
$ rake compile
```

#### Run tests
```shell
$ rake [test]
```
