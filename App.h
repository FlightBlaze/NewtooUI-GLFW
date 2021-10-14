#pragma once

#include <GLFW/glfw3.h>

#define PLATFORM_WIN32 1
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Common/interface/RefCntAutoPtr.hpp"

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec4 color;
};

class App {
public:
    int run();
    void draw();
    void update();
    void resize(int width, int height);

    static void errorCallback(int error, const char* description);
    static void resizeCallback(GLFWwindow* window, int width, int height);

private:
    void initializeDiligentEngine();
    void initializeResources();

    float getTime();

    float mPreviousTime;
    float mCurrentTime;
    float mDeltaTime;

    GLFWwindow* mWindow;
    int mWidth, mHeight;

    Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  mDevice;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> mImmediateContext;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> mSwapChain;
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> mPSO;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> mVSConstants;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> mQuadVertexBuffer;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> mQuadIndexBuffer;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> mSRB;
    glm::mat4  mModelViewProjection;
    float mRotation;
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

static Vertex QuadVerts[8] =
{
        {glm::vec3(-100,-100,0), glm::vec4(1,0,0,1)},
        {glm::vec3(-100,+100,0), glm::vec4(0,1,0,1)},
        {glm::vec3(+100,+100,0), glm::vec4(0,0,1,1)},
        {glm::vec3(+100,-100,0), glm::vec4(1,1,0,1)}
};

static Diligent::Uint32 QuadIndices[] =
{
        0,1,2, 2,3,0
};
