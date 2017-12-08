
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'perf_counters/version'

Gem::Specification.new do |spec|
  spec.name          = 'perf_counters'
  spec.version       = PerfCounters::VERSION
  spec.authors       = ['Javier Honduvilla Coto']
  spec.email         = ['javierhonduco@gmail.com']

  spec.summary       = "Read the CPU's performance counters perf_event_open(2)"
  spec.description   = spec.summary
  spec.homepage      = 'https://github.com/javierhonduco/ruby_perf_counters'
  spec.license       = 'MIT'

  spec.files = `git ls-files -z`.split("\x0").reject do |f|
    f.match(%r{^(test|spec|features)/})
  end

  spec.bindir        = 'exe'
  spec.executables   = spec.files.grep(%r{^exe/}) { |f| File.basename(f) }
  spec.require_paths = ['lib']
  spec.extensions    = ['ext/perf_counters/extconf.rb']

  spec.add_development_dependency 'benchmark-ips', '~> 2.7.2'
  spec.add_development_dependency 'bundler', '~> 1.16'
  spec.add_development_dependency 'kalibera'
  spec.add_development_dependency 'minitest', '~> 5.0'
  spec.add_development_dependency 'rake', '~> 10.0'
  spec.add_development_dependency 'rake-compiler'
end
