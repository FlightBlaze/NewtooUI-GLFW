#include <platform/MacOS/LayerManager.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#import <Metal/Metal.h>

void* setupLayersAndGetView(void* window) {
    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    NSWindow* nswindow = (NSWindow*)window;
    id nsview = nswindow.contentView;
    CGSize viewScale = [nsview convertSizeToBacking: CGSizeMake(1.0, 1.0)];
    // metalLayer.contentsScale = MIN(viewScale.width, viewScale.height);
    [nsview setLayer:metalLayer];
    [nsview setWantsLayer:YES];
    return nsview;
}
