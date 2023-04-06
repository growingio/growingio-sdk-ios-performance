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

##### 2023.4.6 更新

~~请使用 **Xcode 13** 进行打包，Xcode 14 对 _objc_msgSend 调用进行了[优化](https://www.wwdcnotes.com/notes/wwdc22/110363/)，以及[废弃了 bitcode](https://developer.apple.com/documentation/Xcode-Release-Notes/xcode-14-release-notes#Deprecations)，将导致 Xcode 14 的打包产物在 Xcode 13 上进行链接时报错。~~

> ## App Store submission requirement starts April 25
>
> March 28, 2023
>
> Starting April 25, 2023, iOS, iPadOS, and watchOS apps submitted to the App Store must be built with Xcode 14.1 or later. The latest version of Xcode 14, which includes the latest SDKs for iOS 16, iPadOS 16, and watchOS 9, is available for free on the [Mac App Store](https://apps.apple.com/us/app/xcode/id497799835?mt=12).
>
> See: https://developer.apple.com/news/?id=jd9wcyov

由于上述原因，自 2023.4.25 起，使用 **Xcode 14** 进行打包

> 另外，如果使用者为企业级应用开发者，通过内部平台进行分发，不需要上传至 App Store，那么还是有可能使用 Xcode 13 进行日常开发，这就需要按照下方的兼容步骤进行修改打包使用
> See: https://support.apple.com/zh-cn/guide/deployment/depce7cefc4d/web

##### 打包机使用 Xcode14 编译，兼容使用者为 Xcode 13 环境

 - 在打包机 (Xcode14)，修改 ./Project/GrowingAPM/generate_xcframework.sh 中 xcodebuild 命令参数，添加 `OTHER_CFLAGS="-fno-objc-msgsend-selector-stubs"`
 - 在使用者环境 (Xcode13)，设置需要链接的主工程 `ENABLE_BITCODE = NO`，并在 Podfile 添加 `post_install` 钩子之后，再执行 pod install：

```ruby
post_install do |installer|
  installer.pods_project.targets.each do |target|
    target.build_configurations.each do |config|
      config.build_settings['ENABLE_BITCODE'] = 'NO'
    end
  end
end
```
