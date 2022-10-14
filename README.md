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


