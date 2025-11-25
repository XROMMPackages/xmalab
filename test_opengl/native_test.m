// Native macOS OpenGL test - no Qt involved
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

@interface GLView : NSOpenGLView
@property int frameCount;
@end

@implementation GLView

- (instancetype)initWithFrame:(NSRect)frameRect {
    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAAccelerated,
        0
    };
    NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    if (!pixelFormat) {
        NSLog(@"Failed to create pixel format with acceleration, trying without");
        NSOpenGLPixelFormatAttribute attrs2[] = {
            NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
            NSOpenGLPFAColorSize, 24,
            NSOpenGLPFAAlphaSize, 8,
            NSOpenGLPFADepthSize, 24,
            NSOpenGLPFADoubleBuffer,
            0
        };
        pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs2];
    }
    
    self = [super initWithFrame:frameRect pixelFormat:pixelFormat];
    if (self) {
        _frameCount = 0;
        NSLog(@"GLView created successfully");
    }
    return self;
}

- (void)prepareOpenGL {
    [super prepareOpenGL];
    [[self openGLContext] makeCurrentContext];
    NSLog(@"OpenGL Version: %s", glGetString(GL_VERSION));
    NSLog(@"OpenGL Renderer: %s", glGetString(GL_RENDERER));
    NSLog(@"OpenGL Vendor: %s", glGetString(GL_VENDOR));
}

- (void)drawRect:(NSRect)dirtyRect {
    _frameCount++;
    NSLog(@"drawRect called, frame %d", _frameCount);
    
    [[self openGLContext] makeCurrentContext];
    
    glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    NSLog(@"About to flushBuffer...");
    [[self openGLContext] flushBuffer];
    NSLog(@"flushBuffer completed!");
    
    if (_frameCount >= 10) {
        NSLog(@"SUCCESS: 10 frames rendered without crash!");
        [NSApp terminate:nil];
    } else {
        // Request another draw
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
            [self setNeedsDisplay:YES];
        });
    }
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        
        NSRect frame = NSMakeRect(100, 100, 400, 300);
        NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                                       styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];
        [window setTitle:@"Native OpenGL Test"];
        
        GLView *glView = [[GLView alloc] initWithFrame:frame];
        [window setContentView:glView];
        [window makeKeyAndOrderFront:nil];
        
        NSLog(@"Entering run loop...");
        [app run];
    }
    return 0;
}
