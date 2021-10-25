#pragma once

#include <Diligent.h>
#include <glm/glm.hpp>
#include <Path.h>

struct MinMax2 {
    float minX, maxX, minY, maxY;
};

struct ShapeVertex {
    glm::vec3 position;
    glm::vec2 texCoord;
};

class Fill {
public:
	Diligent::RefCntAutoPtr<Diligent::IBuffer> vertexBuffer;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> indexBuffer;
    int numIndices = 0;
    float width = 0;
    float height = 0;
    glm::vec2 offset;
    std::vector<ShapeVertex> vertices;
    std::vector<int> indices;
    MinMax2 bounds;

    bool containsPoint(glm::vec2 point);
};

Fill CreateFill(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice,
    tube::Path path, bool isConvex);

class SolidFillRenderer {
public:
	SolidFillRenderer(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice,
		Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapChain);

	void draw(
		Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context,
		glm::mat4 modelViewProjection,
		Fill& shape, glm::vec3 color, float opacity);

private:
	Diligent::RefCntAutoPtr<Diligent::IBuffer> mVSConstants;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> mPSConstants;
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> mPSO;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> mSRB;
};

struct SolidFillPSConstants {
    glm::vec3 color;
    float opacity;
};

static const char* SolidFillPSSource = R"(
cbuffer Constants
{
    float3 g_Color;
    float g_Opacity;
};

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float2 UV    : TEX_COORD;
};
struct PSOutput
{
    float4 Color : SV_TARGET;
};

void main(in  PSInput  PSIn,
          out PSOutput PSOut)
{
    PSOut.Color = float4(g_Color, g_Opacity);
}
)";

static const char* ShapeVSSource = R"(
cbuffer Constants
{
    float4x4 g_ModelViewProj;
};

struct VSInput
{
    float3 Pos   : ATTRIB0;
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
    PSIn.Pos = mul(float4(VSIn.Pos, 1.0), g_ModelViewProj);
    PSIn.UV = VSIn.TexCoord;
}
)";