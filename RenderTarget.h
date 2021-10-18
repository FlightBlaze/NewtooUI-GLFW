#pragma once

#include <MyProject.h>
#include <Diligent.h>

struct RenderTargetCreateInfo {
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  device;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapChain;
    Diligent::RefCntAutoPtr<Diligent::IShader> pixelShader;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> uniformBuffer;
    int width, height;
    bool alphaBlending = false;
};

class RenderTarget {
public:
	RenderTarget(RenderTargetCreateInfo& createInfo);

    void recreatePipelineState(RenderTargetCreateInfo& createInfo);
    void recreateTextures(RenderTargetCreateInfo& createInfo);

    void use(Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context);
    void clear(Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context,
        const float color[4]);
    void draw(Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context);

	Diligent::RefCntAutoPtr<Diligent::IPipelineState> PSO;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> SRB;
    Diligent::RefCntAutoPtr<Diligent::ITexture> color;
    Diligent::RefCntAutoPtr<Diligent::ITexture> depth;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> colorRTV;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> depthDSV;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> uniformBuffer;

	static constexpr Diligent::TEXTURE_FORMAT RenderTargetFormat = Diligent::TEX_FORMAT_RGBA8_UNORM;
	static constexpr Diligent::TEXTURE_FORMAT DepthBufferFormat = Diligent::TEX_FORMAT_D32_FLOAT;

private:

    Diligent::RefCntAutoPtr<Diligent::IShader> mVertexShader;
};

static const char* RenderTargetVSSource = R"(
struct VSInput
{
    uint VertexID : SV_VertexID;
};

struct PSInput
{
    float4 Pos : SV_POSITION;
    float2 UV  : TEX_COORD;
};

void main(in  VSInput VSIn,
          out PSInput PSIn)
{
    float4 Pos[4];
    Pos[0] = float4(-1.0, -1.0, 0.0, 1.0);
    Pos[1] = float4(-1.0, +1.0, 0.0, 1.0);
    Pos[2] = float4(+1.0, -1.0, 0.0, 1.0);
    Pos[3] = float4(+1.0, +1.0, 0.0, 1.0);

    float2 UV[4];
    UV[0] = float2(+0.0, +1.0);
    UV[1] = float2(+0.0, +0.0);
    UV[2] = float2(+1.0, +1.0);
    UV[3] = float2(+1.0, +0.0);

    PSIn.Pos = Pos[VSIn.VertexID];
    PSIn.UV  = UV[VSIn.VertexID];
}
)";
