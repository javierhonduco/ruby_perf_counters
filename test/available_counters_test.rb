require 'test_helper'
require 'perf_counters/available_perf_counters'

class AvailablePerfCountersTest < Minitest::Test
  def test_type_of_all
    assert_kind_of Array, PerfCounters::AvailableCounters::Hardware.all
  end

  def test_all_includes_a_valid_perf_counter
    all_perf_counters = PerfCounters::AvailableCounters::Hardware.all
    assert_operator all_perf_counters, :include?, 'instructions'
  end
  def test_all_includes_a_valid_perf_counter
    event_data = PerfCounters::AvailableCounters::Hardware.event(
      'instructions'
    )
    assert_kind_of String, event_data
  end
end
