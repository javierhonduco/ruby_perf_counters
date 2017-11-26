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
      Event::PAGE_FAULTS_MAJ
    ]

    perf = PerfCounters::Measurement.new(
      exclude_kernel: true,
      events: events
    )
    perf.start
    results = perf.stop

    assert_kind_of Hash, results
    assert_equal 6, results.size

    %i[
      instructions cpu_cycles branch_instructions
      cache_misses page_faults_min page_faults_maj
    ].each do |event|
      refute_nil results[event]
      assert_kind_of Integer, results[event]
    end

    results
  end

  def test_yield_works
    PerfCounters.measure(events: [Event::INSTRUCTIONS]) do
      'allocate_some_memory' * 100
    end
  end

  def test_other_events
    events = [
      Event::CPU_CLOCK,
      Event::PAGE_FAULTS,
      Event::CONTEXT_SWITCHES,
      Event::BPF_OUTPUT
    ]
    PerfCounters.measure(events: events) do
      :lol
    end
  end

  def test_event_that_does_not_exist_raises_exception
    fake_event = Counter.new(:lol, Event::Type::SOFTWARE, 11)
    perf = PerfCounters::Measurement.new(events: [fake_event])

    assert_raises(ArgumentError) do
      perf.start
    end
    assert_nil perf.stop
  end

  def test_multiple_stop_before_start_do_nothing
    perf = PerfCounters::Measurement.new(events: [Event::INSTRUCTIONS])

    assert_nil perf.stop
    assert_nil perf.stop
  end

  def test_does_not_leak_fds
    fds_count = -> () { Dir.glob('/proc/self/fd/*').count }
    events = [
      Event::CPU_CLOCK,
      Event::PAGE_FAULTS,
      Event::CONTEXT_SWITCHES,
      Event::BPF_OUTPUT
    ]

    fds_count_before = fds_count.call
    100.times do
      PerfCounters.measure(events: events) { ; }
    end
    fds_count_after = fds_count.call

    assert_equal fds_count_before, fds_count_after
  end

  def test_bad_event_types
    bad_typed_event = Counter.new(12, {a: :b}, [:a, :b])
    perf = PerfCounters::Measurement.new(events: [bad_typed_event])

    assert_raises(TypeError) do
      perf.start
    end

    assert_raises(ArgumentError) do
      perf.stop
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
