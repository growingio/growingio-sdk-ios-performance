GrowingIO APM
======
![GrowingIO](https://www.growingio.com/vassets/images/home_v3/gio-logo-primary.svg) 

### SDK 简介

**GrowingAPM** 提供移动端性能采集分析功能，包括崩溃分析、启动分析、页面加载分析等。

### 如何集成

#### 源码集成（对内调试）

1. 将 Sources 文件夹内的所有文件拷贝至您的项目根目录
2. 在 Podfile 中添加 `pod 'GrowingAPM', :path => './'`
3. 执行 pod install 安装并运行您的项目

#### 二进制集成（对外，后续可改为私有 Pod 方式上传集成）

1. 执行 `sh ./generate_zip.sh`
2. 将生成的 GrowingAPM.zip 解压缩
3. 将解压后的 GrowingAPM 文件夹内的所有文件拷贝至您的项目根目录
4. 在 Podfile 中添加 `pod 'GrowingAPM', :path => './'`
5. 执行 pod install 安装并运行您的项目

#### 按需集成对应模块

```ruby
# 按照所需模块自由组合
pod 'GrowingAPM/UIMonitor', :path => './'
pod 'GrowingAPM/LaunchMonitor', :path => './'
pod 'GrowingAPM/CrashMonitor', :path => './'
# ...
```

### 初始化SDK

- 在 main.m 以及 AppDelegate.m 导入 GrowingAPM：

```objc
#import "GrowingAPM.h"
```

- 在 main 函数中，添加如下代码 (此操作仅为了尽可能地在 App 运行前 Swizzle，除必要的崩溃日志本地缓存外，不会生成、上报数据)：

```objc
int main(int argc, char * argv[]) {
    // GrowingAPM Swizzle
    [GrowingAPM swizzle:GrowingAPMMonitorsCrash | GrowingAPMMonitorsLaunch | GrowingAPMMonitorsUserInterface];
    NSString * appDelegateClassName;
    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
        appDelegateClassName = NSStringFromClass([AppDelegate class]);
    }
    return UIApplicationMain(argc, argv, nil, appDelegateClassName);
}
```

- 在 AppDelegate.m application:didFinishLaunchingWithOptions: 中初始化 GrowingAPM：

```objc
// 添加 GrowingAPM 初始化配置
GrowingAPMConfig *config = GrowingAPMConfig.config;
// 根据您需要的监控类型
config.monitors = GrowingAPMMonitorsCrash | GrowingAPMMonitorsLaunch | GrowingAPMMonitorsUserInterface;
// 初始化 GrowingAPM
[GrowingAPM startWithConfig:config];
```

