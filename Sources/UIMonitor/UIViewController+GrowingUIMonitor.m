//
//  UIViewController+GrowingUIMonitor.m
//  GrowingAnalytics
//
//  Created by YoloMao on 2022/10/10.
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

#import "UIViewController+GrowingUIMonitor.h"
#import "GrowingAPMUIMonitor+Private.h"
#import "GrowingTimeUtil.h"
#import "GrowingSwizzle.h"
#import <objc/runtime.h>

static void growingapm_loadView(UIViewController *self, SEL sel) {
    Class originClass = class_getSuperclass(object_getClass(self));
    IMP originIMP = method_getImplementation(class_getInstanceMethod(originClass, sel));
    assert(originIMP != NULL);
    
    void (*func)(UIViewController *, SEL) = (void (*)(UIViewController *, SEL))originIMP;

    double startTime = [GrowingTimeUtil currentSystemTimeMillis];
    func(self, sel);
    double endTime = [GrowingTimeUtil currentSystemTimeMillis];
    self.growingapm_loadViewTime = endTime - startTime;
}

static void growingapm_viewDidLoad(UIViewController *self, SEL sel) {
    Class originClass = class_getSuperclass(object_getClass(self));
    IMP originIMP = method_getImplementation(class_getInstanceMethod(originClass, sel));
    assert(originIMP != NULL);
    
    void (*func)(UIViewController *, SEL) = (void (*)(UIViewController *, SEL))originIMP;

    double startTime = [GrowingTimeUtil currentSystemTimeMillis];
    func(self, sel);
    double endTime = [GrowingTimeUtil currentSystemTimeMillis];
    self.growingapm_viewDidLoadTime = endTime - startTime;
}

static void growingapm_viewWillAppear(UIViewController *self, SEL sel, BOOL animated) {
    Class originClass = class_getSuperclass(object_getClass(self));
    IMP originIMP = method_getImplementation(class_getInstanceMethod(originClass, sel));
    assert(originIMP != NULL);
    
    void (*func)(UIViewController *, SEL, BOOL) = (void (*)(UIViewController *, SEL, BOOL))originIMP;

    double startTime = [GrowingTimeUtil currentSystemTimeMillis];
    func(self, sel, animated);
    double endTime = [GrowingTimeUtil currentSystemTimeMillis];
    self.growingapm_viewWillAppearTime = endTime - startTime;
}

static void growingapm_viewDidAppear(UIViewController *self, SEL sel, BOOL animated) {
    Class originClass = class_getSuperclass(object_getClass(self));
    IMP originIMP = method_getImplementation(class_getInstanceMethod(originClass, sel));
    assert(originIMP != NULL);
    
    void (*func)(UIViewController *, SEL, BOOL) = (void (*)(UIViewController *, SEL, BOOL))originIMP;

    double startTime = [GrowingTimeUtil currentSystemTimeMillis];
    func(self, sel, animated);
    double endTime = [GrowingTimeUtil currentSystemTimeMillis];
    self.growingapm_viewDidAppearTime = endTime - startTime;
    
    if (!self.growingapm_didAppear) {
        [[GrowingAPMUIMonitor sharedInstance] pageLoadCompletedWithViewController:self
                                                                     loadViewTime:self.growingapm_loadViewTime
                                                                  viewDidLoadTime:self.growingapm_viewDidLoadTime
                                                               viewWillAppearTime:self.growingapm_viewWillAppearTime
                                                                viewDidAppearTime:self.growingapm_viewDidAppearTime];
        self.growingapm_didAppear = YES;
    }
}

#pragma mark - KVO Helpers

@interface GrowingAPMKVOObserverStub : NSObject

+ (instancetype)stub;

@end

@implementation GrowingAPMKVOObserverStub

+ (instancetype)stub {
    static id sharedInstance;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[self alloc] init];
    });
    return sharedInstance;
}

@end

@interface GrowingAPMKVORemover : NSObject

@property (nonatomic, unsafe_unretained) id obj;
@property (nonatomic, copy) NSString *keyPath;

@end

@implementation GrowingAPMKVORemover

- (void)dealloc {
    [_obj removeObserver:[GrowingAPMKVOObserverStub stub] forKeyPath:_keyPath];
    _obj = nil;
}

@end

#pragma mark - UIViewController (GrowingUIMonitor)

@implementation UIViewController (GrowingUIMonitor)

+ (void)growingapm_startUIMonitorSwizzle {
    [self growing_swizzleMethod:@selector(initWithNibName:bundle:)
                     withMethod:@selector(growingapm_initWithNibName:bundle:)
                          error:nil];
    
    [self growing_swizzleMethod:@selector(initWithCoder:)
                     withMethod:@selector(growingapm_initWithCoder:)
                          error:nil];
}

#pragma mark - Swizzle Method

- (instancetype)growingapm_initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    [self growingapm_iSASwizzle];
    return [self growingapm_initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
}

- (instancetype)growingapm_initWithCoder:(NSCoder *)coder {
    [self growingapm_iSASwizzle];
    return [self growingapm_initWithCoder:coder];
}

static char kAssociateRemoveKey;
- (void)growingapm_iSASwizzle {
    NSString *identifier = [NSString stringWithFormat:@"growingapm_%@", [[NSProcessInfo processInfo] globallyUniqueString]];
    [self addObserver:[GrowingAPMKVOObserverStub stub]
           forKeyPath:identifier
              options:NSKeyValueObservingOptionNew
              context:nil];

    GrowingAPMKVORemover *remover = [GrowingAPMKVORemover new];
    remover.obj = self;
    remover.keyPath = identifier.copy;
    objc_setAssociatedObject(self, &kAssociateRemoveKey, remover, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

    // NSKVONotifying_ViewController
    Class class = object_getClass(self);
    // ViewController
    Class originClass = class_getSuperclass(class);
    
    IMP currentViewDidLoadImp = class_getMethodImplementation(class, @selector(viewDidLoad));
    if (currentViewDidLoadImp == (IMP)growingapm_viewDidLoad) {
        return;
    }

    const char *originLoadViewEncoding =
        method_getTypeEncoding(class_getInstanceMethod(originClass, @selector(loadView)));
    const char *originViewDidLoadEncoding =
        method_getTypeEncoding(class_getInstanceMethod(originClass, @selector(viewDidLoad)));
    const char *originViewDidAppearEncoding =
        method_getTypeEncoding(class_getInstanceMethod(originClass, @selector(viewDidAppear:)));
    const char *originViewWillAppearEncoding =
        method_getTypeEncoding(class_getInstanceMethod(originClass, @selector(viewWillAppear:)));
    
    class_addMethod(class, @selector(loadView), (IMP)growingapm_loadView, originLoadViewEncoding);
    class_addMethod(class, @selector(viewDidLoad), (IMP)growingapm_viewDidLoad, originViewDidLoadEncoding);
    class_addMethod(class, @selector(viewDidAppear:), (IMP)growingapm_viewDidAppear, originViewDidAppearEncoding);
    class_addMethod(class, @selector(viewWillAppear:), (IMP)growingapm_viewWillAppear, originViewWillAppearEncoding);
}

#pragma mark - Associated Object

- (double)growingapm_loadViewTime {
    return ((NSNumber *)objc_getAssociatedObject(self, _cmd)).doubleValue;
}

- (void)setGrowingapm_loadViewTime:(double)time {
    objc_setAssociatedObject(self, @selector(growingapm_loadViewTime), @(time), OBJC_ASSOCIATION_COPY_NONATOMIC);
}

- (double)growingapm_viewDidLoadTime {
    return ((NSNumber *)objc_getAssociatedObject(self, _cmd)).doubleValue;
}

- (void)setGrowingapm_viewDidLoadTime:(double)time {
    objc_setAssociatedObject(self, @selector(growingapm_viewDidLoadTime), @(time), OBJC_ASSOCIATION_COPY_NONATOMIC);
}

- (double)growingapm_viewWillAppearTime {
    return ((NSNumber *)objc_getAssociatedObject(self, _cmd)).doubleValue;
}

- (void)setGrowingapm_viewWillAppearTime:(double)time {
    objc_setAssociatedObject(self, @selector(growingapm_viewWillAppearTime), @(time), OBJC_ASSOCIATION_COPY_NONATOMIC);
}

- (double)growingapm_viewDidAppearTime {
    return ((NSNumber *)objc_getAssociatedObject(self, _cmd)).doubleValue;
}

- (void)setGrowingapm_viewDidAppearTime:(double)time {
    objc_setAssociatedObject(self, @selector(growingapm_viewDidAppearTime), @(time), OBJC_ASSOCIATION_COPY_NONATOMIC);
}

- (BOOL)growingapm_didAppear {
    return ((NSNumber *)objc_getAssociatedObject(self, _cmd)).boolValue;
}

- (void)setGrowingapm_didAppear:(BOOL)didAppear {
    objc_setAssociatedObject(self, @selector(growingapm_didAppear), @(didAppear), OBJC_ASSOCIATION_COPY_NONATOMIC);
}

@end
