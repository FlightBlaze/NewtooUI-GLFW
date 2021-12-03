#include <RenderTarget.h>

#include "Graphics/GraphicsTools/interface/CommonlyUsedStates.h"

RenderTarget::RenderTarget(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice,
             Diligent::TEXTURE_FORMAT colorBufferFormat,
             Diligent::TEXTURE_FORMAT depthBufferFormat,
             int width, int height, int numSamples)
{
    Diligent::TextureDesc colorDesc;
    colorDesc.Name = "Render Target Color";
    colorDesc.Type = Diligent::RESOURCE_DIM_TEX_2D;
    colorDesc.Width = width;
    colorDesc.Height = height;
    colorDesc.MipLevels = 1;
    colorDesc.Format = colorBufferFormat;
    colorDesc.SampleCount = numSamples;
    colorDesc.BindFlags = Diligent::BIND_SHADER_RESOURCE | Diligent::BIND_RENDER_TARGET;
    renderDevice->CreateTexture(colorDesc, nullptr, &color);
    colorSRV = color->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
    RTV = color->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);

    Diligent::TextureDesc depthDesc = colorDesc;
    depthDesc.Name = "Render Target Depth";
    depthDesc.Format = depthBufferFormat;
    depthDesc.SampleCount = numSamples;
    depthDesc.BindFlags = Diligent::BIND_SHADER_RESOURCE | Diligent::BIND_DEPTH_STENCIL;
    renderDevice->CreateTexture(depthDesc, nullptr, &depth);
    DSV = depth->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
}

RenderTarget::RenderTarget() {
    
}
    
void RenderTarget::clear(Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context,
                         const float color[4])
{
	context->ClearRenderTarget(RTV, color,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	context->ClearDepthStencil(DSV, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void RenderTarget::bind(Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context) {
    context->SetRenderTargets(1, &RTV, DSV,
          Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}
