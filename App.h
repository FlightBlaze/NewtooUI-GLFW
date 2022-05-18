#pragma once

#include <MyProject.h>
#include <Diligent.h>
#include <RenderTarget.h>
#include <TextRenderer.h>
#include <Shape.h>
#include <Elements.h>
#include <Context.h>
#include <HalfEdge.h>
#include <Mesh.h>
// #include <GLFW/glfw3.h>
#include <SDL.h>
#include <glm/glm.hpp>
#include <Spring.hh>
#include <Font.hh>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <backends/diligent.hh>
#include <blazegizmo.hh>

struct Vertex {
    glm::vec3 pos;
    glm::vec4 color;
};

struct MotionBlurConstants {
    glm::vec2 direction;
    glm::vec2 resolution;
};

enum class DemoType {
    VECTOR_GRAPHICS,
    GIZMOS,
    UI,
    HALF_EDGE
};

class App {
public:
    int run();
    void draw();
    void update();
    void resize(int width, int height);

    App();
    virtual ~App();
    
    // static void errorCallback(int error, const char* description);
    // static void resizeCallback(GLFWwindow* window, int width, int height);
    // static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

private:
    void initializeDiligentEngine();
    void initializeResources();
    void initializeFont();

    float getTime();

    float mPreviousTime = 0.0f;
    float mCurrentTime = 0.0f;
    float mDeltaTime = 0.0f;

    // GLFWwindow* mWindow = nullptr;
    SDL_Window* mWindow = nullptr;
    int mWidth = 0,
        mHeight = 0;
    
    float mScale = 1.0f;

    #ifdef PLATFORM_MACOS
    Diligent::RENDER_DEVICE_TYPE mDeviceType = Diligent::RENDER_DEVICE_TYPE_VULKAN;
    #else
    Diligent::RENDER_DEVICE_TYPE mDeviceType = Diligent::RENDER_DEVICE_TYPE_D3D11;
    #endif
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  mDevice;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> mImmediateContext;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> mSwapChain;
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> mPSO;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> mVSConstants;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> mPSConstants;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> mQuadVertexBuffer;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> mQuadIndexBuffer;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> mSRB;
    glm::mat4 mViewProjection;
    glm::mat4  mModelViewProjection = glm::mat4(1.0f);
    glm::mat4  mTextModelViewProjection = glm::mat4(1.0f);
    float mRotation = 0.0f;
    float mTextSize = 16.0f;

    Diligent::RefCntAutoPtr<Diligent::IBuffer> mMotionBlurConstants;
    Diligent::RefCntAutoPtr<Diligent::IShader> mMotionBlurPS;
    Diligent::RefCntAutoPtr<Diligent::IShader> mPrintPS;

    ui::SpringPhysicalProperties mQuadPhysicalProps;
    ui::Spring mQuadPosX;
//    std::shared_ptr<ui::Font> mFont;
//    std::shared_ptr<TextRenderer> mTextRenderer;
//    std::shared_ptr<ShapeRenderer> mShapeRenderer;

    float mQuadTime = 0.0f;
    float mSecondTime = 0.0f;
    int mLastFPS = 0;
    int mFPS = 0;
    int mLastRaycasts = 0;
    
    RenderTarget renderTarget;
    
    void recreateRenderTargets();
    
    float mPitch = 45.0f;
    float mYaw = 45.0f;
    
    float mZoom = 15.0f;
    
    float mMouseX = 0.0f,
        mMouseY = 0.0f;
    
    bool mIsControlPressed = false;
    
    glm::mat4 mModel = glm::mat4(1.0f);
    gizmo::Transform mTransform;
    gizmo::GizmoState mGizmoState;
    gizmo::GizmoTool mGizmoTool = gizmo::GizmoTool::Translate;
    gizmo::GizmoProperties mGizmoProps;
    bool mPlayRotation = false;
    
    DemoType mDemoType = DemoType::VECTOR_GRAPHICS;
    
public:
    glm::vec2 mSquirclePos;
    Shape mSquircleFill;
    Shape mSquircleStroke;
    
    Shape mRayStroke;
    bool isMouseDown = false;
    
    std::shared_ptr<Shape> mSquircleShape;
    
    Context ctx;
    
    bvg::DiligentContext bvgCtx;
    
//    HE::Mesh mesh;
//    HE::MeshViewer meshViewer;
    PolyMesh mesh;
    MeshViewer meshViewer;
    std::wstring heSummary;
    
    void doUI();
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
cbuffer Constants
{
    float g_Opacity;
};

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
    PSOut.Color = float4(PSIn.Color.rgb, g_Opacity);
}
)";

static const char* RenderTargetPSSource = R"(
/* Gaussian blur with 1D kernel */

cbuffer Constants
{
    float2 g_Direction;
    float2 g_Resolution;
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

    float2 pixelOffset = float2(1.33333, 1.33333) * g_Direction;
    float2 normalizedOffset = pixelOffset / g_Resolution;

    /* These magic numbers are calculated using http://dev.theomader.com/gaussian-kernel-calculator */

    PSOut.Color = g_Texture.Sample(g_Texture_sampler, UV) * 0.29411764705882354 +
        g_Texture.Sample(g_Texture_sampler, UV + normalizedOffset) * 0.35294117647058826 +
        g_Texture.Sample(g_Texture_sampler, UV - normalizedOffset) * 0.35294117647058826;
}
)";

static const char* PrintPSSource = R"(
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

    PSOut.Color = g_Texture.Sample(g_Texture_sampler, UV);
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

static std::vector<float> GaussianKernel31x1 =
{
    0.1479298670997733f,
    0.15396059101965992f,
    0.15981088575094535f,
    0.16544217619865625f,
    0.17081625499596645f,
    0.17589570887009753f,
    0.18064434869980706f,
    0.18502763641967188f,
    0.18901310177328587f,
    0.1925707419106246f,
    0.19567339696848868f,
    0.1982970950675025f,
    0.20042136060094926f,
    0.2020294802720795f,
    0.20310872204587352f,
    0.20365050300338722f,
    0.20365050300338722f,
    0.20310872204587355f,
    0.2020294802720795f,
    0.20042136060094928f,
    0.19829709506750254f,
    0.1956733969684887f,
    0.19257074191062462f,
    0.18901310177328592f,
    0.18502763641967193f,
    0.18064434869980714f,
    0.1758957088700976f,
    0.17081625499596653f,
    0.16544217619865634f,
    0.1598108857509454f,
    0.15396059101966f,
    0.14792986709977338f
};

static std::vector<float> GaussianKernel15x1 =
{
    0.19306470526010783f,
    0.22045166276214131f,
    0.24663841379876808f,
    0.27036153640026195f,
    0.2903794913659932f,
    0.30557922275140015f,
    0.31507834163275994f,
    0.3183098861837907f,
    0.31507834163276f,
    0.3055792227514002f,
    0.2903794913659933f,
    0.270361536400262f,
    0.24663841379876816f,
    0.22045166276214143f,
    0.1930647052601079f
};

static std::vector<float> GaussianKernel9x1 =
{
    0.23264196693351172f,
    0.3305198208945779f,
    0.43011005402760094f,
    0.5126657667016575f,
    0.5597082137916062f,
    0.5126657667016575f,
    0.43011005402760094f,
    0.33051982089457793f,
    0.23264196693351175f
};

static std::vector<float> GaussianKernel5x1 =
{
    0.17231423441478907f,
    0.6197522207772878f,
    1.1753482366109071f,
    0.6197522207772876f,
    0.17231423441478907f
};
