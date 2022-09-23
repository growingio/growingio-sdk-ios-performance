Pod::Spec.new do |s|
  s.name             = 'GrowingAPM'
  s.version          = '0.0.1'
  s.summary          = 'iOS SDK of GrowingIO.'
  s.description      = <<-DESC
GrowingAPM提供移动端性能采集分析功能，包括崩溃分析、启动分析、页面加载分析等。
                       DESC
  s.homepage         = 'https://www.growingio.com/'
  s.license          = { :type => 'Apache2.0', :file => 'LICENSE' }
  s.author           = { 'GrowingIO' => 'support@growingio.com' }
  s.source           = { :git => 'https://github.com/growingio/growingio-sdk-ios-performance.git', :tag => s.version.to_s }
  s.ios.deployment_target = '8.0'
  s.osx.deployment_target =  '10.8'
  s.tvos.deployment_target =  '9.0'
  s.watchos.deployment_target =  '2.0'
  s.frameworks = 'Foundation'
  s.libraries = 'c++', 'z'
  s.requires_arc = true
  s.xcconfig = { 'GCC_ENABLE_CPP_EXCEPTIONS' => 'YES' }
  s.default_subspecs = 'CrashMonitor'

  s.subspec 'CrashMonitor' do |crashmonitor|
    crashmonitor.subspec 'Recording' do |recording|
      recording.compiler_flags = '-fno-optimize-sibling-calls'
      recording.source_files   = 'CrashMonitor/Recording/**/*.{h,m,mm,c,cpp}',
                                 'CrashMonitor/llvm/**/*.{h,m,mm,c,cpp}',
                                 'CrashMonitor/swift/**/*.{h,m,mm,c,cpp,def}',
                                 'CrashMonitor/Reporting/Filters/GrowingCrashReportFilter.h'
      recording.public_header_files = 'CrashMonitor/Recording/GrowingCrash.h',
                                      'CrashMonitor/Recording/GrowingCrashC.h',
                                      'CrashMonitor/Recording/GrowingCrashReportWriter.h',
                                      'CrashMonitor/Recording/GrowingCrashReportFields.h',
                                      'CrashMonitor/Recording/Monitors/GrowingCrashMonitorType.h',
                                      'CrashMonitor/Reporting/Filters/GrowingCrashReportFilter.h'

      recording.subspec 'Tools' do |tools|
        tools.source_files = 'CrashMonitor/Recording/Tools/*.h'
        tools.compiler_flags = '-fno-optimize-sibling-calls'
      end
    end

    crashmonitor.subspec 'Reporting' do |reporting|
      reporting.dependency 'GrowingAPM/CrashMonitor/Recording'

      reporting.subspec 'Filters' do |filters|
        filters.source_files = 'CrashMonitor/Reporting/Filters/**/*.{h,m,mm,c,cpp}'
        filters.public_header_files = 'CrashMonitor/Reporting/Filters/*.h'
      end

      reporting.subspec 'Tools' do |tools|
        tools.source_files = 'CrashMonitor/Reporting/Tools/**/*.{h,m,mm,c,cpp}'
      end
    end

    crashmonitor.subspec 'Installations' do |installations|
      installations.dependency 'GrowingAPM/CrashMonitor/Recording'
      installations.dependency 'GrowingAPM/CrashMonitor/Reporting'
      installations.source_files = 'CrashMonitor/Installations/**/*.{h,m,mm,c,cpp}'
    end
  end
end
