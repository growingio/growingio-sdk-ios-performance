//
//  GrowingAPMUIMonitor.m
//  GrowingAnalytics
//
//  Created by YoloMao on 2022/9/27.
//  Copyright (C) 2022 Beijing Yishu Technology Co., Ltd.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#import "GrowingAPMUIMonitor.h"
#import "GrowingAPMMonitor.h"
#import "UIViewController+GrowingUIMonitor.h"

#import "GrowingTimeUtil.h"
#import "GrowingAppLifecycle.h"
#import "GrowingSwizzle.h"

#import <objc/runtime.h>
#import <sys/sysctl.h>
#import <mach/mach.h>

@interface GrowingAPMUIMonitor () <GrowingAPMMonitor, GrowingAppLifecycleDelegate>

@property (class, nonatomic, assign) double mainStartTime;
@property (nonatomic, copy) NSString *firstPageName;
@property (nonatomic, assign) double firstPageloadDuration;
@property (nonatomic, assign) double firstPageDidAppearTime;
@property (nonatomic, assign) BOOL didSendColdReboot;

@property (nonatomic, strong) NSMutableArray *ignoredPrivateControllers;

@end

@implementation GrowingAPMUIMonitor

+ (instancetype)sharedInstance {
    static id _sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _sharedInstance = [[self alloc] init];
    });
    return _sharedInstance;
}

#pragma mark - Monitor

- (void)startMonitor {
    [GrowingAppLifecycle.sharedInstance addAppLifecycleDelegate:self];
    
    // 延迟初始化时，补发 cold reboot (如果未发)
    dispatch_async(dispatch_get_main_queue(), ^{
        [self sendColdReboot];
    });
}

+ (void)setup:(Class)appDelegateClass {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        [UIViewController growingapm_startUIMonitorSwizzle];
        
        BOOL isActivePrewarm = [[NSProcessInfo processInfo].environment[@"ActivePrewarm"] isEqualToString:@"1"];
        if (isActivePrewarm) {
            // 依据 Sentry SDK 的注释可知，iOS 14 设备上 App 启动就已经有 prewarming
            // https://github.com/getsentry/sentry-cocoa/blob/0979ac6c30342058175f08b68d136e42b5187c43/Sources/Sentry/SentryAppStartTracker.m#L74
            if (@available(iOS 14, *)) {
                
                // https://developer.apple.com/documentation/uikit/app_and_environment/responding_to_the_launch_of_your_app/about_the_app_launch_sequence#3894431
                // iOS 15.0+ 有 prewarming，main 函数的时间不准确
                // 使用 hook 方式来实现虽然侵入性大，但考虑到 GrowingAnalyticsSDK + GrowingAPMSDK 延迟初始化的场景，比起在
                // -[GrowingAPMUIMonitor startMonitor] 中直接计算 mainStartTime 更合理一些
                
                Class class = appDelegateClass;
                if (!class) {
                    return;
                }
                __block NSInvocation *invocation = nil;
                SEL selector = NSSelectorFromString(@"application:didFinishLaunchingWithOptions:");
                if (![class instancesRespondToSelector:selector]) {
                    return;
                }
                id block = ^(id delegate, UIApplication *app, NSDictionary *options) {
                    return applicationDidFinishLaunchingWithOptions(invocation, delegate, app, options);
                };
                invocation = [class growing_swizzleMethod:selector withBlock:block error:nil];
            } else {
                self.mainStartTime = GrowingTimeUtil.currentTimeMillis;
            }
        } else {
            self.mainStartTime = GrowingTimeUtil.currentTimeMillis;
        }
    });
}

#pragma mark - Private Method

static double getProcessStartTime(void) {
    // 获取进程开始时间
    struct kinfo_proc kProcInfo;
    int pid = [[NSProcessInfo processInfo] processIdentifier];
    int cmd[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, pid};
    size_t size = sizeof(kProcInfo);
    if (sysctl(cmd, sizeof(cmd)/sizeof(*cmd), &kProcInfo, &size, NULL, 0) == 0) {
        return kProcInfo.kp_proc.p_un.__p_starttime.tv_sec * 1000.0 + kProcInfo.kp_proc.p_un.__p_starttime.tv_usec / 1000.0;
    }
    return 0;
}

static double cppInitTime = 0;
static void __attribute__((constructor)) beforeMain(void) {
    // c++ init 时间
    cppInitTime = GrowingTimeUtil.currentTimeMillis;
}

static BOOL applicationDidFinishLaunchingWithOptions(NSInvocation *invocation,
                                                     id delegate,
                                                     UIApplication *application,
                                                     NSDictionary *launchOptions) {
    // 获取 didFinishLaunching 开始时间
    if (!invocation) {
        return NO;
    }

    GrowingAPMUIMonitor.mainStartTime = GrowingTimeUtil.currentTimeMillis;

    [invocation retainArguments];
    [invocation setArgument:&application atIndex:2];
    [invocation setArgument:&launchOptions atIndex:3];
    [invocation invokeWithTarget:delegate];

    BOOL ret = NO;
    [invocation getReturnValue:&ret];
    return ret;
}

- (void)sendColdReboot {
    if (self.didSendColdReboot) {
        return;
    }
    
    if (self.firstPageName.length == 0
        || self.firstPageloadDuration == 0
        || self.firstPageDidAppearTime == 0) {
        return;
    }
    
    // 实际冷启动过程：
    // exec -> load -> c++ init -> main -> didFinishLaunching -> first_render_time -> first_vc_loadView -> first_vc_didAppear

    // 当前 SDK 监控 pre-main 和 after-main 2个阶段：

    // pre-main:
    // exec to c++ init

    // after-main(由于 iOS 15+ prewarming 的原因，分别计算):
    // iOS 15+: didFinishLaunching to first_vc_didAppear
    // iOS 15-: main to first_vc_didAppear

    // total:
    // pre-main + after-main
    
    if (GrowingAPMUIMonitor.mainStartTime == 0) {
        // mainStartTime 没获取到（prewarming 且 hook UIApplicationDelegate 方法失败，可能未传入 AppDelegate Class）
        if (self.monitorBlock) {
            self.monitorBlock(self.firstPageName,
                              self.firstPageloadDuration,
                              0,
                              NO);
            self.didSendColdReboot = YES;
        }
        return;
    }
    
    double processStartTime = getProcessStartTime();
    double preMainTime = (processStartTime == 0 || cppInitTime == 0) ? 0 : (cppInitTime - processStartTime);
    double afterMainTime = self.firstPageDidAppearTime - GrowingAPMUIMonitor.mainStartTime;
    
    if (self.monitorBlock) {
        self.monitorBlock(self.firstPageName,
                          self.firstPageloadDuration,
                          preMainTime + afterMainTime,
                          NO);
        self.didSendColdReboot = YES;
    }
}

- (void)pageLoadCompletedWithViewController:(UIViewController *)viewController
                               loadViewTime:(double)loadViewTime
                            viewDidLoadTime:(double)viewDidLoadTime
                         viewWillAppearTime:(double)viewWillAppearTime
                          viewDidAppearTime:(double)viewDidAppearTime {
    NSString *pageName = NSStringFromClass([viewController class]);
    if ([pageName hasPrefix:@"GrowingTK"]) {
        return;
    }
    if ([self.ignoredPrivateControllers containsObject:pageName]) {
        return;
    }
    
    double loadDuration = loadViewTime + viewDidLoadTime + viewWillAppearTime + viewDidAppearTime;
    
    if (!self.didSendColdReboot) {
        // cold reboot
        if (self.firstPageDidAppearTime == 0) {
            self.firstPageName = pageName;
            self.firstPageloadDuration = loadDuration;
            self.firstPageDidAppearTime = GrowingTimeUtil.currentTimeMillis;
        }
        
        [self sendColdReboot];
    } else {
        // usual page loading
        if (self.monitorBlock) {
            self.monitorBlock(pageName, loadDuration, 0, NO);
        }
    }
}

- (UIViewController *)topViewController:(UIViewController *)controller {
    if ([controller isKindOfClass:[UINavigationController class]]) {
        return [self topViewController:[(UINavigationController *)controller topViewController]];
    } else if ([controller isKindOfClass:[UITabBarController class]]) {
        return [self topViewController:[(UITabBarController *)controller selectedViewController]];
    } else {
        return controller;
    }
    return nil;
}

- (UIWindow *)keyWindow {
    if (@available(iOS 13.0, *)) {
        for (UIWindowScene *windowScene in [UIApplication sharedApplication].connectedScenes) {
            if (windowScene.activationState == UISceneActivationStateForegroundActive) {
                NSArray *windows = windowScene.windows;
                for (UIWindow *window in windows) {
                    if (!window.hidden) {
                        return window;
                    }
                }
            }
        }
    }
    
    UIWindow *keyWindow = nil;
    if ([UIApplication.sharedApplication.delegate respondsToSelector:@selector(window)]) {
        keyWindow = UIApplication.sharedApplication.delegate.window;
    } else {
        NSArray *windows = [UIApplication sharedApplication].windows;
        for (UIWindow *window in windows) {
            if (!window.hidden) {
                keyWindow = window;
                break;
            }
        }
    }
    return keyWindow;
}

#pragma mark - GrowingAppLifecycleDelegate

- (void)applicationDidBecomeActive {
    GrowingAppLifecycle *appLifecycle = GrowingAppLifecycle.sharedInstance;
    if (appLifecycle.appDidEnterBackgroundTime == 0) {
        // 首次启动
        return;
    }
    if (appLifecycle.appWillEnterForegroundTime < appLifecycle.appWillResignActiveTime) {
        // 下拉通知或进入后台应用列表
        return;
    }
    
    // 动态获取当前页面
    UIViewController *curController = [self topViewController:self.keyWindow.rootViewController];
    while (curController.presentedViewController) {
        curController = [self topViewController:curController.presentedViewController];
    }
    if (!curController) {
        return;
    }
    
    // warm reboot
    double duration = appLifecycle.appDidBecomeActiveTime - appLifecycle.appWillEnterForegroundTime;
    if (self.monitorBlock) {
        self.monitorBlock(NSStringFromClass([curController class]), duration, duration, YES);
    }
}

#pragma mark - Getter & Setter

- (NSMutableArray *)ignoredPrivateControllers {
    if (!_ignoredPrivateControllers) {
        _ignoredPrivateControllers = [NSMutableArray arrayWithArray:@[
            @"UIInputWindowController",
            @"UIActivityGroupViewController",
            @"UIKeyboardHiddenViewController",
            @"UICompatibilityInputViewController",
            @"UISystemInputAssistantViewController",
            @"UIPredictionViewController",
            @"GrowingWindowViewController",
            @"UIApplicationRotationFollowingController",
            @"UIAlertController",
            @"FlutterViewController",
            @"UIEditingOverlayViewController" // did not call [super viewDidAppear:animated]
        ]];
    }
    return _ignoredPrivateControllers;
}

+ (double)mainStartTime {
    return ((NSNumber *)objc_getAssociatedObject(self, _cmd)).doubleValue;
}

+ (void)setMainStartTime:(double)time {
    objc_setAssociatedObject(self, @selector(mainStartTime), @(time), OBJC_ASSOCIATION_COPY_NONATOMIC);
}

@end
