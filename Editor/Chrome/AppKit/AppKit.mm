#include "PulsarCore/Log.hpp"

#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#include <iostream>

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    auto result = Pulsar::Log::init();
    if (!result.has_value()) {
        fmt::println("Failed to create logger: {}", result.error());
    }

    PL_LOG_INFO("Pulsar Editor Started...");

    NSWindow* window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 800, 600) styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];
    [window setTitle:@"Pulsar Editor"];
    [window center];
    [window makeKeyAndOrderFront:nil];

    NSView* view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 800, 600)];
    [window setContentView:view];

    [NSApp activateIgnoringOtherApps:YES];
}

- (void)applicationWillTerminate:(NSNotification*)notification {
    PL_LOG_INFO("Pulsar Editor Terminated...");
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
