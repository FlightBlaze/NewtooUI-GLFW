#include <App.h>
#include <Resource.h>
#include <Path.h>
#include <Tube.h>
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
#include <codecvt>

App::App():
	mQuadPhysicalProps(1.0f, 95.0f, 5.0f),
	mQuadPosX(-100.0f, 100.0f, mQuadPhysicalProps)
{
}

int App::run()
{
	if(SDL_Init(SDL_INIT_VIDEO) != 0) {
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
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
		#ifdef PLATFORM_MACOS
		// | SDL_WINDOW_VULKAN
		#endif
		);
	
	if(mWindow == nullptr) {
		std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
		exit(-1);
	}

	initializeDiligentEngine();
	initializeResources();

	// resize(getActualWidth(), getActualHeight());

	bool quit = false;
	SDL_Event currentEvent;
    
    bool redrawRay = false;
    int rayX = 0, rayY = 0;
    
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
                        isMouseDown = true;
                        redrawRay = true;
                        rayX = x;
                        rayY = y;
                    }
				}
				break;
            case SDL_MOUSEBUTTONUP:
                if(currentEvent.button.button == SDL_BUTTON_LEFT) {
                    isMouseDown = false;
                }
            case SDL_MOUSEMOTION:
                if(isMouseDown) {
                    redrawRay = true;
                    rayX = currentEvent.motion.x;
                    rayY = currentEvent.motion.y;
                }
                break;
			default:
				break;
			}
		}
        
        if(redrawRay) {
            glm::vec2 rayOrigin = glm::vec2((float)rayX, (float)rayY);
            glm::vec2 rayDir = glm::vec2(0.0f, 1.0f);
            
            glm::vec2 relativePoint = mSquircleFill.offset +
                rayOrigin - mSquirclePos;
            
            float intersectionDistance = mSquircleFill.intersectsRay(relativePoint, rayDir);
            float endY = mHeight;
            if(intersectionDistance != NoIntersection) {
                glm::vec2 intersectionPoint = relativePoint + rayDir * intersectionDistance -
                    mSquircleFill.offset + mSquirclePos;
                float absoluteIntersectionDistance = glm::distance(intersectionPoint, rayOrigin);
                endY = intersectionPoint.y;
            }
            
            tube::Path path;
            path.points = {
                tube::Point(glm::vec3(rayX, rayY, 0.0f)),
                tube::Point(glm::vec3(rayX, endY, 0.0f))
            };
            tube::Builder builder = tube::Builder(path).withShape(tube::Shapes::circle(2.0f, 4));
            
            mRayStroke = CreateStroke(mDevice, builder);
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
	const float ClearColor[] = { 0.5f,  0.5f,  0.5f, 1.0f };
	const float Transparent[] = { 0.0f,  0.0f,  0.0f, 0.0f };

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
    
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    
//	mShapeRenderer->draw(mImmediateContext,
//		glm::translate(mViewProjection, glm::vec3(mSquirclePos - mSquircleFill.offset, 0.0f)), mSquircleFill, glm::vec3(1.0f), 1.0f);
//
//    mShapeRenderer->draw(mImmediateContext,
//        glm::translate(mViewProjection, glm::vec3(mSquirclePos - mSquircleFill.offset, 0.0f)), mSquircleStroke, glm::vec3(0.0f), 1.0f);
//
//    glm::vec3 textColor = glm::mix(glm::vec3(1.0, 0.1, 0.4), glm::vec3(0.2, 0.2, 0.8), sinf(getTime()) / 2.0f + 1.0f);
//
//	mTextRenderer->draw(mImmediateContext,
//		mTextModelViewProjection, L"Здравствуй, мир!", mTextSize, textColor);
//
//    if(isMouseDown) {
//        mShapeRenderer->draw(mImmediateContext,
//            glm::translate(mViewProjection, glm::vec3(0.0f)), mRayStroke, glm::vec3(1.0f, 0.0f, 0.0f), 0.75f);
//    }
//
//    tube::Path path;
//    path.points = {
//        tube::Point(glm::vec3(500.0f, 100.0f, 0.0f), glm::vec3(550.0f, 60.0f, 0.0f)),
//        tube::Point(glm::vec3(600.0f, 100.0f, 0.0f))
//    };
//    tube::TwoPathes twoPathes = path.divide(0.5);
//
//    tube::Builder path1Builder = tube::Builder(twoPathes.first)
//                                .withRoundedCaps(2.0f)
//                                .withShape(tube::Shapes::circle(2.0f, 4));
//
//    tube::Builder path2Builder = tube::Builder(twoPathes.second)
//                                .withRoundedCaps(2.0f)
//                                .withShape(tube::Shapes::circle(2.0f, 4));
//
//    Shape stroke1 = CreateStroke(mDevice, path1Builder);
//    Shape stroke2 = CreateStroke(mDevice, path2Builder);
//
//    mShapeRenderer->draw(mImmediateContext,
//        glm::translate(mViewProjection, glm::vec3(0.0f)), stroke1, glm::vec3(1.0f, 0.1f, 0.1f), 1.0f);
//
//    mShapeRenderer->draw(mImmediateContext,
//        glm::translate(mViewProjection, glm::vec3(0.0f)), stroke2, glm::vec3(0.0f), 0.5f);
    
//    ctx.currentPass = ContextPass::LAYOUT;
//    doUI();
//    ctx.currentPass = ContextPass::DRAW;
//    doUI();
    
    std::wstring FPS = L"FPS: " + converter.from_bytes(std::to_string(mLastFPS).c_str()) + L", raycasts per frame: " + converter.from_bytes(std::to_string(mLastRaycasts).c_str());
    mTextRenderer->draw(mImmediateContext,
        glm::translate(mViewProjection, glm::vec3(glm::vec2(10.0f, 10.0f), 0.0f)), FPS, 16.0f);
    
    float t = 0.5f + sinf(getTime()) / 2.0f;
    
    std::vector<glm::vec2> points1 = {
        glm::vec2(0.0f, 0.0f),
        glm::vec2(40.0f, 70.0f),
        glm::vec2(90.0f, 120.0f)
    };
    auto lines1 = dashedPolyline(points1, 24.0f, 12.0f, sin(getTime()) * 80.0f);
    ShapeMesh mesh;
    for(auto line : lines1) {
        ShapeMesh strokeMesh = strokePolyline(line, 16);
        mesh.add(strokeMesh);
    }
    std::vector<glm::vec2> points2 = {
        glm::vec2(90.0f, 120.0f),
        glm::vec2(180.0f + cos(getTime()) * 150, 100.0f + sin(getTime()) * 100)
    };
    // points2 = dividePolyline(points2, t).first;
    ShapeMesh mesh2 = strokePolyline(points2, 16);
    mesh.add(mesh2);
    ShapeMesh join = roundJoin(points1, points2, 16);
    mesh.add(join);
    ShapeMesh cap1 = roundedCap(points2.back(),
                                points2[points2.size() - 1] - points2[points2.size() - 2],
                                16);
    mesh.add(cap1);
    Shape stroke = CreateShapeFromMesh(mDevice, mesh);
    mShapeRenderer->draw(mImmediateContext,
    glm::translate(mViewProjection, glm::vec3(100.0f, 100.0f, 0.0f)), stroke, glm::vec3(0.0f), 1.0f);
    
    {
        std::vector<glm::vec2> line1 = {
            glm::vec2(0.0f, 0.0f),
            glm::vec2(80.0f, 110.0f)
        };
        std::vector<glm::vec2> line2 = {
            glm::vec2(80.0f, 110.0f),
            glm::vec2(-80.0f, 110.0f)
        };
        std::vector<glm::vec2> line3 = {
            glm::vec2(-80.0f, 110.0f),
            glm::vec2(0.0f, 0.0f)
        };
        float thickness = 16;
        
        ShapeMesh mesh1 = strokePolyline(line1, thickness);
        ShapeMesh mesh2 = strokePolyline(line2, thickness);
        ShapeMesh mesh3 = strokePolyline(line3, thickness);
        
        ShapeMesh join1;
        ShapeMesh join2;
        ShapeMesh join3;
        
        float time = getTime();
        float timeCycle = 3;
        float localTime = time - floorf(time / timeCycle) * timeCycle;
        
        if(localTime < 1) {
            join1 = miterJoin(line1, line2, thickness);
            join2 = miterJoin(line2, line3, thickness);
            join3 = miterJoin(line3, line1, thickness);
        } else if(localTime < 2) {
            join1 = roundJoin(line1, line2, thickness);
            join2 = roundJoin(line2, line3, thickness);
            join3 = roundJoin(line3, line1, thickness);
        } else if(localTime < 3) {
            join1 = bevelJoin(line1, line2, thickness);
            join2 = bevelJoin(line2, line3, thickness);
            join3 = bevelJoin(line3, line1, thickness);
        }
            
        mesh1.add(join1);
        mesh1.add(mesh2);
        mesh1.add(join2);
        mesh1.add(mesh3);
        mesh1.add(join3);
        
        Shape triangle = CreateShapeFromMesh(mDevice, mesh1);
        mShapeRenderer->draw(mImmediateContext,
        glm::translate(mViewProjection, glm::vec3(600.0f, 300.0f, 0.0f)), triangle, glm::vec3(0.0f), 1.0f);
    }
    
    bvgCtx.orthographic(mWidth, mHeight);
    
    bvgCtx.beginPath();
    bvgCtx.moveTo(200, 200 + 200);
    bvgCtx.cubicTo(200, 150 + 200, 300, 150 + 200, 300, 200 + 200);
    //bvgCtx.quadraticTo(300, 230, 250, 250);
    bvgCtx.lineTo(250, 250 + 200);
    // bvgCtx.closePath();
    bvgCtx.lineWidth = 8.0f;
    bvgCtx.lineDash = bvg::LineDash(12, 6);
    bvgCtx.lineDash.offset = sin(getTime()) * 80.0f;
    // bvgCtx.lineJoin = bvg::LineJoin::Round;
    bvgCtx.lineCap = bvg::LineCap::Round;
    bvgCtx.strokeStyle = bvg::SolidColor(bvg::Color(1.0f, 0.1f, 0.1f));
    bvgCtx.fillStyle = bvg::SolidColor(bvg::colors::White);
    bvgCtx.convexFill();
    bvgCtx.stroke();
    
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
    
    mLastRaycasts = ctx.NumRaycasts;
    ctx.NumRaycasts = 0;

	if (mQuadTime >= 2.0f) {
		mQuadTime = 0.0f;
		mQuadPosX.targetValue = -mQuadPosX.targetValue;
	}

	mQuadPosX.update(mDeltaTime);

	mRotation = sinf(getTime()) * 0.57f;

	glm::mat4 view = glm::identity<glm::mat4>();
	glm::mat4 projection = glm::ortho(0.0f, (float)mWidth, (float)mHeight, 0.0f, -1000.0f, 1000.0f); // glm::perspective(65.0f, (float)mWidth / (float)mHeight, 0.001f, 100.0f);
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

	mShapeRenderer = std::make_shared<ShapeRenderer>(ShapeRenderer(mDevice, mSwapChain));

	tube::Path path;
	const static glm::vec2 size = glm::vec2(100.0f * 2.0f, 160.0f * 2.0f);
	path.points = {
		tube::Point(glm::vec3(0.0f, size.y / 2.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
		tube::Point(glm::vec3(size.x / 2.0f, 0.0f, 0.0f), glm::vec3(size.x, 0.0f, 0.0f)),
		tube::Point(glm::vec3(size.x, size.y / 2.0f, 0.0f), glm::vec3(size.x, size.y, 0.0f)),
		tube::Point(glm::vec3(size.x / 2.0f, size.y, 0.0f), glm::vec3(0.0f, size.y, 0.0f)),
		tube::Point(glm::vec3(0.0f, size.y / 2.0f, 0.0f))
	};
    // path = path.toPoly().evenlyDistributed(60);
    path.closed = true;
    tube::Builder builder = tube::Builder(path)
        .toPoly()
        .evenlyDistributed(4)
        .dash(10, 10)
        .withRoundedCaps(2.0f)
        .withShape(tube::Shapes::circle(2.0f, 4));
    
    mSquircleShape = std::make_shared<Shape>(CreateFill(mDevice, path, true));
	mSquircleFill = CreateFill(mDevice, path, true);
    mSquircleStroke = CreateStroke(mDevice, builder);
    
	initializeFont();
    
    ctx.textRenderer = mTextRenderer;
    ctx.shapeRenderer = mShapeRenderer;
    ctx.context = mImmediateContext;
    ctx.renderDevice = mDevice;
    ctx.swapChain = mSwapChain;
    
    bvgCtx = bvg::DiligentContext(mWidth, mHeight, mDevice,
                                  mImmediateContext,
                                  mSwapChain->GetDesc().ColorBufferFormat,
                                  mSwapChain->GetDesc().DepthBufferFormat);
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

void App::doUI() {
    beginAffine(ctx, glm::translate(mViewProjection, glm::vec3(100.0f, 100.0f, 0.0f)));
    {
        if(ctx.currentPass == ContextPass::DRAW) {
            mShapeRenderer->draw(
                mImmediateContext,
                ctx.affineList.back().world *
                glm::translate(glm::mat4(1.0f), glm::vec3(mSquircleShape->offset, 0.0f)),
                *mSquircleShape.get(),
                glm::vec3(1.0f),
                0.5f);
        }
        
        beginBoundary(ctx, mSquircleShape);
        {
            elements::Text(ctx, L"Идейные соображения высшего порядка, а также рамки и место обучения кадров способствует подготовки и реализации дальнейших направлений развития", ctx.textRenderer,
                           24.0f, Color(0.0f, 0.0f, 0.0f, 1.0f));
        }
        endBoundary(ctx);
    }
    endAffine(ctx);
}
