require 'bundler/gem_tasks'
require 'rake/testtask'

Rake::TestTask.new(:test) do |t|
  t.libs << 'test'
  t.libs << 'lib'
  t.test_files = FileList['test/**/*_test.rb']
end

require 'rake/extensiontask'

task build: :compile

Rake::ExtensionTask.new('perf_counters') do |ext|
  ext.lib_dir = 'lib/perf_counters'
end

task default: %i[clobber compile test]

task benchmark: %i[clobber compile] do
  require 'benchmark/ips'
  require 'perf_counters'

  # TODO:
  # - `x.stats = :bootstrap` seems to get frozen
  # - investigate why results are not very consistent
  Benchmark.ips do |x|
    x.iterations = 3

    x.report('no perf_counters') do |times|
      (0..times).inject(:+)
    end

    x.report('using yield') do |times|
      PerfCounters.measure(events: [Event::INSTRUCTIONS]) do
        (0..times).inject(:+)
      end
    end

    x.report('using yield (frozen array)') do |times|
      PerfCounters.measure(events: [Event::INSTRUCTIONS].freeze) do
        (0..times).inject(:+)
      end
    end

    x.report('not using yield') do |times|
      pc = PerfCounters::Measurement.new(events: [Event::INSTRUCTIONS])
      pc.start
      (0..times).inject(:+)
      pc.stop
    end

    x.report('not using yield (frozen array)') do |times|
      pc = PerfCounters::Measurement.new(events: [Event::INSTRUCTIONS].freeze)
      pc.start
      (0..times).inject(:+)
      pc.stop
    end
    x.compare!
  end
end
