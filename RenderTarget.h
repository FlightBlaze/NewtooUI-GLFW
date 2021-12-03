#pragma once

#include <MyProject.h>
#include <Diligent.h>

class RenderTarget {
public:
    RenderTarget(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice,
                 Diligent::TEXTURE_FORMAT colorBufferFormat,
                 Diligent::TEXTURE_FORMAT depthBufferFormat,
                 int width, int height, int numSamples = 1);
    RenderTarget();
    
    void clear(Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context,
               const float color[4]);
    
    void bind(Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context);
    
    Diligent::RefCntAutoPtr<Diligent::ITexture> color;
    Diligent::RefCntAutoPtr<Diligent::ITexture> depth;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> colorSRV;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> RTV;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> DSV;
};
