//
//  main.m
//  Example
//
//  Created by YoloMao on 2022/10/8.
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"
#import "GrowingAPM.h"

int main(int argc, char * argv[]) {
    [GrowingAPM setupMonitors];
    NSString * appDelegateClassName;
    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
        appDelegateClassName = NSStringFromClass([AppDelegate class]);
    }
    return UIApplicationMain(argc, argv, nil, appDelegateClassName);
}
