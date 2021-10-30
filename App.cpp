#include <App.h>
#include <Resource.h>
#include <Path.h>
#include <iostream>
#ifdef PLATFORM_MACOS
extern "C" {
#include <platform/MacOS/LayerManager.h>
}
// #define GLFW_EXPOSE_NATIVE_COCOA
#include "Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h"
#else
// #define GLFW_EXPOSE_NATIVE_WIN32
#include "Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h"
#include "Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h"
#endif
// #include <GLFW/glfw3native.h>
#include <SDL_syswm.h>
#include <SDL_events.h>
#include "Graphics/GraphicsTools/interface/CommonlyUsedStates.h"
#include "Graphics/GraphicsTools/interface/MapHelper.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <stb_image.h>
#include <unistd.h>

App::App():
	mQuadPhysicalProps(1.0f, 95.0f, 5.0f),
	mQuadPosX(-100.0f, 100.0f, mQuadPhysicalProps)
{
}

int App::run()
{
	if(SDL_Init( SDL_INIT_VIDEO ) != 0) {
    	std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
 		exit(-1);
	}
	mWidth = 800;
	mHeight = 600;
	mWindow = SDL_CreateWindow(
		"My Project",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		mWidth,
		mHeight,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	
	if(mWindow == nullptr) {
		std::cerr << "Failed to create window" << std::endl;
		exit(-1);
	}

	float ddpi, hdpi, vdpi;
	SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi);
	float inchPx = 96;
	mDotsPerPixel = hdpi / inchPx;

	initializeDiligentEngine();
	initializeResources();

	// resize(getActualWidth(), getActualHeight());

	bool quit = false;
	SDL_Event currentEvent;
	while(!quit) {
		while(SDL_PollEvent(&currentEvent) != 0) {
			switch(currentEvent.type) {
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_WINDOWEVENT:
				switch(currentEvent.window.event) {
				case SDL_WINDOWEVENT_RESIZED:
					mWidth = currentEvent.window.data1;
					mHeight = currentEvent.window.data2;
					resize(mWidth, mHeight);
					break;
				default:
					break;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				if(currentEvent.button.button == SDL_BUTTON_LEFT) {
					if(currentEvent.button.state == SDL_PRESSED) {
						int x = currentEvent.button.x;
						int y = currentEvent.button.y;
						glm::vec2 relativePoint = mSquircleFill.offset +
							glm::vec2((float)x, (float)y) - mSquirclePos;
						bool isInside = mSquircleFill.containsPoint(relativePoint);
						std::cout << "Is inside squircle: " << isInside << std::endl;
					}
				}
				break;
			default:
				break;
			}
		}
		update();
		draw();
	}
	SDL_DestroyWindow(mWindow);
	SDL_Quit();
	return 0;
}

void App::draw()
{
	/*MotionBlurConstants motionBlur;
	motionBlur.direction = glm::vec2(1.0f, 0.0f);
	motionBlur.resolution = glm::vec2(mWidth, mHeight);

	RenderTargetCreateInfo renderTargetCI;
	renderTargetCI.device = mDevice;
	renderTargetCI.swapChain = mSwapChain;
	renderTargetCI.pixelShader = mMotionBlurPS;
	renderTargetCI.uniformBuffer = mMotionBlurConstants;
	renderTargetCI.alphaBlending = false;
	mRenderTargets.current->recreatePipelineState(renderTargetCI);
	mRenderTargets.previous->recreatePipelineState(renderTargetCI);
	{
		Diligent::MapHelper<MotionBlurConstants> CBConstants(mImmediateContext, mRenderTargets.current->uniformBuffer,
			Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
		*CBConstants = motionBlur;
	}
	{
		Diligent::MapHelper<MotionBlurConstants> CBConstants(mImmediateContext, mRenderTargets.previous->uniformBuffer,
			Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
		*CBConstants = motionBlur;
	}*/

	const float ClearColor[] = { 0.5f,  0.5f,  0.5f, 1.0f };
	const float Transparent[] = { 0.0f,  0.0f,  0.0f, 0.0f };

	// mRenderTargets.current->use(mImmediateContext);
	// mRenderTargets.current->clear(mImmediateContext, Transparent);

	auto* pRTV = mSwapChain->GetCurrentBackBufferRTV();
	auto* pDSV = mSwapChain->GetDepthBufferDSV();

	mImmediateContext->SetRenderTargets(1, &pRTV, pDSV,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	mImmediateContext->ClearRenderTarget(pRTV, ClearColor,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	mImmediateContext->ClearDepthStencil(pDSV, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

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

	float speed = fabsf(mQuadPosX.velocity);
	std::vector<float>* kernel = nullptr;
	if (speed > 110)
		kernel = &GaussianKernel5x1;
	if (speed > 190)
		kernel = &GaussianKernel9x1;
	if (speed > 250)
		kernel = &GaussianKernel15x1;
	if (speed > 380)
		kernel = &GaussianKernel31x1;
	if (kernel != nullptr) {
		glm::vec2 blurDirection = glm::vec2(1.0f, 0.0f);
		int kernelSize = (int)kernel->size();
		int kernelCenter = kernelSize / 2;
		for (int i = 0; i < kernelSize; i++) {
			float offset = (float)(i - kernelCenter);
			float stepPx = 1.0f;
			glm::vec2 resolution = glm::vec2(mWidth, mHeight);
			glm::vec3 offsetInDirection = glm::vec3(glm::vec2(blurDirection * offset * stepPx) / resolution, 0.0f);
			glm::mat4 MVP = glm::translate(offsetInDirection) * mModelViewProjection;
			// Write model view projection matrix to uniform
			{
				Diligent::MapHelper<glm::mat4> CBConstants(mImmediateContext, mVSConstants,
					Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
				*CBConstants = glm::transpose(MVP);
			}
			{
				Diligent::MapHelper<float> CBConstants(mImmediateContext, mPSConstants,
					Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
				*CBConstants = kernel->at(i);
			}
			mImmediateContext->DrawIndexed(DrawAttrs);
		}
	}
	else {
		{
			Diligent::MapHelper<glm::mat4> CBConstants(mImmediateContext, mVSConstants,
				Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
			*CBConstants = glm::transpose(mModelViewProjection);
		}
		{
			Diligent::MapHelper<float> CBConstants(mImmediateContext, mPSConstants,
				Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
			*CBConstants = 1.0f; // opacity
		}
		mImmediateContext->DrawIndexed(DrawAttrs);
	}

	// /*int blurIterations = (int)fabsf(mQuadPosX.velocity) / 20;
	// if (blurIterations < 25)
	// 	blurIterations /= 4;
	// if (blurIterations > 80)
	// 	blurIterations = 80;
	// for (int i = 0; i < blurIterations; i++) {
	// 	mRenderTargets.swap();
	// 	mRenderTargets.current->use(mImmediateContext);
	// 	mRenderTargets.previous->draw(mImmediateContext);
	// }*/

	// /*renderTargetCI.pixelShader = mPrintPS;
	// renderTargetCI.uniformBuffer = nullptr;
	// renderTargetCI.alphaBlending = true;
	// mRenderTargets.current->recreatePipelineState(renderTargetCI);
	// mRenderTargets.previous->recreatePipelineState(renderTargetCI);*/

	// // mRenderTargets.current->draw(mImmediateContext);

	mSolidFillRenderer->draw(mImmediateContext,
		glm::translate(mViewProjection, glm::vec3(mSquirclePos - mSquircleFill.offset, 0.0f)), mSquircleFill, glm::vec3(1.0f), 1.0f);

	mTextRenderer->draw(mImmediateContext,
		mTextModelViewProjection, "Hello world!", mTextSize, 1.0f);
	
	std::string FPS = std::string("FPS: ") +std::to_string(mLastFPS);
	mTextRenderer->draw(mImmediateContext,
		glm::translate(mViewProjection, glm::vec3(glm::vec2(10.0f, 10.0f), 0.0f)), FPS, 16.0f, 1.0f);

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
		mLastFPS = mFPS;
		mFPS = 0;
	}

	if (mQuadTime >= 2.0f) {
		mQuadTime = 0.0f;
		mQuadPosX.targetValue = -mQuadPosX.targetValue;
	}

	mQuadPosX.update(mDeltaTime);

	mRotation = sinf(getTime()) * 0.57f;

	glm::mat4 view = glm::identity<glm::mat4>();
	glm::mat4 projection = glm::ortho(0.0f, (float)mWidth, (float)mHeight, 0.0f); // glm::perspective(65.0f, (float)mWidth / (float)mHeight, 0.001f, 100.0f);
	glm::mat4 model = glm::rotate(glm::translate(glm::vec3(mWidth / 2 + mQuadPosX.currentValue, mHeight - 80.0f, 0.0f)), mRotation, glm::vec3(0, 0, 1));

	mViewProjection = projection * view;
	mModelViewProjection = mViewProjection * model;

	glm::mat4 textModel = glm::rotate(glm::translate(glm::vec3(100.0f, 100.0f, 0.0f)), mRotation, glm::vec3(0, 0, 1));
	mTextModelViewProjection = mViewProjection * textModel;
	mTextSize = sinf(getTime()) * 32.0f + 32.0f + 10.0f;

	mSquirclePos = glm::vec2(mWidth / 2 - mSquircleFill.width / 2, mHeight / 2 - mSquircleFill.height / 2);
}

void App::resize(int width, int height)
{
	mSwapChain->Resize(width, height);

	// RenderTargetCreateInfo renderTargetCI;
	// renderTargetCI.device = mDevice;
	// renderTargetCI.swapChain = mSwapChain;
	// renderTargetCI.width = width;
	// renderTargetCI.height = height;
	// mRenderTargets.current->recreateTextures(renderTargetCI);
	// mRenderTargets.previous->recreateTextures(renderTargetCI);
}

void App::initializeDiligentEngine()
{
	Diligent::SwapChainDesc SCDesc;
	SDL_SysWMinfo wmi;
	SDL_VERSION(&wmi.version);
	if (!SDL_GetWindowWMInfo(mWindow, &wmi)) {
		std::cerr << "Could not get window manager information" << std::endl;
		exit(-1);
	}
	#ifdef PLATFORM_MACOS
	Diligent::MacOSNativeWindow window;
	window.pNSView = setupLayersAndGetView(wmi.info.cocoa.window);

	// We need at least 3 buffers in Metal to avoid massive
    // performance degradation in full screen mode.
    // https://github.com/KhronosGroup/MoltenVK/issues/808
    SCDesc.BufferCount = 3;
	#else
	Diligent::Win32NativeWindow window{ glfwGetWin32Window(mWindow) };
	#endif
	switch (mDeviceType)
	{
		#ifdef PLATFORM_MACOS
		case Diligent::RENDER_DEVICE_TYPE_VULKAN:
			{
				auto* pFactoryVk = Diligent::GetEngineFactoryVk();
				Diligent::EngineVkCreateInfo EngineCI;
				
				pFactoryVk->CreateDeviceAndContextsVk(EngineCI, &mDevice, &mImmediateContext);
				if (!mDevice)
				{
					std::cerr << "Unable to initialize Diligent Engine in Vulkan mode. The API may not be available, "
							"or required features may not be supported by this GPU/driver/OS version." << std::endl;
					exit(-1);
				}
				pFactoryVk->CreateSwapChainVk(mDevice, mImmediateContext, SCDesc, window, &mSwapChain);
				}
			break;
		#else
		case Diligent::RENDER_DEVICE_TYPE_GL:
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
		#endif
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
	PSOCreateInfo.GraphicsPipeline.RTVFormats[0] = mSwapChain->GetDesc().ColorBufferFormat;
	PSOCreateInfo.GraphicsPipeline.DSVFormat = mSwapChain->GetDesc().DepthBufferFormat;
	PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
	PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = Diligent::False;

	Diligent::BlendStateDesc BlendState;
	BlendState.RenderTargets[0].BlendEnable = Diligent::True;
	BlendState.RenderTargets[0].SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
	BlendState.RenderTargets[0].DestBlend = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
	// BlendState.RenderTargets[0].SrcBlendAlpha = Diligent::BLEND_FACTOR_ONE;
	// BlendState.RenderTargets[0].DestBlendAlpha = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
	PSOCreateInfo.GraphicsPipeline.BlendDesc = BlendState;

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

		Diligent::BufferDesc CBDesc;
		CBDesc.Name = "PS constants CB";
		CBDesc.Size = sizeof(float);
		CBDesc.Usage = Diligent::USAGE_DYNAMIC;
		CBDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
		CBDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
		mDevice->CreateBuffer(CBDesc, nullptr, &mPSConstants);
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
	VertBuffDesc.Name = "Quad vertex buffer";
	VertBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
	VertBuffDesc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
	VertBuffDesc.Size = sizeof(QuadVerts);
	Diligent::BufferData VBData;
	VBData.pData = QuadVerts;
	VBData.DataSize = sizeof(QuadVerts);
	mDevice->CreateBuffer(VertBuffDesc, &VBData, &mQuadVertexBuffer);

	Diligent::BufferDesc IndBuffDesc;
	IndBuffDesc.Name = "Quad index buffer";
	IndBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
	IndBuffDesc.BindFlags = Diligent::BIND_INDEX_BUFFER;
	IndBuffDesc.Size = sizeof(QuadIndices);
	Diligent::BufferData IBData;
	IBData.pData = QuadIndices;
	IBData.DataSize = sizeof(QuadIndices);
	mDevice->CreateBuffer(IndBuffDesc, &IBData, &mQuadIndexBuffer);

	mDevice->CreateGraphicsPipelineState(PSOCreateInfo, &mPSO);

	mPSO->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "Constants")->Set(mVSConstants);
	mPSO->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "Constants")->Set(mPSConstants);
	mPSO->CreateShaderResourceBinding(&mSRB, true);

	
	// Render target creation


	// {
	// 	ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
	// 	ShaderCI.EntryPoint = "main";
	// 	ShaderCI.Desc.Name = "Render target pixel shader";
	// 	ShaderCI.Source = RenderTargetPSSource;
	// 	mDevice->CreateShader(ShaderCI, &mMotionBlurPS);

	// 	ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
	// 	ShaderCI.EntryPoint = "main";
	// 	ShaderCI.Desc.Name = "Render target pixel shader";
	// 	ShaderCI.Source = PrintPSSource;
	// 	mDevice->CreateShader(ShaderCI, &mPrintPS);

	// 	Diligent::BufferDesc CBDesc;
	// 	CBDesc.Name = "RTPS constants CB";
	// 	CBDesc.Size = sizeof(MotionBlurConstants);
	// 	CBDesc.Usage = Diligent::USAGE_DYNAMIC;
	// 	CBDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
	// 	CBDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
	// 	mDevice->CreateBuffer(CBDesc, nullptr, &mMotionBlurConstants);
	// }
	
	// RenderTargetCreateInfo renderTargetCI;
	// renderTargetCI.device = mDevice;
	// renderTargetCI.swapChain = mSwapChain;
	// renderTargetCI.width = mWidth;
	// renderTargetCI.height = mHeight;
	// renderTargetCI.pixelShader = mPrintPS;
	// renderTargetCI.uniformBuffer = nullptr;
	// renderTargetCI.alphaBlending = true;

	// mRenderTargets.current = std::make_shared<RenderTarget>(renderTargetCI);
	// mRenderTargets.previous = std::make_shared<RenderTarget>(renderTargetCI);

	mSolidFillRenderer = std::make_shared<SolidFillRenderer>(SolidFillRenderer(mDevice, mSwapChain));

	tube::Path path;
	const static glm::vec2 size = glm::vec2(100.0f, 160.0f);
	path.points = {
		tube::Point(glm::vec3(0.0f, size.y / 2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
		tube::Point(glm::vec3(size.x / 2.0f, 0.0f, 0.0f), glm::vec3(size.x, 0.0f, 0.0f)),
		tube::Point(glm::vec3(size.x, size.y / 2.0f, 0.0f), glm::vec3(size.x, size.y, 0.0f)),
		tube::Point(glm::vec3(size.x / 2.0f, size.y, 0.0f), glm::vec3(0.0f, size.y, 0.0f)),
		tube::Point(glm::vec3(0.0f, size.y / 2.0f, 0.0f))
	};

	mSquircleFill = CreateFill(mDevice, path, true);

	initializeFont();
}

void App::initializeFont() {
	std::vector<std::shared_ptr<ui::FontAtlas>> atlases;

	const static char* defaultFontAtlasPath = "assets/Roboto-Regular.png";
	const static char* defaultFontPath = "assets/Roboto-Regular.fnt";

	// Check if files exist
	if(access(defaultFontAtlasPath, 0) != 0 || access(defaultFontPath, 0) != 0) {
		std::cerr << "Could not load font files" << std::endl;
		exit(-1);
	}

	int width, height, numChannels;
	unsigned char* data = stbi_load(defaultFontAtlasPath, &width, &height, &numChannels, 0);
	atlases.push_back(
		std::make_shared<ui::FontAtlas>(
			ui::FontAtlas(
				mDevice, mSwapChain, defaultFontAtlasPath, data, width, height, numChannels
			)
		));
	stbi_image_free(data);

	std::string fontFnt = LoadTextResource("Roboto-Regular.fnt");
	mFont = ui::LoadFont(mDevice, fontFnt, atlases);
	mTextRenderer = std::make_shared<TextRenderer>(TextRenderer(mDevice, mSwapChain, mFont));
}

float App::getTime()
{
	return (float)SDL_GetTicks() / 1000.0f;
}

int App::getActualWidth() {
	return (int)((float)mWidth * mDotsPerPixel);
}

int App::getActualHeight() {
	return (int)((float)mHeight * mDotsPerPixel);
}
