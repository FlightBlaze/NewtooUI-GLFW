#pragma once

#include <MyProject.h>
#include <Diligent.h>
#include <RenderTarget.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <Spring.hh>
#include <memory>

// Linear interpolation
float lerpf(float a, float b, float t);

struct Vertex {
    glm::vec3 pos;
    glm::vec4 color;
};

struct CurrentTargetF {
    float current, target;
};

class App {
public:
    int run();
    void draw();
    void update();
    void resize(int width, int height);

    App();

    static void errorCallback(int error, const char* description);
    static void resizeCallback(GLFWwindow* window, int width, int height);

private:
    void initializeDiligentEngine();
    void initializeResources();

    float getTime();

    float mPreviousTime = 0.0f;
    float mCurrentTime = 0.0f;
    float mDeltaTime = 0.0f;

    GLFWwindow* mWindow = nullptr;
    int mWidth = 0,
        mHeight = 0;

    Diligent::RENDER_DEVICE_TYPE mDeviceType = Diligent::RENDER_DEVICE_TYPE_D3D11;
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  mDevice;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> mImmediateContext;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> mSwapChain;
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> mPSO;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> mVSConstants;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> mQuadVertexBuffer;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> mQuadIndexBuffer;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> mSRB;
    glm::mat4  mModelViewProjection = glm::mat4(1.0f);
    float mRotation = 0.0f;

    std::shared_ptr<RenderTarget> mRenderTarget;

    ui::SpringPhysicalProperties mQuadPhysicalProps;
    ui::Spring mQuadPosX;

    float mQuadTime = 0.0f;
    float mSecondTime = 0.0f;
    int mFPS = 0;
};

static const char* VSSource = R"(
cbuffer Constants
{
    float4x4 g_ModelViewProj;
};

struct VSInput
{
    float3 Pos   : ATTRIB0;
    float4 Color : ATTRIB1;
};

struct PSInput
{
    float4 Pos   : SV_POSITION;
    float4 Color : COLOR0;
};

void main(in  VSInput VSIn,
          out PSInput PSIn)
{
    PSIn.Pos   = mul( float4(VSIn.Pos,1.0), g_ModelViewProj);
    PSIn.Color = VSIn.Color;
}
)";

static const char* PSSource = R"(
struct PSInput
{
    float4 Pos   : SV_POSITION;
    float4 Color : COLOR0;
};
struct PSOutput
{
    float4 Color : SV_TARGET;
};
void main(in  PSInput  PSIn,
          out PSOutput PSOut)
{
    PSOut.Color = float4(PSIn.Color.rgb, 1.0);
}
)";

static const char* RenderTargetPSSource = R"(
cbuffer Constants
{
    float g_Time;
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

void main(in  PSInput  PSIn,
          out PSOutput PSOut)
{
#if defined(DESKTOP_GL) || defined(GL_ES)
    float2 UV = float2(PSIn.UV.x, 1.0 - PSIn.UV.y);
#else
    float2 UV = PSIn.UV;
#endif

    float2 DistortedUV = UV + float2(sin(UV.y*10.0)*0.1  * sin(g_Time.x*3.0),
                                     sin(UV.x*10.0)*0.02 * sin(g_Time.x*2.0));
    PSOut.Color = g_Texture.Sample(g_Texture_sampler, DistortedUV);
}
)";

static Vertex QuadVerts[8] =
{
        {glm::vec3(-50,-50,0), glm::vec4(1.0f,0.1f,0.1f,1)},
        {glm::vec3(-50,+50,0), glm::vec4(0.1f,0.1f,1.0f,1)},
        {glm::vec3(+50,+50,0), glm::vec4(0.1f,0.1f,1.0f,1)},
        {glm::vec3(+50,-50,0), glm::vec4(1.0f,0.1f,0.1f,1)}
};

static Diligent::Uint32 QuadIndices[] =
{
        0,1,2, 2,3,0
};
