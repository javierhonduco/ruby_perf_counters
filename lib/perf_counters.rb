require 'perf_counters/version'
require 'perf_counters/perf_counters'

Counter = Struct.new(:name, :type, :value)

module Event
  module Type
    HARDWARE = 0
    SOFTWARE = 1
  end

  CPU_CYCLES          = Counter.new(:cpu_cycles, Type::HARDWARE, 0)
  INSTRUCTIONS        = Counter.new(:instructions, Type::HARDWARE, 1)
  CACHE_MISSES        = Counter.new(:cache_misses, Type::HARDWARE, 3)
  BRANCH_INSTRUCTIONS = Counter.new(:branch_instructions, Type::HARDWARE, 4)
  PAGE_FAULTS_MIN     = Counter.new(:page_faults_min, Type::SOFTWARE, 5)
  PAGE_FAULTS_MAJ     = Counter.new(:page_faults_maj, Type::SOFTWARE, 6)
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
