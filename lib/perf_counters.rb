require 'perf_counters/version'
require 'perf_counters/perf_counters'

module Event
  module Type
    HARDWARE = 0
    SOFTWARE = 1
  end

  module CPU_CYCLES
    TYPE = Type::HARDWARE
    VALUE = 0
  end
  module INSTRUCTIONS
    TYPE = Type::HARDWARE
    VALUE = 1
  end
  module PERF_COUNT_HW_CACHE_MISSES
    TYPE = Type::HARDWARE
    VALUE = 3
  end

  module BRANCH_INSTRUCTIONS
    TYPE = Type::HARDWARE
    VALUE = 4
  end

  module PAGE_FAULTS_MIN
    TYPE = Type::SOFTWARE
    VALUE = 5
  end

  module SW_PAGE_FAULTS_MAJ
    TYPE = Type::SOFTWARE
    VALUE = 6
  end
end

module PerfCounters
  class Measurement
    attr_accessor :events, :exclude_kernel

    def initialize(events: [], exclude_kernel: true)
      @events = events
      @exclude_kernel = exclude_kernel
    end
  end

  def self.measure(*args)
    perf = PerfCounters::Measurement.new(*args)
    perf.start
    yield
    perf.stop
  end
end
