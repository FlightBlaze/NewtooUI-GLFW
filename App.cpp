#include <App.h>
#include <iostream>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h"
#include "Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h"
#include "Graphics/GraphicsTools/interface/MapHelper.hpp"
#include <glm/gtx/transform.hpp>

float lerpf(float a, float b, float t)
{
	return a + t * (b - a);
}

App* findAppByWindow(GLFWwindow* window)
{
	return static_cast<App*>(glfwGetWindowUserPointer(window));
}

App::App():
	mQuadPhysicalProps(1.0f, 35.0f, 5.0f),
	mQuadPosX(-100.0f, 100.0f, mQuadPhysicalProps)
{
}

void App::errorCallback(int error, const char* description)
{
	std::cerr << "Error: " << description << std::endl;
}

void App::resizeCallback(GLFWwindow* window, int width, int height)
{
	auto* self = findAppByWindow(window);
	self->resize(width, height);
	self->mWidth = width;
	self->mHeight = height;
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
	glfwGetWindowSize(mWindow, &mWidth, &mHeight);

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
	// Write model view projection matrix to uniform
	{
		Diligent::MapHelper<glm::mat4> CBConstants(mImmediateContext, mVSConstants,
			Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
		*CBConstants = glm::transpose(mModelViewProjection);
	}
	{
		Diligent::MapHelper<float> CBConstants(mImmediateContext, mRenderTarget->uniformBuffer,
			Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
		*CBConstants = getTime();
	}

	const float ClearColor[] = { 0.75f,  0.75f,  0.75f, 1.0f };

	mRenderTarget->use(mImmediateContext);
	mRenderTarget->clear(mImmediateContext, ClearColor);

	Diligent::Uint64   offset = 0;
	Diligent::IBuffer* pBuffs[] = { mQuadVertexBuffer };
	mImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
	mImmediateContext->SetIndexBuffer(mQuadIndexBuffer, 0,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	mImmediateContext->SetPipelineState(mPSO);

	mImmediateContext->CommitShaderResources(mSRB, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::DrawIndexedAttribs DrawAttrs;
	DrawAttrs.IndexType = Diligent::VT_UINT32;
	DrawAttrs.NumIndices = _countof(QuadIndices);
	DrawAttrs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	mImmediateContext->DrawIndexed(DrawAttrs);

	const float Black[] = { 0.0f,  0.0f,  0.0f, 1.0f };

	auto* pRTV = mSwapChain->GetCurrentBackBufferRTV();
	auto* pDSV = mSwapChain->GetDepthBufferDSV();

	mImmediateContext->SetRenderTargets(1, &pRTV, pDSV,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	mImmediateContext->ClearRenderTarget(pRTV, Black,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	mImmediateContext->ClearDepthStencil(pDSV, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	mRenderTarget->draw(mImmediateContext);

	mSwapChain->Present();
}

void App::update()
{
	mCurrentTime = getTime();
	mDeltaTime = mCurrentTime - mPreviousTime;
	mPreviousTime = mCurrentTime;

	mDeltaTime = fminf(mDeltaTime, 0.02f);

	mQuadTime += mDeltaTime;
	mSecondTime += mDeltaTime;
	mFPS++;

	if (mSecondTime >= 1.0f) {
		mSecondTime = 0.0f;
		std::cout << "FPS: " << mFPS << std::endl;
		mFPS = 0;
	}

	if (mQuadTime >= 2.0f) {
		mQuadTime = 0.0f;
		mQuadPosX.targetValue = -mQuadPosX.targetValue;
	}

	mQuadPosX.update(mDeltaTime);

	glm::mat4 view = glm::identity<glm::mat4>();
	glm::mat4 projection = glm::ortho(0.0f, (float)mWidth, (float)mHeight, 0.0f); // glm::perspective(65.0f, (float)mWidth / (float)mHeight, 0.001f, 100.0f);
	glm::mat4 model = glm::rotate(glm::translate(glm::vec3(mWidth / 2 + mQuadPosX.currentValue, mHeight / 2, 0.0f)), mRotation, glm::vec3(0, 0, 1));

	mModelViewProjection = projection * view * model;
}

void App::resize(int width, int height)
{
	mSwapChain->Resize(width, height);

	RenderTargetCreateInfo renderTargetCI;
	renderTargetCI.device = mDevice;
	renderTargetCI.width = width;
	renderTargetCI.height = height;
	mRenderTarget->recreateTextures(renderTargetCI);
}

void App::initializeDiligentEngine()
{
	Diligent::Win32NativeWindow window{ glfwGetWin32Window(mWindow) };
	Diligent::SwapChainDesc SCDesc;
	switch (mDeviceType)
	{
		case Diligent::RENDER_DEVICE_TYPE_GLES:
			{
#    if ENGINE_DLL
				auto* GetEngineFactoryOpenGL = Diligent::LoadGraphicsEngineOpenGL();
				auto* pFactoryOpenGL = GetEngineFactoryOpenGL();
#		else
				auto* pFactoryOpenGL = Diligent::GetEngineFactoryOpenGL();
#		endif

				Diligent::EngineGLCreateInfo EngineCI;
				EngineCI.Window = window;

				ModifyEngineInitInfo({mDeviceType, pFactoryOpenGL, EngineCI, SCDesc});

				pFactoryOpenGL->CreateDeviceAndSwapChainGL(
					EngineCI,
					&mDevice,
					&mImmediateContext,
					SCDesc,
					&mSwapChain);
			}
			break;
		case Diligent::RENDER_DEVICE_TYPE_D3D11:
			{
#		if ENGINE_DLL
				auto* GetEngineFactoryD3D11 = Diligent::LoadGraphicsEngineD3D11();
				auto* pFactoryD3D11 = GetEngineFactoryD3D11();
#		else
				auto* pFactoryD3D11 = Diligent::GetEngineFactoryD3D11();
#		endif

				Diligent::EngineD3D11CreateInfo EngineCI;

				ModifyEngineInitInfo({ mDeviceType, pFactoryD3D11, EngineCI, SCDesc });

				pFactoryD3D11->CreateDeviceAndContextsD3D11(EngineCI, &mDevice, &mImmediateContext);
				pFactoryD3D11->CreateSwapChainD3D11(mDevice, mImmediateContext, SCDesc, Diligent::FullScreenModeDesc{}, window, &mSwapChain);
			}
			break;
		default:
			std::cerr << "Unsupported render device type" << std::endl;
			exit(-1);
			break;
	}
}

void App::initializeResources()
{
	Diligent::GraphicsPipelineStateCreateInfo PSOCreateInfo;
	PSOCreateInfo.PSODesc.Name = "Simple triangle PSO";
	PSOCreateInfo.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
	PSOCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
	PSOCreateInfo.GraphicsPipeline.RTVFormats[0] = RenderTarget::RenderTargetFormat;
	PSOCreateInfo.GraphicsPipeline.DSVFormat = RenderTarget::DepthBufferFormat;
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

		Diligent::BufferDesc CBDesc;
		CBDesc.Name = "VS constants CB";
		CBDesc.Size = sizeof(glm::mat4);
		CBDesc.Usage = Diligent::USAGE_DYNAMIC;
		CBDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
		CBDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
		mDevice->CreateBuffer(CBDesc, nullptr, &mVSConstants);
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

	Diligent::LayoutElement LayoutElems[] =
	{
		// Attribute 0 - vertex position
		Diligent::LayoutElement{0, 0, 3, Diligent::VT_FLOAT32, Diligent::False},
		// Attribute 1 - vertex color
		Diligent::LayoutElement{1, 0, 4, Diligent::VT_FLOAT32, Diligent::False}
	};
	PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
	PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements = _countof(LayoutElems);
	PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	Diligent::BufferDesc VertBuffDesc;
	VertBuffDesc.Name = "Cube vertex buffer";
	VertBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
	VertBuffDesc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
	VertBuffDesc.Size = sizeof(QuadVerts);
	Diligent::BufferData VBData;
	VBData.pData = QuadVerts;
	VBData.DataSize = sizeof(QuadVerts);
	mDevice->CreateBuffer(VertBuffDesc, &VBData, &mQuadVertexBuffer);

	Diligent::BufferDesc IndBuffDesc;
	IndBuffDesc.Name = "Cube index buffer";
	IndBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
	IndBuffDesc.BindFlags = Diligent::BIND_INDEX_BUFFER;
	IndBuffDesc.Size = sizeof(QuadIndices);
	Diligent::BufferData IBData;
	IBData.pData = QuadIndices;
	IBData.DataSize = sizeof(QuadIndices);
	mDevice->CreateBuffer(IndBuffDesc, &IBData, &mQuadIndexBuffer);

	mDevice->CreateGraphicsPipelineState(PSOCreateInfo, &mPSO);

	mPSO->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "Constants")->Set(mVSConstants);
	mPSO->CreateShaderResourceBinding(&mSRB, true);

	
	// Render target creation


	Diligent::RefCntAutoPtr<Diligent::IShader> RTPixelShader;
	Diligent::RefCntAutoPtr<Diligent::IBuffer> RTPSConstants;
	{
		ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
		ShaderCI.EntryPoint = "main";
		ShaderCI.Desc.Name = "Render target pixel shader";
		ShaderCI.Source = RenderTargetPSSource;
		mDevice->CreateShader(ShaderCI, &RTPixelShader);

		Diligent::BufferDesc CBDesc;
		CBDesc.Name = "RTPS constants CB";
		CBDesc.Size = sizeof(float);
		CBDesc.Usage = Diligent::USAGE_DYNAMIC;
		CBDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
		CBDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
		mDevice->CreateBuffer(CBDesc, nullptr, &RTPSConstants);
	}
	
	RenderTargetCreateInfo renderTargetCI;
	renderTargetCI.device = mDevice;
	renderTargetCI.swapChain = mSwapChain;
	renderTargetCI.width = mWidth;
	renderTargetCI.height = mHeight;
	renderTargetCI.pixelShader = RTPixelShader;
	renderTargetCI.uniformBuffer = RTPSConstants;

	mRenderTarget = std::make_shared<RenderTarget>(renderTargetCI);
}

float App::getTime()
{
	return (float)glfwGetTime();
}
