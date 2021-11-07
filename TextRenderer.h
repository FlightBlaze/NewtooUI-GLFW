#pragma once

#include <glm/glm.hpp>
#include <Diligent.h>
#include <Font.hh>

class TextRenderer {
public:
    const std::shared_ptr<ui::Font> font;

    TextRenderer(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice,
        Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapChain,
        std::shared_ptr<ui::Font> font);

    void draw(
        Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context,
        glm::mat4 modelViewProjection,
        const std::wstring& text,
        float sizePx = 32.0f,
        glm::vec3 color = glm::vec3(0.0f),
        float opacity = 1.0f);
    
    float measureWidth(const std::wstring& text, float sizePx = 32.0f);
    float measureHeight(float sizePx = 32.0f);
    float getScale(float sizePx);

protected:
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> mPSO;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> mVSConstants;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> mPSConstants;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> mQuadIndexBuffer;
    std::vector<Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding>> mPageSRBs;
};

static Diligent::Uint32 GlyphQuadIndices[] =
{
        0,1,2, 2,3,0
};

static const char* GlyphVSSource = R"(
cbuffer Constants
{
    float4x4 g_ModelViewProj;
};

struct VSInput
{
    float2 Pos   : ATTRIB0;
    float2 TexCoord : ATTRIB1;
};

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float2 UV    : TEX_COORD;
};

void main(in  VSInput VSIn,
          out PSInput PSIn)
{
    PSIn.Pos = mul(float4(VSIn.Pos, 0.0, 1.0), g_ModelViewProj);
    PSIn.UV = VSIn.TexCoord;
}
)";

struct GlyphPSConstants {
    glm::vec3 color;
    float opacity;
    float distanceRange;
    glm::vec2 atlasSize;
};

static const char* GlyphPSSource = R"(
cbuffer Constants
{
    float3 g_Color;
    float g_Opacity;
    float g_DistanceRange;
    float2 g_AtlasSize;
};

Texture2D    g_Texture;
SamplerState g_Texture_sampler;

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float2 UV    : TEX_COORD;
};
struct PSOutput
{
    float4 Color : SV_TARGET;
};

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main(in  PSInput  PSIn,
          out PSOutput PSOut)
{
#if defined(DESKTOP_GL) || defined(GL_ES)
    float2 UV = float2(PSIn.UV.x, 1.0 - PSIn.UV.y);
#else
    float2 UV = PSIn.UV;
#endif
    float2 dxdy = fwidth(PSIn.UV) * g_AtlasSize;
    float4 MSD = g_Texture.Sample(g_Texture_sampler, UV);
    float SDF = median(MSD.r, MSD.g, MSD.b);
    float Opacity = clamp(SDF * g_DistanceRange/* / length(dxdy)*/, 0.0, 1.0);
    if(Opacity < 0.5)
        discard;
    PSOut.Color = float4(g_Color, g_Opacity * Opacity);
}
)";
