#include <App.h>
#include <iostream>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h>

App* findAppByWindow(GLFWwindow* window) {
	return static_cast<App*>(glfwGetWindowUserPointer(window));
}

void App::errorCallback(int error, const char* description)
{
	std::cerr << "Error: " << description << std::endl;
}

void App::resizeCallback(GLFWwindow* window, int width, int height) {
	auto* self = findAppByWindow(window);
	self->mSwapChain->Resize(width, height);
}

int App::run()
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	glfwSetErrorCallback(errorCallback);

	mWindow = glfwCreateWindow(640, 480, "My Application", NULL, NULL);
	if (!mWindow)
	{
		std::cerr << "Failed to create window" << std::endl;
		return -1;
	}

	glfwSetWindowUserPointer(mWindow, this);
	glfwSetFramebufferSizeCallback(mWindow, &resizeCallback);

	initializeDiligentEngine();
	initializeResources();

	while (!glfwWindowShouldClose(mWindow))
	{
		glfwPollEvents();
		update();
		draw();
	}

	glfwDestroyWindow(mWindow);
	glfwTerminate();
	return 0;
}

void App::draw()
{
	const float ClearColor[] = { 0.350f,  0.350f,  0.350f, 1.0f };
	auto* pRTV = mSwapChain->GetCurrentBackBufferRTV();
	auto* pDSV = mSwapChain->GetDepthBufferDSV();
	mImmediateContext->SetRenderTargets(1, &pRTV, pDSV,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	mImmediateContext->ClearRenderTarget(pRTV, ClearColor,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	mImmediateContext->ClearDepthStencil(pDSV, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	mImmediateContext->SetPipelineState(mPSO);

	Diligent::DrawAttribs drawAttrs;
	drawAttrs.NumVertices = 3;
	mImmediateContext->Draw(drawAttrs);

	mSwapChain->Present();
}

void App::update()
{
	mCurrentTime = getTime();
	mDeltaTime = mCurrentTime - mPreviousTime;
	mPreviousTime = mCurrentTime;
}

void App::resize(int width, int height)
{
}

void App::initializeDiligentEngine()
{
	Diligent::Win32NativeWindow window{ glfwGetWin32Window(mWindow) };
	Diligent::SwapChainDesc SCDesc;
	{
		auto* pFactoryOpenGL = Diligent::GetEngineFactoryOpenGL();

		Diligent::EngineGLCreateInfo EngineCI;
		EngineCI.Window = window;

		pFactoryOpenGL->CreateDeviceAndSwapChainGL(
			EngineCI,
			&mDevice,
			&mImmediateContext,
			SCDesc,
			&mSwapChain);
	}
}

void App::initializeResources()
{
	Diligent::GraphicsPipelineStateCreateInfo PSOCreateInfo;
	PSOCreateInfo.PSODesc.Name = "Simple triangle PSO";
	PSOCreateInfo.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
	PSOCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
	PSOCreateInfo.GraphicsPipeline.RTVFormats[0] = mSwapChain->GetDesc().ColorBufferFormat;
	PSOCreateInfo.GraphicsPipeline.DSVFormat = mSwapChain->GetDesc().DepthBufferFormat;
	PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
	PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = Diligent::False;

	Diligent::ShaderCreateInfo ShaderCI;
	ShaderCI.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
	ShaderCI.UseCombinedTextureSamplers = Diligent::True;

	// Create a vertex shader
	Diligent::RefCntAutoPtr<Diligent::IShader> pVS;
	{
		ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
		ShaderCI.EntryPoint = "main";
		ShaderCI.Desc.Name = "Triangle vertex shader";
		ShaderCI.Source = VSSource;
		mDevice->CreateShader(ShaderCI, &pVS);
	}

	// Create a pixel shader
	Diligent::RefCntAutoPtr<Diligent::IShader> pPS;
	{
		ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
		ShaderCI.EntryPoint = "main";
		ShaderCI.Desc.Name = "Triangle pixel shader";
		ShaderCI.Source = PSSource;
		mDevice->CreateShader(ShaderCI, &pPS);
	}

	PSOCreateInfo.pVS = pVS;
	PSOCreateInfo.pPS = pPS;
	mDevice->CreateGraphicsPipelineState(PSOCreateInfo, &mPSO);
}

float App::getTime()
{
	return (float)glfwGetTime();
}
