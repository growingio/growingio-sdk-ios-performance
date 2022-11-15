GrowingIO APM (Internal)
======
![GrowingIO](https://www.growingio.com/vassets/images/home_v3/gio-logo-primary.svg) 

### SDK 简介

**GrowingAPM** 提供移动端性能采集分析功能，包括崩溃分析、启动分析、页面加载分析等。


#### 源码集成（对内调试）

1. 将 Sources 文件夹内的所有文件拷贝至您的项目根目录
2. 在 Podfile 中添加 `pod 'GrowingAPM', :path => './'`
3. 执行 pod install 安装并运行您的项目

#### 二进制集成（对外，生成的 zip 解压到 [GrowingAPM External](https://github.com/growingio/growingio-sdk-ios-performance-ext) 来发版，注意修改版本号）

1. 执行 `sh ./generate_zip.sh`
2. 将生成的 GrowingAPM.zip 解压缩
3. 将解压后的 GrowingAPM 文件夹内的所有文件拷贝至您的项目根目录
4. 在 Podfile 中添加 `pod 'GrowingAPM', :path => './'`
5. 执行 pod install 安装并运行您的项目

#### 注意

请使用 **Xcode 13** 进行打包，Xcode 14 对 _objc_msgSend 调用进行了[优化](https://www.wwdcnotes.com/notes/wwdc22/110363/)，以及[废弃了 bitcode](https://developer.apple.com/documentation/Xcode-Release-Notes/xcode-14-release-notes#Deprecations)，将导致 Xcode 14 的打包产物在 Xcode 13 上进行链接时报错。如果只能用 Xcode 14 进行打包，则需要先进行以下步骤：

 - 修改 ./Project/GrowingAPM/generate_xcframework.sh 中 xcodebuild 命令参数，添加 `OTHER_CFLAGS="-fno-objc-msgsend-selector-stubs"`
 - (Xcode 13) 设置需要链接的主工程 `ENABLE_BITCODE = NO`，并在 Podfile 添加 `post_install` 钩子之后，再执行 pod install：

```ruby
post_install do |installer|
  installer.pods_project.targets.each do |target|
    target.build_configurations.each do |config|
      config.build_settings['ENABLE_BITCODE'] = 'NO'
    end
  end
end
```
