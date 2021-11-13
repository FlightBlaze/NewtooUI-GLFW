#pragma once

#include <Diligent.h>
#include <MathExtras.h>
#include <glm/glm.hpp>
#include <Path.h>

struct MinMax2 {
    float minX, maxX, minY, maxY;
};

struct ShapeVertex {
    glm::vec3 position;
    glm::vec2 texCoord;
};

class Shape {
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
    float intersectsRay(glm::vec2 rayOrigin, glm::vec2 rayDirection);
};

Shape CreateFill(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice,
    tube::Path path, bool isConvex);

Shape CreateStroke(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice,
    tube::Builder& builder);

class ShapeRenderer {
public:
	ShapeRenderer(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice,
		Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapChain);

	void draw(
		Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context,
		glm::mat4 modelViewProjection,
		Shape& shape, glm::vec3 color, float opacity);

private:
	Diligent::RefCntAutoPtr<Diligent::IBuffer> mVSConstants;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> mPSConstants;
	Diligent::RefCntAutoPtr<Diligent::IPipelineState> mPSO;
	Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> mSRB;
};

struct ShapePSConstants {
    glm::vec3 color;
    float opacity;
};

static const char* ShapePSSource = R"(
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

struct TwoPolylines {
    std::vector<glm::vec2> first, second;
};

struct TriangeIndices {
    int a, b, c;
};

struct ShapeMesh {
    std::vector<glm::vec2> vertices;
    std::vector<TriangeIndices> indices;
    
    void add(ShapeMesh& b);
};

ShapeMesh strokePolyline(std::vector<glm::vec2>& points, const float diameter);
ShapeMesh bevelJoin(std::vector<glm::vec2>& a, std::vector<glm::vec2>& b, const float diameter);
ShapeMesh roundJoin(std::vector<glm::vec2>& a, std::vector<glm::vec2>& b, const float diameter);
ShapeMesh miterJoin(std::vector<glm::vec2>& a, std::vector<glm::vec2>& b, const float diameter,
                    float miterLimitAngle = M_PI_2 + M_PI_4);
TwoPolylines dividePolyline(std::vector<glm::vec2>& points, float t);
std::vector<std::vector<glm::vec2>> dashedPolyline(std::vector<glm::vec2>& points, float dashLength, float gapLength);

Shape CreateShapeFromMesh(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice,
                          ShapeMesh& mesh);
