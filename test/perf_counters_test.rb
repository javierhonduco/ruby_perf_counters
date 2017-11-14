require 'test_helper'

class PerfCountersTest < Minitest::Test
  def test_that_it_has_a_version_number
    refute_nil ::PerfCounters::VERSION
  end

  def test_it_does_something_useful
    events = [
      Event::INSTRUCTIONS,
      Event::CPU_CYCLES,
      Event::BRANCH_INSTRUCTIONS,
      Event::CACHE_MISSES,
      Event::PAGE_FAULTS_MIN,
      Event::PAGE_FAULTS_MAJ,
    ]

    perf = PerfCounters::Measurement.new(
      exclude_kernel: true,
      events: events,
    )
    perf.start
    results = perf.stop

    assert_kind_of Hash, results
    assert_equal 6, results.size

    [
      :instructions, :cpu_cycles, :branch_instructions,
      :cache_misses, :page_faults_min, :page_faults_maj,
    ].each do |event|
      refute_nil results[event]
      assert_kind_of Numeric, results[event]
    end

    results
  end

  def test_yield_works
    perf = PerfCounters.measure(events: [Event::INSTRUCTIONS]) do
      'allocate_some_memory' * 100
    end
  end

  if ENV['STRESS_TEST']
    def test_lol
      100_000.times do
        puts test_it_does_something_useful
      end
    end
  end
end
