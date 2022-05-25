//
//  Editor.hpp
//  MyProject
//
//  Created by Dmitry on 21.05.2022.
//

#pragma once

#include <Mesh.h>
#include <Diligent.h>
#include <glm/glm.hpp>

typedef Diligent::RefCntAutoPtr<Diligent::IRenderDevice> DgRenderDevice;
typedef Diligent::RefCntAutoPtr<Diligent::IPipelineState> DgPipelineState;
typedef Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> DgShaderResourceBinding;
typedef Diligent::RefCntAutoPtr<Diligent::IDeviceContext> DgDeviceContext;
typedef Diligent::RefCntAutoPtr<Diligent::ISwapChain> DgSwapChain;
typedef Diligent::RefCntAutoPtr<Diligent::IBuffer> DgBuffer;
typedef Diligent::RefCntAutoPtr<Diligent::ITexture> DgTexture;
typedef Diligent::RefCntAutoPtr<Diligent::ITextureView> DgTextureView;

struct Texture {
    Texture(DgRenderDevice renderDevice,
            Diligent::TEXTURE_FORMAT colorBufferFormat,
            void* imageData,
            int width,
            int height,
            int numChannels);
    
    Texture();
    
    DgTexture texture;
    DgTextureView textureSRV;
};

struct Ray {
    glm::vec3 origin, direction;
};

Ray screenPointToRay(glm::vec2 pos, glm::vec2 screenDims, glm::mat4 viewProj);

struct RenderVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 UV;
    glm::vec4 color;
};

struct RenderTriange {
    int a, b, c;
};

struct RenderLine {
    int a, b;
};

struct EdgeSelectionVertex {
    glm::vec3 pos;
    PolyMesh::EdgeHandle eh;
};

class Model {
    void populateRenderBuffers(DgRenderDevice renderDevice, DgDeviceContext context);
    
    int lastNumVerts = 0;
    int lastNumTris = 0;
    int lastNumWireframeVerts = 0;
    
public:
    Model();
    
    PolyMesh originalMesh;
    PolyMesh renderMesh;
    
    std::vector<EdgeSelectionVertex> selectionWireframeVerts;
    std::vector<RenderTriange> selectionWireframeTris;
    
    std::unordered_map<PolyMesh::VertexHandle, PolyMesh::VertexHandle> originalToRenderVerts;
    
    bool isFlatShaded = true;
    
    DgBuffer vertexBuffer, triangleBuffer, wireframeTriangleBuffer, wireframeVertexBuffer;
    int numTrisIndices = 0;
    int numLinesIndices = 0;
    
    void invalidate(DgRenderDevice renderDevice, DgDeviceContext context);
};

Model createCubeModel();

struct RendererObjects {
    DgBuffer VSConsts;
    DgBuffer PSConsts;
    DgPipelineState PSO;
    DgShaderResourceBinding SRB;
};

struct RendererCreateOptions {
    int sampleCount = 1;
};

class ModelRenderer {
public:
    ModelRenderer(DgRenderDevice renderDevice, DgSwapChain swapChain, Texture& matcap,
                  RendererCreateOptions options);

    void draw(
        DgDeviceContext context,
        glm::mat4 modelViewProj, glm::mat4 modelView, Model& model);

private:
    RendererObjects surface;
    RendererObjects wireframe;
};

class Editor {
public:
    Editor(DgRenderDevice renderDevice, DgSwapChain swapChain, Texture& matcap,
           RendererCreateOptions options);
    
    glm::mat4 viewProj;
    glm::mat4 view;
    glm::vec3 eye;
    
    Texture matcap;
    DgRenderDevice renderDevice;
    ModelRenderer renderer;
    
    bool isShiftPressed = false;
    
    void raycastEdges(glm::vec2 mouse, glm::vec2 screenDims,
                      DgRenderDevice renderDevice, DgDeviceContext context);
    
    Model* model = nullptr;
    
    void input(bool isMouseDown, float mouseX, float mouseY);
    
    void draw(DgDeviceContext context);
};

struct RendererPSConstants {
    glm::mat4 modelView;
};

struct RendererVSConstants {
    glm::mat4 MVP;
    glm::mat3 normal;
    int32_t padding[4];
};

static const char* RendererWireframePSSource = R"(
//cbuffer Constants
//{
//    float4x4 g_ModelView;
//};

struct PSInput
{
    float4 Pos      : SV_POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEX_COORD;
    float4 Color    : COLOR0;
};
struct PSOutput
{
    float4 Color : SV_TARGET;
};

void main(in  PSInput  PSIn,
          out PSOutput PSOut)
{
    PSOut.Color = PSIn.Color;
}
)";

static const char* RendererPSSource = R"(
cbuffer Constants
{
    float4x4 g_ModelView;
};

Texture2D    g_Texture;
SamplerState g_Texture_sampler;

struct PSInput
{
    float4 Pos      : SV_POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEX_COORD;
    float4 Color    : COLOR0;
};
struct PSOutput
{
    float4 Color : SV_TARGET;
};

// https://github.com/hughsk/matcap/blob/master/matcap.glsl
float2 matcap(float3 eye, float3 normal) {
    float3 reflected = reflect(eye, normal);
    float m = 2.8284271247461903 * sqrt(reflected.z + 1.0);
    return reflected.xy / m + 0.5;
}

float2 matcap2(float3 normal) {
    float3 viewNormal = float3(mul(float4(normalize(normal), 1.0), g_ModelView));
    // float m = 2.8284271247461903 * sqrt(viewNormal.z + 1.0);
    float m = 2.0 * sqrt(
        pow(viewNormal.x, 2.0) +
        pow(viewNormal.y, 2.0) +
        pow(viewNormal.z + 1.0, 2.0)
   );
    return viewNormal.xy / 2.8284271247461903 + 0.5;
}

void main(in  PSInput  PSIn,
          out PSOutput PSOut)
{
    float3 no = PSIn.Normal / 2.0 + 0.5;
    float3 eye = normalize(float3(mul(PSIn.Pos, g_ModelView)));
    float3 base = g_Texture.Sample(g_Texture_sampler, matcap2(PSIn.Normal)).rgb;
    float4 col = PSIn.Color;
    if(col.g < 0.005) {
        col = float4(1.0, 0.5, 0.0, 1.0);
    } else {
        col = float4(1.0, 1.0, 1.0, 1.0);
    }
    PSOut.Color = float4(base, 1.0) * col;
    // PSOut.Color = float4(-float3(mul(float4(normalize(-PSIn.Normal), 1.0), g_ModelView)), 1.0);
    // PSOut.Color = g_Texture.Sample(g_Texture_sampler, float2(PSIn.Pos.x / 1280.0, PSIn.Pos.y / 720.0));
    // float4(no.x, no.y, no.z, 1.0f);
}
)";

static const char* RendererVSSource = R"(
cbuffer Constants
{
    float4x4 g_ModelViewProj;
    float3x3 g_Normal;
};

struct VSInput
{
    float3 Pos      : ATTRIB0;
    float3 Normal   : ATTRIB1;
    float2 TexCoord : ATTRIB2;
    float4 Color    : ATTRIB3;
};

struct PSInput
{
    float4 Pos      : SV_POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEX_COORD;
    float4 Color    : COLOR0;
};

void main(in  VSInput VSIn,
          out PSInput PSIn)
{
    PSIn.Pos = mul(float4(VSIn.Pos, 1.0), g_ModelViewProj);
    PSIn.TexCoord = VSIn.TexCoord;
    PSIn.Normal = VSIn.Normal;//mul(VSIn.Normal, g_Normal);
    PSIn.Color = VSIn.Color;
}
)";

static const char* RendererWireframeVSSource = R"(
cbuffer Constants
{
    float4x4 g_ModelViewProj;
    float3x3 g_Normal;
};

struct VSInput
{
    float3 Pos      : ATTRIB0;
    float3 Normal   : ATTRIB1;
    float2 TexCoord : ATTRIB2;
    float4 Color    : ATTRIB3;
};

struct PSInput
{
    float4 Pos      : SV_POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEX_COORD;
    float4 Color    : COLOR0;
};

void main(in  VSInput VSIn,
          out PSInput PSIn)
{
    PSIn.Pos = mul(float4(VSIn.Pos, 1.0), g_ModelViewProj);
//    PSIn.Pos += float4(PSIn.Normal * 0.1, 0.0f);
    PSIn.TexCoord = VSIn.TexCoord;
    PSIn.Normal = VSIn.Normal;
    PSIn.Color = VSIn.Color;
}
)";
