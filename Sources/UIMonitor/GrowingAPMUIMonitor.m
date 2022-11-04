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

static BOOL kIsActivePrewarm = NO;
static double kLoadTime = 0;
static double kCppInitTime = 0;
static double kMainStartTime = 0;
static double kDidFinishLaunchingStartTime = 0;
static double kFirstPageDidAppearTime = 0;
static double kMaxColdRebootDuration = 30 * 1000L;

@interface GrowingAPMUIMonitor () <GrowingAPMMonitor, GrowingAppLifecycleDelegate>

@property (strong, nonatomic, readonly) NSPointerArray *delegates;
@property (strong, nonatomic, readonly) NSLock *delegateLock;

@property (nonatomic, copy) NSString *firstPageName;
@property (nonatomic, assign) double firstPageloadDuration;
@property (nonatomic, assign) BOOL didSendColdReboot;

@property (nonatomic, strong) NSMutableArray *ignoredPrivateControllers;

@end

@implementation GrowingAPMUIMonitor

- (instancetype)init {
    self = [super init];
    if (self) {
        _delegates = [NSPointerArray pointerArrayWithOptions:NSPointerFunctionsWeakMemory];
        _delegateLock = [[NSLock alloc] init];
    }

    return self;
}

+ (instancetype)sharedInstance {
    static id _sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _sharedInstance = [[self alloc] init];
    });
    return _sharedInstance;
}

#pragma mark - Monitor

+ (void)setup:(Class)appDelegateClass {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        [UIViewController growingapm_startUIMonitorSwizzle];
        
        kIsActivePrewarm = [[NSProcessInfo processInfo].environment[@"ActivePrewarm"] isEqualToString:@"1"];
        if (kIsActivePrewarm) {
            // 依据 Sentry SDK 的注释可知，iOS 14 设备上 App 启动就已经有 prewarming
            // https://github.com/getsentry/sentry-cocoa/blob/0979ac6c30342058175f08b68d136e42b5187c43/Sources/Sentry/SentryAppStartTracker.m#L74
            if (@available(iOS 14, *)) {
                
                // 使用 hook 方式来实现虽然侵入性大，但考虑到 GrowingAnalyticsSDK + GrowingAPMSDK 延迟初始化的场景，比起在
                // -[GrowingAPMUIMonitor startMonitor] 中直接计算 didFinishLaunchingStartTime 更合理一些
                
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
            }
        } else {
            kMainStartTime = GrowingTimeUtil.currentTimeMillis;
        }
    });
}

- (void)startMonitor {
    [GrowingAppLifecycle.sharedInstance addAppLifecycleDelegate:self];
    
    // 延迟初始化时，补发 cold reboot (如果未发)
    dispatch_async(dispatch_get_main_queue(), ^{
        [self sendColdReboot];
    });
}

- (void)addMonitorDelegate:(id <GrowingAPMUIMonitorDelegate>)delegate {
    [self.delegateLock lock];
    if (![self.delegates.allObjects containsObject:delegate]) {
        [self.delegates addPointer:(__bridge void *)delegate];
    }
    [self.delegateLock unlock];
}

- (void)removeMonitorDelegate:(id <GrowingAPMUIMonitorDelegate>)delegate {
    [self.delegateLock lock];
    [self.delegates.allObjects enumerateObjectsWithOptions:NSEnumerationReverse usingBlock:^(NSObject *obj, NSUInteger idx, BOOL *_Nonnull stop) {
        if (delegate == obj) {
            [self.delegates removePointerAtIndex:idx];
            *stop = YES;
        }
    }];
    [self.delegateLock unlock];
}

#pragma mark - Private Method

static double getExecTime(void) {
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

+ (void)load {
    kLoadTime = GrowingTimeUtil.currentTimeMillis;
}

__used __attribute__((constructor(60000))) static void beforeMain(void) {
    // c++ init 时间
    kCppInitTime = GrowingTimeUtil.currentTimeMillis;
}

static BOOL applicationDidFinishLaunchingWithOptions(NSInvocation *invocation,
                                                     id delegate,
                                                     UIApplication *application,
                                                     NSDictionary *launchOptions) {
    // 获取 didFinishLaunching 开始时间
    if (!invocation) {
        return NO;
    }

    kDidFinishLaunchingStartTime = GrowingTimeUtil.currentTimeMillis;

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
        || kFirstPageDidAppearTime == 0) {
        return;
    }
    
    // 实际冷启动过程：
    // exec -> load -> c++ init -> main -> didFinishLaunching -> first_render_time -> first_vc_loadView -> first_vc_didAppear
    
    // 1. 对于 prewarming，官方原话是：Prewarming executes an app’s launch sequence up until, but not including, when main() calls UIApplicationMain(_:_:_:_:).
    // https://developer.apple.com/documentation/uikit/app_and_environment/responding_to_the_launch_of_your_app/about_the_app_launch_sequence#3894431
    // 2. stackoverflow 上有人提到支持 scenes 时，didFinishLaunching 也会在 prewarming 期间调用：
    // https://stackoverflow.com/questions/71025205/ios-15-prewarming-causing-appwilllaunch-method-when-prewarm-is-done
    // 3. 真机实测 iPhoneXR iOS 15.1，prewarming 会执行到 main；iPhone12 Pro iOS 16.1，prewarming 仅执行到 exec
    // 4. sentry-cocoa 不统计 prewarming 状态下的冷启动耗时:
    // https://github.com/getsentry/sentry-cocoa/blob/0e2e5938cd3bf4c83cc9158efea42b5988395d09/Sources/Sentry/SentryAppStartTracker.m#L126
    
    // 因此，针对于以上各场景兼容，计算冷启动时长方式如下：
    
    // 当前 SDK 监控 pre-main 和 after-main 2个阶段：

    // pre-main(由于 iOS 15+ prewarming 的原因，分别计算):
    // iOS 15+ prewarming: zero
    // cold reboot: exec to c++ init

    // after-main(由于 iOS 15+ prewarming 的原因，分别计算):
    // iOS 15+ prewarming: didFinishLaunching to first_vc_didAppear
    // cold reboot: main to first_vc_didAppear

    // total:
    // pre-main + after-main
    
    // **并且，如计算结果大于最大冷启动时长(考虑第 2 点，after-main 计算值有可能异常)，则不上报该次冷启动**
    // **同样地，为了向后兼容，计算结果大于最大冷启动或小于 0，则不上报该次冷启动*8
    
    double execTime = getExecTime();
    double preMainTime = (kIsActivePrewarm || execTime == 0 || kCppInitTime == 0) ? 0 : (kCppInitTime - execTime);
    double afterMainTime = kIsActivePrewarm ? (kFirstPageDidAppearTime - kDidFinishLaunchingStartTime)
                                            : (kFirstPageDidAppearTime - kMainStartTime);
    double total = preMainTime + afterMainTime;
    
    [self.delegateLock lock];
    for (id delegate in self.delegates) {
        if ([delegate respondsToSelector:@selector(growingapm_UIMonitorHandleWithPageName:loadDuration:rebootTime:isWarm:)]) {
            [delegate growingapm_UIMonitorHandleWithPageName:self.firstPageName
                                                loadDuration:self.firstPageloadDuration
                                                  rebootTime:(total > kMaxColdRebootDuration || total < 0) ? 0 : total
                                                      isWarm:NO];
        }
    }
    [self.delegateLock unlock];
    self.didSendColdReboot = YES;
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
        if (kFirstPageDidAppearTime == 0) {
            self.firstPageName = pageName;
            self.firstPageloadDuration = loadDuration;
            kFirstPageDidAppearTime = GrowingTimeUtil.currentTimeMillis;
        }
        
        [self sendColdReboot];
    } else {
        // usual page loading
        [self.delegateLock lock];
        for (id delegate in self.delegates) {
            if ([delegate respondsToSelector:@selector(growingapm_UIMonitorHandleWithPageName:loadDuration:rebootTime:isWarm:)]) {
                [delegate growingapm_UIMonitorHandleWithPageName:pageName
                                                    loadDuration:loadDuration
                                                      rebootTime:0
                                                          isWarm:NO];
            }
        }
        [self.delegateLock unlock];
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

- (NSDictionary *)coldRebootMonitorDetails {
    return @{
        @"isActivePrewarm" : kIsActivePrewarm ? @"YES" : @"NO",
        @"exec" : @(getExecTime()),
        @"load" : @(kLoadTime),
        @"C++ Init" : @(kCppInitTime),
        @"main" : @(kMainStartTime),
        @"didFinishLaunching" : @(kDidFinishLaunchingStartTime),
        @"firstVCDidAppear" : @(kFirstPageDidAppearTime),
        @"execTofirstVCDidAppear" : @(kFirstPageDidAppearTime - getExecTime())
    };
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
    [self.delegateLock lock];
    for (id delegate in self.delegates) {
        if ([delegate respondsToSelector:@selector(growingapm_UIMonitorHandleWithPageName:loadDuration:rebootTime:isWarm:)]) {
            [delegate growingapm_UIMonitorHandleWithPageName:NSStringFromClass([curController class])
                                                loadDuration:duration
                                                  rebootTime:duration
                                                      isWarm:YES];
        }
    }
    [self.delegateLock unlock];
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

@end
