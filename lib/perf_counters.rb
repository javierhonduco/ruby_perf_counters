require 'perf_counters/version'
require 'perf_counters/perf_counters'

Counter = Struct.new(:name, :type, :value)

module Event
  # from linux's include/uapi/linux/perf_event.h

  # `perf_type_id`
  module Type
    HARDWARE = 0
    SOFTWARE = 1
  end

  # TODO: does this make more sense?
  #
  # class HWCounter < Counter
  #   def initialize(name, value)
  #     super(name, Type::HARDWARE, value)
  #   end
  # end

  # `perf_hw_id`
  CPU_CYCLES              = Counter.new(:cpu_cycles, Type::HARDWARE, 0)
  INSTRUCTIONS            = Counter.new(:instructions, Type::HARDWARE, 1)
  CACHE_REFERENCES        = Counter.new(:cache_references, Type::HARDWARE, 2)
  CACHE_MISSES            = Counter.new(:cache_misses, Type::HARDWARE, 3)
  BRANCH_INSTRUCTIONS     = Counter.new(:branch_instructions, Type::HARDWARE, 4)
  BRANCH_MISSES           = Counter.new(:branch_misses, Type::HARDWARE, 5)
  BUS_CYCLES              = Counter.new(:bus_cycles, Type::HARDWARE, 6)
  STALLED_CYCLES_FRONTEND = Counter.new(:stalled_cycles_frontend, Type::HARDWARE, 7)
  STALLED_CYCLES_BACKEND  = Counter.new(:stalled_cycles_backend, Type::HARDWARE, 8)
  REF_CPU_CYCLES          = Counter.new(:ref_cpu_cycles, Type::HARDWARE, 9)

  # `perf_sw_ids`
  CPU_CLOCK         = Counter.new(:cpu_clock, Type::SOFTWARE, 0)
  TASK_CLOCK        = Counter.new(:task_clock, Type::SOFTWARE, 1)
  PAGE_FAULTS       = Counter.new(:page_faults, Type::SOFTWARE, 2)
  CONTEXT_SWITCHES  = Counter.new(:context_switches, Type::SOFTWARE, 3)
  CPU_MIGRATIONS    = Counter.new(:cpu_migrations, Type::SOFTWARE, 4)
  PAGE_FAULTS_MIN   = Counter.new(:page_faults_min, Type::SOFTWARE, 5)
  PAGE_FAULTS_MAJ   = Counter.new(:page_faults_maj, Type::SOFTWARE, 6)
  ALIGNMENT_FAULTS  = Counter.new(:alignment_faults, Type::SOFTWARE, 7)
  EMULATION_FAULTS  = Counter.new(:emulation_faults, Type::SOFTWARE, 8)
  DUMMY             = Counter.new(:dummy, Type::SOFTWARE, 9)
  BPF_OUTPUT        = Counter.new(:bpf_output, Type::SOFTWARE, 10)
end

module PerfCounters
  class Measurement
    attr_accessor :events, :exclude_kernel, :disabled, :exclude_hv

    def initialize(events: [], exclude_kernel: true, disabled: true, exclude_hv: true)
      @events = events

      @exclude_kernel = exclude_kernel
      @disabled = disabled
      @exclude_hv = exclude_hv

      @__events = @events.flat_map do |event|
        [event.name, event.type, event.value]
      end
    end

    def start
      __start
    end

    def stop
      return nil unless result_array = __stop

      result_array.each_with_object({}).with_index do |(result, final), i|
        final[events[i].name] = result
      end
    end
  end

  def self.measure(*args)
    perf = PerfCounters::Measurement.new(*args)
    perf.start
    yield
    perf.stop
  end
end
