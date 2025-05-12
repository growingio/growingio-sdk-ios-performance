Pod::Spec.new do |s|
  s.name             = 'GrowingAPM'
  s.version          = '1.0.2'
  s.summary          = 'iOS SDK of GrowingIO.'
  s.description      = <<-DESC
GrowingAPM提供移动端性能采集分析功能，包括崩溃分析、启动分析、页面加载分析等。
                       DESC
  s.homepage         = 'https://www.growingio.com/'
  s.license          = { :type => 'Apache2.0', :file => 'LICENSE' }
  s.author           = { 'GrowingIO' => 'support@growingio.com' }
  s.source           = { :git => 'https://github.com/growingio/growingio-sdk-ios-performance-ext.git', :tag => s.version.to_s }
  s.ios.deployment_target = '10.0'
  s.frameworks = 'Foundation'
  s.requires_arc = true
  s.pod_target_xcconfig = { "OTHER_LDFLAGS" => '$(inherited) -ObjC'}
  
  s.subspec 'Core' do |core|
    core.dependency 'GrowingUtils/TrackerCore'
    core.source_files = 'Core/**/*.{h,m,mm,c,cpp}'
  end

  s.subspec 'CrashMonitor' do |monitor|
    monitor.vendored_framework = "CrashMonitor/GrowingAPMCrashMonitor.xcframework"
    monitor.resource_bundles = {'GrowingAPMCrashMonitor' => ['CrashMonitor/Resources/GrowingAPMCrashMonitor.bundle/PrivacyInfo.xcprivacy']}
    monitor.libraries = 'c++', 'z'
    monitor.pod_target_xcconfig = { 'GCC_ENABLE_CPP_EXCEPTIONS' => 'YES' }
    monitor.dependency 'GrowingAPM/Core'
  end

  s.subspec 'UIMonitor' do |monitor|
    monitor.vendored_framework = "UIMonitor/GrowingAPMUIMonitor.xcframework"
    monitor.dependency 'GrowingAPM/Core'
  end
end
