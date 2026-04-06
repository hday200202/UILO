#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "UILO.hpp"
#include "Factory.hpp"
#include <iostream>

using namespace uilo;

// Flat vertex passed to the Metal pipeline
struct MtlVertex { float x, y, r, g, b, a; };

// Shaders: vertex transforms pixel coords to NDC, fragment is pass-through
static NSString* const kShaders = @
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct VI { float2 p [[attribute(0)]]; float4 c [[attribute(1)]]; };\n"
    "struct VO { float4 p [[position]]; float4 c; };\n"
    "vertex VO v(VI in [[stage_in]], constant float2& sz [[buffer(1)]]) {\n"
    "    VO o;\n"
    "    o.p = float4(in.p.x / sz.x * 2.0 - 1.0, 1.0 - in.p.y / sz.y * 2.0, 0.0, 1.0);\n"
    "    o.c = in.c;\n"
    "    return o;\n"
    "}\n"
    "fragment float4 f(VO in [[stage_in]]) { return in.c; }\n";

int main() {
    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        [app finishLaunching];

        const float W = 800, H = 600;

        NSWindow* win = [[NSWindow alloc]
            initWithContentRect:NSMakeRect(0, 0, W, H)
            styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
            backing:NSBackingStoreBuffered
            defer:NO];
        [win setTitle:@"UILO - metal_basic"];
        [win center];
        [win makeKeyAndOrderFront:nil];
        [app activateIgnoringOtherApps:YES];

        float scale = (float)win.backingScaleFactor;
        float pw = W * scale, ph = H * scale;

        // --- Metal setup ---
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        CAMetalLayer* layer  = [CAMetalLayer layer];
        layer.device         = device;
        layer.pixelFormat    = MTLPixelFormatBGRA8Unorm;
        layer.drawableSize   = CGSizeMake(pw, ph);
        layer.contentsScale  = scale;
        [win.contentView setWantsLayer:YES];
        [win.contentView setLayer:layer];

        id<MTLCommandQueue> queue = [device newCommandQueue];

        NSError* err = nil;
        id<MTLLibrary> lib = [device newLibraryWithSource:kShaders options:nil error:&err];
        if (!lib) { NSLog(@"Shader compile error: %@", err); return 1; }

        MTLVertexDescriptor* vd       = [MTLVertexDescriptor new];
        vd.attributes[0].format       = MTLVertexFormatFloat2;
        vd.attributes[0].offset       = offsetof(MtlVertex, x);
        vd.attributes[0].bufferIndex  = 0;
        vd.attributes[1].format       = MTLVertexFormatFloat4;
        vd.attributes[1].offset       = offsetof(MtlVertex, r);
        vd.attributes[1].bufferIndex  = 0;
        vd.layouts[0].stride          = sizeof(MtlVertex);

        MTLRenderPipelineDescriptor* pd = [MTLRenderPipelineDescriptor new];
        pd.vertexFunction               = [lib newFunctionWithName:@"v"];
        pd.fragmentFunction             = [lib newFunctionWithName:@"f"];
        pd.vertexDescriptor             = vd;
        pd.colorAttachments[0].pixelFormat               = MTLPixelFormatBGRA8Unorm;
        pd.colorAttachments[0].blendingEnabled           = YES;
        pd.colorAttachments[0].sourceRGBBlendFactor      = MTLBlendFactorSourceAlpha;
        pd.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pd.colorAttachments[0].sourceAlphaBlendFactor    = MTLBlendFactorOne;
        pd.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

        id<MTLRenderPipelineState> pso = [device newRenderPipelineStateWithDescriptor:pd error:&err];
        if (!pso) { NSLog(@"Pipeline error: %@", err); return 1; }

        // --- UILO layout (mirrors sfml_basic) ---
        auto* header     = row(Modifier().setHeight(60_px).setColor(Colors::Red).setRounded(16.f).setPadding(4.f), {});
        auto* leftPanel  = column(Modifier().setWidth(50_pct).setColor(Colors::Blue).setRounded(16.f).setPadding(4.f), {}, "leftPanel");
        auto* rightPanel = column(Modifier().setWidth(50_pct).setColor(Colors::Green).setRounded(16.f).setPadding(4.f), {});
        auto* content    = row(Modifier().setHeight(100_pct).setColor({30, 30, 30, 255}), {leftPanel, rightPanel});
        auto* footer     = row(Modifier().setHeight(40_px).setColor(Colors::Cyan).setRounded(16.f).setPadding(4.f), {});
        auto* root       = column(Modifier().setWidth(100_pct).setHeight(100_pct).setColor({25, 25, 25, 255}), {header, content, footer});

        UILO uilo;
        uilo.setScreenBounds({{0.f, 0.f}, {pw, ph}});
        uilo.setOnResize([&](float w, float h) { layer.drawableSize = CGSizeMake(w, h); });
        uilo.addPage(page(root, "main"));
        uilo.setPage("main");

        // --- Renderer: accumulate all draw calls into a single vertex list per frame ---
        std::vector<MtlVertex> verts;
        verts.reserve(4096);

        Renderer renderer;
        auto drawShape = [&](uilo::Rect* r) {
            for (uint32_t i : r->getIndices()) {
                const auto& v = r->getVertices()[i];
                verts.push_back({v.position.x, v.position.y,
                    v.color.r / 255.f, v.color.g / 255.f,
                    v.color.b / 255.f, v.color.a / 255.f});
            }
        };
        renderer.setDrawFilledRect([&](uilo::Rect* r) { drawShape(r); });
        renderer.setDrawRoundedRect([&](uilo::Rect* r, float radius) { r->setCornerRadius(radius); drawShape(r); });

        // --- Main loop ---
        __block bool running = true;
        id token = [[NSNotificationCenter defaultCenter]
            addObserverForName:NSWindowWillCloseNotification object:win queue:nil
            usingBlock:^(NSNotification*) { running = false; }];

        Input input;

        while (running) {
            @autoreleasepool {
                input.reset();

                NSEvent* ev;
                while ((ev = [app nextEventMatchingMask:NSEventMaskAny
                                             untilDate:nil
                                                inMode:NSDefaultRunLoopMode
                                               dequeue:YES])) {
                    if (ev.type == NSEventTypeKeyDown) {
                        if ([ev.characters isEqualToString:@"d"]) {
                            if (auto* el = uilo.getElement<Column>("leftPanel"))
                                el->erase();
                        } else if ([ev.characters isEqualToString:@"+"] || [ev.characters isEqualToString:@"="]) {
                            uilo.setScale(std::min(4.0f, uilo.getScale() + 0.25f));
                            std::cout << "Scale: " << uilo.getScale() << std::endl;
                        }
                        else if ([ev.characters isEqualToString:@"-"] || [ev.characters isEqualToString:@"_"]) {
                            uilo.setScale(std::max(0.25f, uilo.getScale() - 0.25f));
                            std::cout << "Scale: " << uilo.getScale() << std::endl;
                        }
                    }
                    if (ev.type == NSEventTypeLeftMouseDown)  input.leftMouse  = true;
                    if (ev.type == NSEventTypeRightMouseDown) input.rightMouse = true;
                    if (ev.type == NSEventTypeScrollWheel)    input.scrollDelta = (float)ev.deltaY;
                    [app sendEvent:ev];
                }

                // Resize
                NSSize cs = win.contentView.bounds.size;
                float npw = (float)(cs.width * scale), nph = (float)(cs.height * scale);
                if (npw != pw || nph != ph) {
                    pw = npw; ph = nph;
                    uilo.setScreenBounds({{0.f, 0.f}, {pw, ph}});
                }

                // Mouse: flip Y (Cocoa origin is bottom-left), scale to physical pixels
                NSPoint mp = [win.contentView convertPoint:[win mouseLocationOutsideOfEventStream] fromView:nil];
                input.mousePosition = {(float)(mp.x * scale), (float)((cs.height - mp.y) * scale)};

                uilo.update(input);

                // Render
                id<CAMetalDrawable> drawable = [layer nextDrawable];
                if (!drawable) continue;

                MTLRenderPassDescriptor* rpd        = [MTLRenderPassDescriptor new];
                rpd.colorAttachments[0].texture     = drawable.texture;
                rpd.colorAttachments[0].loadAction  = MTLLoadActionClear;
                rpd.colorAttachments[0].storeAction = MTLStoreActionStore;
                rpd.colorAttachments[0].clearColor  = MTLClearColorMake(0.08, 0.08, 0.08, 1.0);

                verts.clear();
                uilo.render(renderer);

                id<MTLCommandBuffer>        cmd = [queue commandBuffer];
                id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:rpd];
                [enc setRenderPipelineState:pso];

                if (!verts.empty()) {
                    float sz[2] = {pw, ph};
                    id<MTLBuffer> vbuf = [device newBufferWithBytes:verts.data()
                                                             length:verts.size() * sizeof(MtlVertex)
                                                            options:MTLResourceStorageModeShared];
                    [enc setVertexBuffer:vbuf offset:0 atIndex:0];
                    [enc setVertexBytes:sz length:sizeof(sz) atIndex:1];
                    [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:verts.size()];
                }

                [enc endEncoding];
                [cmd presentDrawable:drawable];
                [cmd commit];
            }
        }

        [[NSNotificationCenter defaultCenter] removeObserver:token];
    }
    return 0;
}
