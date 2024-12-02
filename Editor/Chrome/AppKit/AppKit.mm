#import <AppKit/AppKit.h>
#include <iostream>
#import <Cocoa/Cocoa.h>
#include <PulsarCore/Log.hpp>

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    auto result = Pulsar::Log::Init();
    if (!result.has_value()) {
        fmt::println("Failed to create logger: {}", result.error());
    }

    PL_LOG_INFO("Pulsar Editor Started...");
}

@end

int main([[maybe_unused]] int argc, [[maybe_unused]] const char* argv[]) {
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        AppDelegate* delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}
