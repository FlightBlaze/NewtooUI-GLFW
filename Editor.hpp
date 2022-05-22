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

class Model {
    void populateRenderBuffers(DgRenderDevice renderDevice);
    
public:
    Model();
    
    PolyMesh originalMesh;
    PolyMesh renderMesh;
    
    std::unordered_map<PolyMesh::VertexHandle, PolyMesh::VertexHandle> originalToRenderVerts;
    
    bool isFlatShaded = true;
    
    DgBuffer vertexBuffer, triangleBuffer, lineBuffer;
    int numTrisIndices = 0;
    int numLinesIndices = 0;
    
    void invalidate(DgRenderDevice renderDevice);
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
    PSOut.Color = float4(0.0f, 0.0f, 0.0f, 1.0f);
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

void main(in  PSInput  PSIn,
          out PSOutput PSOut)
{
    float3 no = PSIn.Normal / 2.0 + 0.5;
    float3 eye = normalize(float3(mul(PSIn.Pos, g_ModelView)));
    PSOut.Color = g_Texture.Sample(g_Texture_sampler, matcap(eye, PSIn.Normal));
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
    float2 TexCoord : ATTRIB3;
    float4 Color    : ATTRIB2;
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
    PSIn.Normal = VSIn.Normal;
    PSIn.Color = VSIn.Color;
}
)";
