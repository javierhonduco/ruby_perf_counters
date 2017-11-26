module PerfCounters
  module AvailableCounters
    module Hardware
      EVENTS_SYS_PATH = '/sys/bus/event_source/devices/cpu/events/'

      class << self
        def all
          Dir
            .entries(EVENTS_SYS_PATH)
            .reject { |el| ['.', '..'].include?(el) }
        end

        def event(event_name)
          File.read("#{EVENTS_SYS_PATH}#{event_name}")
        end
      end
    end
  end
end
