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
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stb_image.h>
#include <unistd.h>
#include <codecvt>

App::App():
	mQuadPhysicalProps(1.0f, 95.0f, 5.0f),
	mQuadPosX(-100.0f, 100.0f, mQuadPhysicalProps),
    ctx(bvgCtx),
    mesh(),
    meshViewer(&this->mesh)
{
    /*
        1     2
     
        0     3
     
        5     4
     
        6     7
     */
    
    PolyMesh::VertexHandle vhandle[10];
     
    vhandle[0] = mesh.add_vertex(PolyMesh::Point(210.0f, 200.0f, 0));
    vhandle[1] = mesh.add_vertex(PolyMesh::Point(200.0f, 100.0f, 0));
    vhandle[2] = mesh.add_vertex(PolyMesh::Point(300.0f, 100.0f, 0));
    vhandle[3] = mesh.add_vertex(PolyMesh::Point(300.0f, 210.0f, 0));
    vhandle[4] = mesh.add_vertex(PolyMesh::Point(300.0f, 260.0f, 0));
    vhandle[5] = mesh.add_vertex(PolyMesh::Point(200.0f, 260.0f, 0));
    vhandle[6] = mesh.add_vertex(PolyMesh::Point(300.0f, 320.0f, 0));
    vhandle[7] = mesh.add_vertex(PolyMesh::Point(200.0f, 320.0f, 0));
    vhandle[8] = mesh.add_vertex(PolyMesh::Point(200.0f, 410.0f, 0));
    vhandle[9] = mesh.add_vertex(PolyMesh::Point(300.0f, 410.0f, 0));
    
    std::vector<PolyMesh::VertexHandle>  face_vhandles;
     
    face_vhandles.clear();
    face_vhandles.push_back(vhandle[0]);
    face_vhandles.push_back(vhandle[1]);
    face_vhandles.push_back(vhandle[2]);
    face_vhandles.push_back(vhandle[3]);
    mesh.add_face(face_vhandles);

    face_vhandles.clear();
    face_vhandles.push_back(vhandle[0]);
    face_vhandles.push_back(vhandle[3]);
    face_vhandles.push_back(vhandle[4]);
    face_vhandles.push_back(vhandle[5]);
    mesh.add_face(face_vhandles);

    face_vhandles.clear();
    face_vhandles.push_back(vhandle[5]);
    face_vhandles.push_back(vhandle[4]);
    face_vhandles.push_back(vhandle[6]);
    face_vhandles.push_back(vhandle[7]);
    mesh.add_face(face_vhandles);
    
    face_vhandles.clear();
    face_vhandles.push_back(vhandle[9]);
    face_vhandles.push_back(vhandle[8]);
    face_vhandles.push_back(vhandle[7]);
    face_vhandles.push_back(vhandle[6]);
    mesh.add_face(face_vhandles);
    
    mesh.request_face_status();
    mesh.request_edge_status();
    mesh.request_halfedge_status();
    mesh.request_vertex_status();
    mesh.request_halfedge_texcoords2D();
    mesh.request_vertex_texcoords2D();
    
    std::vector<std::wstring> lines = {
        L"Esc - exit halfedge demo",
        L"Left click - select vertex",
        L"Shift + Left click - select multiple vertices",
        L"Hold G - move",
        L"Hold S - scale",
        L"A - select/deselect all",
        L"E - extrude",
        L"L - loop cut",
        L"Z - subdivide",
        L"; - mark edge loop",
        L"K - mark edge ring",
        L"D - mark selection boundary"
    };
    for(auto line : lines) {
        heSummary.insert(heSummary.end(), line.begin(), line.end());
        heSummary.push_back(L'\n'); // line end
    }
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
            case SDL_KEYDOWN:
                if(mDemoType == DemoType::HALF_EDGE) {
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                        mDemoType = DemoType::VECTOR_GRAPHICS;
                    
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_LSHIFT)
                        meshViewer.shiftPressed = true;
                    
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_G)
                        meshViewer.currentTransform = CurrentTransform::Move;
                    
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_S)
                        meshViewer.currentTransform = CurrentTransform::Scale;
                    
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_E)
                        extrude(mesh);
                    
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_D) {
                        for(auto heh : mesh.halfedges())
                            mesh.status(heh).set_selected(false);
                        extrude(mesh, true);
                    }
                    
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_L)
                        loopCut(mesh);
                    
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_K) {
                        for(auto heh : mesh.halfedges())
                            mesh.status(heh).set_selected(false);
                        loopCut(mesh, true);
                    }
                    
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_SEMICOLON) {
                        for(auto heh : mesh.halfedges())
                            mesh.status(heh).set_selected(false);
                        selectEdgeLoop(mesh);
                    }
                    
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_Z) {
                        OpenMesh::Subdivider::Uniform::CatmullClarkT<PolyMesh> subdiv;
                        subdiv.attach(mesh);
                        subdiv(1);
                        subdiv.detach();
                    }
                    
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_A) {
                        bool allSelected = true;
                        for(auto vh : mesh.vertices()) {
                            if(!vh.selected()) {
                                allSelected = false;
                                break;
                            }
                        }
                        if(allSelected) {
                            for(auto vh : mesh.vertices())
                                mesh.status(vh).set_selected(false);
                        } else {
                            for(auto vh : mesh.vertices())
                                mesh.status(vh).set_selected(true);
                        }
                    }
                    
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_O)
                        openRegion(mesh);
                    
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_B)
                        bevel(mesh, 3);
                    
//                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
//                        HE::HalfEdge* he = mesh.findDebugSelectedHalfEdge();
//                        if(he != nullptr) {
//                            he->next->isDebugSelected = true;
//                            he->isDebugSelected = false;
//                        }
//                    }
//                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_LEFT) {
//                        HE::HalfEdge* he = mesh.findDebugSelectedHalfEdge();
//                        if(he != nullptr) {
//                            he->prev->isDebugSelected = true;
//                            he->isDebugSelected = false;
//                        }
//                    }
//                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_DOWN ||
//                       currentEvent.key.keysym.scancode == SDL_SCANCODE_UP) {
//                        HE::HalfEdge* he = mesh.findDebugSelectedHalfEdge();
//                        if(he != nullptr) {
//                            he->twin->isDebugSelected = true;
//                            he->isDebugSelected = false;
//                        }
//                    }
                } else if(mDemoType == DemoType::HALF_EDGE_3D) {
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                        mDemoType = DemoType::VECTOR_GRAPHICS;
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_LCTRL) {
                        mIsControlPressed = true;
                    }
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_LSHIFT)
                        editor->isShiftPressed = true;
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_S) {
                        for(auto eh : model.originalMesh.edges()) {
                            if(eh.selected())
                                continue;
                            
                            model.originalMesh.status(eh).set_selected(true);
                            break;
                        }
                        model.invalidate(mDevice, mImmediateContext);
                    }
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_C) {
                        for(auto vh : model.originalMesh.vertices())
                            model.originalMesh.status(vh).set_selected(false);
                        for(auto heh : model.originalMesh.halfedges())
                            model.originalMesh.status(heh).set_selected(false);
                        for(auto eh : model.originalMesh.edges()) {
                            if(eh.selected()) {
                                model.originalMesh.status(eh.v0()).set_selected(true);
                                model.originalMesh.status(eh.v1()).set_selected(true);
                            }
                        }
                        PolyMesh::Point normal;
                        int numVerts = 0;
                        for(auto vh : model.originalMesh.vertices()) {
                            if(vh.selected()) {
                                normal += model.originalMesh.normal(vh);
                                numVerts++;
                            }
                        }
                        normal /= numVerts;
                        
//                        loopCut(model.originalMesh, false);
                        extrude(model.originalMesh);
                        model.originalMesh.garbage_collection();
                        
                        for(auto vh : model.originalMesh.vertices()) {
                            if(vh.selected()) {
//                                 model.originalMesh.set_point(vh, model.originalMesh.point(vh) + normal.normalize());
                                model.originalMesh.set_point(vh, model.originalMesh.point(vh) +
                                                             PolyMesh::Point(0.0f, 1.0f, 0.0f));
                            }
                        }
                        model.originalMesh.update_normals();
                        for(auto heh : model.originalMesh.halfedges()) {
                            if(heh.selected())
                                model.originalMesh.status(heh.edge()).set_selected(true);
                        }
                        
//                        mesh = model.originalMesh;
//                        for(auto vh : model.originalMesh.vertices()) {
//                            model.originalMesh.set_point(vh, model.originalMesh.point(vh) * 20.0f);
//                        }
                        
                        model.invalidate(mDevice, mImmediateContext);
                    }
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_B) {
                        for(auto vh : model.originalMesh.vertices())
                            model.originalMesh.status(vh).set_selected(false);
                        for(auto heh : model.originalMesh.halfedges())
                            model.originalMesh.status(heh).set_selected(false);
                        for(auto eh : model.originalMesh.edges()) {
                            if(eh.selected()) {
                                model.originalMesh.status(eh.v0()).set_selected(true);
                                model.originalMesh.status(eh.v1()).set_selected(true);
                            }
                        }
                        bevel(model.originalMesh, 3, 0.25f);
                        model.originalMesh.garbage_collection();
                        model.originalMesh.update_normals();
                        for(auto heh : model.originalMesh.halfedges()) {
                            if(heh.selected())
                                model.originalMesh.status(heh.edge()).set_selected(true);
                        }
                        model.invalidate(mDevice, mImmediateContext);
                    }
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_Z) {
                        OpenMesh::Subdivider::Uniform::CatmullClarkT<PolyMesh> subdiv;
                        subdiv.attach(model.originalMesh);
                        subdiv(1);
                        subdiv.detach();
                        model.invalidate(mDevice, mImmediateContext);
                    }
                        
                } else {
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_LCTRL)
                        mIsControlPressed = true;
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_T)
                        mGizmoTool = gizmo::GizmoTool::Translate;
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_R)
                        mGizmoTool = gizmo::GizmoTool::Rotate;
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_S)
                        mGizmoTool = gizmo::GizmoTool::Scale;
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_N)
                        mGizmoProps.enabledRotationSnap = !mGizmoProps.enabledRotationSnap;
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_SPACE)
                        mPlayRotation = !mPlayRotation;
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_G)
                        mDemoType = DemoType::GIZMOS;
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_V)
                        mDemoType = DemoType::VECTOR_GRAPHICS;
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_U)
                        mDemoType = DemoType::UI;
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_H)
                        mDemoType = DemoType::HALF_EDGE;
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_3)
                        mDemoType = DemoType::HALF_EDGE_3D;
                }
                break;
            case SDL_KEYUP:
                if(currentEvent.key.keysym.scancode == SDL_SCANCODE_LCTRL)
                    mIsControlPressed = false;
            
                if(mDemoType == DemoType::HALF_EDGE) {
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_LSHIFT)
                        meshViewer.shiftPressed = false;
                    
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_G)
                        meshViewer.currentTransform = CurrentTransform::None;
                    
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_S)
                        meshViewer.currentTransform = CurrentTransform::None;
                }
                else if(mDemoType == DemoType::HALF_EDGE_3D) {
                    if(currentEvent.key.keysym.scancode == SDL_SCANCODE_LSHIFT)
                        editor->isShiftPressed = false;
                }
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
//						int x = currentEvent.button.x;
//						int y = currentEvent.button.y;
//						glm::vec2 relativePoint = mSquircleFill.offset +
//							glm::vec2((float)x, (float)y) - mSquirclePos;
//						bool isInside = mSquircleFill.containsPoint(relativePoint);
//						std::cout << "Is inside squircle: " << isInside << std::endl;
//                        redrawRay = true;
//                        rayX = x;
//                        rayY = y;
                        isMouseDown = true;
                        isMouseMoved = false;
                    }
				}
				break;
            case SDL_MOUSEBUTTONUP:
                if(currentEvent.button.button == SDL_BUTTON_LEFT) {
                    isMouseDown = false;
                    if(mDemoType == DemoType::HALF_EDGE_3D) {
                        if(!isMouseMoved) { // On click
                            glm::vec2 mouse = glm::vec2(mMouseX, mMouseY);
                            glm::vec2 screenDims = glm::vec2(mWidth, mHeight);
                            editor->raycastEdges(mouse, screenDims, mDevice, mImmediateContext);
                        }
                    }
                }
            case SDL_MOUSEMOTION:
                isMouseMoved = true;
                mMouseX = currentEvent.motion.x;
                mMouseY = currentEvent.motion.y;
                if(isMouseDown) {
                    if(mIsControlPressed) {
                        mZoom += currentEvent.motion.yrel * 0.05f;
                    } else {
                        redrawRay = true;
                        rayX = currentEvent.motion.x;
                        rayY = currentEvent.motion.y;
                        if(mGizmoState.selectedControl == gizmo::Control::None) {
                            mYaw -= glm::radians((float)currentEvent.motion.xrel) * 25.0f;
                            mPitch += glm::radians((float)currentEvent.motion.yrel) * 25.0f;
                        }
                    }
                }
                break;
			default:
				break;
			}
		}
        
//        if(redrawRay) {
//            glm::vec2 rayOrigin = glm::vec2((float)rayX, (float)rayY);
//            glm::vec2 rayDir = glm::vec2(0.0f, 1.0f);
//
//            glm::vec2 relativePoint = mSquircleFill.offset +
//                rayOrigin - mSquirclePos;
//
//            float intersectionDistance = mSquircleFill.intersectsRay(relativePoint, rayDir);
//            float endY = mHeight;
//            if(intersectionDistance != NoIntersection) {
//                glm::vec2 intersectionPoint = relativePoint + rayDir * intersectionDistance -
//                    mSquircleFill.offset + mSquirclePos;
//                float absoluteIntersectionDistance = glm::distance(intersectionPoint, rayOrigin);
//                endY = intersectionPoint.y;
//            }
//
//            tube::Path path;
//            path.points = {
//                tube::Point(glm::vec3(rayX, rayY, 0.0f)),
//                tube::Point(glm::vec3(rayX, endY, 0.0f))
//            };
//            tube::Builder builder = tube::Builder(path).withShape(tube::Shapes::circle(2.0f, 4));
//
//            mRayStroke = CreateStroke(mDevice, builder);
//        }
        
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

	Diligent::ITextureView* pRTV = mSwapChain->GetCurrentBackBufferRTV();
    Diligent::ITextureView* pDSV = mSwapChain->GetDepthBufferDSV();
    
	mImmediateContext->SetRenderTargets(1, &renderTarget.RTV, renderTarget.DSV,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	mImmediateContext->ClearRenderTarget(renderTarget.RTV, ClearColor,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	mImmediateContext->ClearDepthStencil(renderTarget.DSV, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

//	Diligent::Uint64   offset = 0;
//	Diligent::IBuffer* pBuffs[] = { mQuadVertexBuffer };
//	mImmediateContext->SetVertexBuffers(0, 1, pBuffs, &offset,
//		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
//		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
//	mImmediateContext->SetIndexBuffer(mQuadIndexBuffer, 0,
//		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
//
//	mImmediateContext->SetPipelineState(mPSO);
//
//	mImmediateContext->CommitShaderResources(mSRB, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
//
//	Diligent::DrawIndexedAttribs DrawAttrs;
//	DrawAttrs.IndexType = Diligent::VT_UINT32;
//	DrawAttrs.NumIndices = _countof(QuadIndices);
//	DrawAttrs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
//
//	float speed = fabsf(mQuadPosX.velocity);
//    speed = 0;
//	std::vector<float>* kernel = nullptr;
//	if (speed > 110)
//		kernel = &GaussianKernel5x1;
//	if (speed > 190)
//		kernel = &GaussianKernel9x1;
//	if (speed > 250)
//		kernel = &GaussianKernel15x1;
//	if (speed > 380)
//		kernel = &GaussianKernel31x1;
//	if (kernel != nullptr) {
//		glm::vec2 blurDirection = glm::vec2(1.0f, 0.0f);
//		int kernelSize = (int)kernel->size();
//		int kernelCenter = kernelSize / 2;
//		for (int i = 0; i < kernelSize; i++) {
//			float offset = (float)(i - kernelCenter);
//			float stepPx = 1.0f;
//			glm::vec2 resolution = glm::vec2(mWidth, mHeight);
//			glm::vec3 offsetInDirection = glm::vec3(glm::vec2(blurDirection * offset * stepPx) / resolution, 0.0f);
//			glm::mat4 MVP = glm::translate(offsetInDirection) * mModelViewProjection;
//			// Write model view projection matrix to uniform
//			{
//				Diligent::MapHelper<glm::mat4> CBConstants(mImmediateContext, mVSConstants,
//					Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
//				*CBConstants = glm::transpose(MVP);
//			}
//			{
//				Diligent::MapHelper<float> CBConstants(mImmediateContext, mPSConstants,
//					Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
//				*CBConstants = kernel->at(i);
//			}
//			mImmediateContext->DrawIndexed(DrawAttrs);
//		}
//	}
//	else {
//        for(int i = 0; i < 3600; i++) {
//            glm::mat4 MVP = mModelViewProjection * glm::translate(glm::vec3(i, 0, 0));
//            {
//                Diligent::MapHelper<glm::mat4> CBConstants(mImmediateContext, mVSConstants,
//                    Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
//                *CBConstants = glm::transpose(MVP);
//            }
//            {
//                Diligent::MapHelper<float> CBConstants(mImmediateContext, mPSConstants,
//                    Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
//                *CBConstants = 1.0f; // opacity
//            }
//            mImmediateContext->DrawIndexed(DrawAttrs);
//        }
//	}
    
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
    
    std::wstring FPS = L"FPS: " + converter.from_bytes(std::to_string(mLastFPS).c_str());
//    mTextRenderer->draw(mImmediateContext,
//        glm::translate(mViewProjection, glm::vec3(glm::vec2(10.0f, 10.0f), 0.0f)), FPS, 16.0f);
    
    bvgCtx.orthographic(mWidth, mHeight);
    bvgCtx.clearTransform();
    bvgCtx.contentScale = mScale;
    
    Diligent::SwapChainDesc SCDesc = mSwapChain->GetDesc();
    
    bvgCtx.setupPipelineStates(SCDesc.ColorBufferFormat, SCDesc.DepthBufferFormat, 2);
    bvgCtx.specifyTextureViews(renderTarget.RTV, renderTarget.DSV);
    
    bvgCtx.beginDrawing();
    
    bvgCtx.font = bvgCtx.fonts["roboto-regular"];
    
    float eyeX = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch)) * mZoom;
    float eyeY = sin(glm::radians(mPitch)) * mZoom;
    float eyeZ = cos(glm::radians(mYaw)) * cos(glm::radians(mPitch)) * mZoom;
    glm::vec3 eye = glm::vec3(eyeX, eyeY, eyeZ);
    
    float normalizedPitch = degreesInRange(mPitch);
    bool flipY = normalizedPitch < 90.0f || normalizedPitch >= 270.0f;
    glm::vec3 up = glm::vec3(0.0, flipY ? 1.0 : -1.0, 0.0);
    
    glm::mat4 view = glm::lookAt(eye, glm::vec3(0.0f), up);
    
    glm::mat4 vp = glm::perspective(45.0f, (float)mWidth / (float)mHeight, 0.01f, 100.0f) * view;
    
    switch(mDemoType) {
    case DemoType::HALF_EDGE:
    {
        bvg::Color primaryColor = bvg::Color(0.0f, 0.1f, 1.0f);
        bvg::Color primaryColorSemiTr = primaryColor;
        primaryColorSemiTr.a = 0.75f;
        bvg::Color primaryColorTr = primaryColor;
        primaryColorSemiTr.a = 0.5f;
        
        bvgCtx.clearTransform();
        bvgCtx.fontSize = 40.0f;
        bvgCtx.fillStyle = bvg::LinearGradient(0, 0, 0, bvgCtx.font->lineHeight,
                                               primaryColorTr,
                                               primaryColorSemiTr);
        bvgCtx.print(L"Half Edge Data Structure", 10, 40);
        
        bvgCtx.fillStyle = bvg::SolidColor(primaryColor);
        bvgCtx.fontSize = 16.0f;
        bvgCtx.print(heSummary, 10, 100);
        
        meshViewer.draw(bvgCtx, isMouseDown, mMouseX, mMouseY);
    }
        break;
    case DemoType::HALF_EDGE_3D:
    {
        editor->viewProj = vp;
        editor->view = view;
        editor->eye = eye;
        editor->draw(mImmediateContext);
    }
        break;
    case DemoType::GIZMOS:
    {
        bool blockMouseEvent = mIsControlPressed;
        
        if(mPlayRotation) {
            mTransform.rotation = glm::slerp(mTransform.rotation, glm::quat(glm::vec3(0.0f)),
                                             mDeltaTime * 3.0f);
        }
        
        gizmo::drawGizmos(bvgCtx, mGizmoState, mGizmoTool, mGizmoProps,
                          mTransform, vp, eye, glm::vec3(0.0f), up,
                          blockMouseEvent ? false : isMouseDown, mMouseX, mMouseY);
    }
        break;
    case DemoType::VECTOR_GRAPHICS:
    {
        bvgCtx.test();
        
        bvgCtx.beginClip();
        bvgCtx.translate(200, 100);
        bvgCtx.beginPath();
        bvgCtx.arc(0.0f, 0.0f, 60, 0.0f, M_PI * 2.0f);
        bvgCtx.fill();
        bvgCtx.endClip();
        
        bvgCtx.beginPath();
    //    bvgCtx.moveTo(-50, 50);
    //    bvgCtx.cubicTo(-50, 0, 50, 0, 50, 50);
    //    bvgCtx.lineTo(0, 100);
        bvgCtx.moveTo(-50, 0);
        bvgCtx.lineTo(50, 0);
        bvgCtx.lineTo(50, 100);
        bvgCtx.lineTo(0, 50);
        bvgCtx.lineTo(-50, 100);
        bvgCtx.lineTo(-50, 0);
        bvgCtx.closePath();

        bvgCtx.lineWidth = 8.0f;
        bvgCtx.lineDash = bvg::LineDash(12, 6);
        bvgCtx.lineDash.offset = sin(getTime()) * 80.0f;
        bvgCtx.lineJoin = bvg::LineJoin::Bevel;
        bvgCtx.lineCap = bvg::LineCap::Butt;
        bvgCtx.strokeStyle = bvg::LinearGradient(0, 0, 0, 100,
                                                 bvg::Color(0.3f, 0.3f, 1.0f, 0.5f),
                                                 bvg::Color(1.0f, 0.1f, 0.1f, 1.0f));
        bvgCtx.fillStyle = bvg::LinearGradient(-50, 20, 50, 100,
                                               bvg::colors::Black,
                                               bvg::colors::White);
        bvgCtx.clearTransform();
        bvgCtx.rotate(mRotation);
        bvgCtx.translate(200, 100);
        bvgCtx.fill();
        bvgCtx.stroke();
        bvgCtx.fillStyle = bvg::SolidColor(bvg::colors::White);
        bvgCtx.print(L"Привет мир!", 0, 0);
        bvgCtx.clearClip();

        bvgCtx.lineJoin = bvg::LineJoin::Miter;

        bvg::Color red = bvg::Color(1.0f, 0.2f, 0.2f);
        bvg::Color blue = bvg::Color(0.2f, 0.2f, 1.0f);

        float timeZeroOne = (sinf(getTime()) + 1.0f) / 2.0f;


        bvgCtx.clearTransform();
        bvgCtx.rotate(mRotation);
        bvgCtx.translate(400, 100);
        bvgCtx.fontSize = sinf(getTime()) / 2.0f * 40.0f + 40.0f;
        bvgCtx.fillStyle = bvg::LinearGradient(0, 0, 0, bvgCtx.font->lineHeight,
                                               red,
                                               blue);
        bvgCtx.print(L"Привет мир!", 0, 0);
        
        bvgCtx.clearTransform();
        bvgCtx.translate(200, 400);
        bvgCtx.beginPath();
        bvgCtx.moveTo(0, 0);
        bvgCtx.cubicTo(50, -100, 100, 0, 200, 0);
        bvgCtx.cubicTo(230, -50, 450, 50, 500, 0);
        // bvgCtx.closePath();
        bvgCtx.lineDash = bvg::LineDash(8, 4);
        bvgCtx.lineDash.dash = { 8, 4, 4, 4 };
        bvgCtx.lineDash.offset = sin(getTime()) * 80.0f;
        bvgCtx.fontSize = 32;

        float t = (sin(getTime()) + 1.0f) / 2.0f;
        bvgCtx.fillStyle = bvg::SolidColor(bvg::Color::lerp(red, blue, t));

        bvgCtx.printOnPath(L"Lorem ipsum dolor sit amet",
                              sin(getTime()) * 80.0f + 80.0f, -4.0f);
        bvgCtx.lineWidth = 2.0f;
        bvgCtx.strokeStyle = bvg::SolidColor(bvg::colors::White);
        bvgCtx.stroke();

        bvgCtx.clearTransform();
        bvgCtx.translate(600, 450);
        bvgCtx.beginPath();
        bvgCtx.lineTo(70, 100);
        bvgCtx.lineTo(-70, 100);
        bvgCtx.closePath();
        bvgCtx.lineDash = bvg::LineDash();
        bvgCtx.fillStyle = bvg::ConicGradient(0, 60, getTime() * 3.0f,
                                              red,
                                              blue);
        bvgCtx.convexFill();
        bvgCtx.lineWidth = 18;
        bvgCtx.strokeStyle = bvg::RadialGradient(0, 60, 140,
                                                 red,
                                                 blue);

        float time = getTime();
        float timeCycle = 3;
        float timeWithinCycle = time - floorf(time / timeCycle) * timeCycle;

        if(timeWithinCycle < 1.0f)
            bvgCtx.lineJoin = bvg::LineJoin::Miter;
        else if(timeWithinCycle < 2.0f)
            bvgCtx.lineJoin = bvg::LineJoin::Round;
        else
            bvgCtx.lineJoin = bvg::LineJoin::Bevel;

    //    bvgCtx.strokeStyle = bvg::SolidColor(bvg::Color(0.0f, 0.0f, 0.0f, 0.5f));
        bvgCtx.stroke();

        bvgCtx.clearTransform();
        float fastTime = getTime() * 3.0f;
        bvgCtx.rotate(fastTime);
        bvgCtx.translate(400, 500);
        bvgCtx.beginPath();
        bvgCtx.arc(0.0f, 0.0f, 25.0f, -0.7f, M_PI * 1.5f);
        bvgCtx.lineDash = bvg::LineDash();
        bvgCtx.lineWidth = 8;
        bvgCtx.lineCap = bvg::LineCap::Round;
        bvg::Color blueTransparent = bvg::Color(0.2f, 0.2f, 1.0f, 0.0f);
        bvgCtx.strokeStyle = bvg::ConicGradient(0, 0, M_PI_2 + 0.3f,
                                                red,
                                                blueTransparent);
        bvgCtx.stroke();

        bvgCtx.clearTransform();
        bvgCtx.translate(250, 500);
        bvgCtx.beginPath();
        bvgCtx.arc(0.0f, 0.0f, 50.0f, -2.0f, 3.0f);
        bvgCtx.lineTo(0, 0);
        bvgCtx.closePath();
        bvgCtx.fillStyle = bvg::SolidColor(bvg::Color(1.0f, 0.7f, 0.1f));
        bvgCtx.strokeStyle = bvg::SolidColor(bvg::Color(1.0f, 0.2f, 0.1f));
        bvgCtx.lineWidth = 4;
        bvgCtx.fill();
        bvgCtx.stroke();

        bvgCtx.clearTransform();
        bvgCtx.translate(30, 400);
        bvgCtx.beginPath();
        bvgCtx.rect(0, 0, 100, 60, 12);
        bvgCtx.fillStyle = bvg::SolidColor(bvg::Color(0.1f, 0.8f, 0.1f));
        bvgCtx.strokeStyle = bvg::SolidColor(bvg::Color(0.0f, 0.5f, 0.0f));
        // bvgCtx.lineDash = bvg::LineDash(8, 4);
        bvgCtx.convexFill();
        bvgCtx.stroke();
        
//        bvgCtx.clearTransform();
//        bvgCtx.rotate(mRotation);
//        bvgCtx.translate(0, 0);
//        bvgCtx.beginPath();
//        bvgCtx.rect(0, 0, 800, 600);
//        bvgCtx.fillStyle = bvg::BoxShadow(100, 100, 200, 100, 16, 8, bvg::Color(1.0f, 0.1f, 0.1f));
//        bvgCtx.convexFill();
        
        bvgCtx.clearTransform();
        bvgCtx.rotate(mRotation);
        bvgCtx.translate(25, 25);
        bvgCtx.beginPath();
        bvgCtx.rect(0, 0, 200, 100);
        bvgCtx.fillStyle = bvg::BoxShadow(12, 12, 100, 50, 16, 8, bvg::Color(1.0f, 0.1f, 0.1f));
        bvgCtx.convexFill();
    }
        break;
    case DemoType::UI:
    {
        ctx.vg.fontSize = 16;
        ctx.vg.fillStyle = bvg::SolidColor(bvg::colors::Black);
        
        beginPass(ctx, ContextPass::LAYOUT);
        {
            doUI();
        }
        endPass(ctx);
        
        beginPass(ctx, ContextPass::DRAW);
        {
            doUI();
        }
        endPass(ctx);
    }
        break;
    }
    
    bvgCtx.clearTransform();
    bvgCtx.translate(12, 12);
    bvgCtx.fontSize = 16;
    bvgCtx.fillStyle = bvg::SolidColor(bvg::colors::Black);
    bvgCtx.print(FPS, 0, 0);
    
    bvgCtx.endDrawing();
     
    Diligent::ResolveTextureSubresourceAttribs ResolveAttribs;
    ResolveAttribs.SrcTextureTransitionMode =
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    ResolveAttribs.DstTextureTransitionMode =
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    Diligent::TextureDesc TDesc = pRTV->GetTexture()->GetDesc();
    if(TDesc.Width != mWidth * mScale || TDesc.Height != mHeight * mScale)
        return;
    mImmediateContext->ResolveTextureSubresource(renderTarget.color, pRTV->GetTexture(),
                                                 ResolveAttribs);
    
	mSwapChain->Present();
    
//    Diligent::StateTransitionDesc STDesc;
//    STDesc.pResource = renderTarget.color;
//    STDesc.OldState = Diligent::RESOURCE_STATE_SHADER_RESOURCE;
//    STDesc.NewState = Diligent::RESOURCE_STATE_RENDER_TARGET;
//    mImmediateContext->TransitionResourceStates(1, &STDesc);
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
    
//    mLastRaycasts = ctx.NumRaycasts;
//    ctx.NumRaycasts = 0;

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
    recreateRenderTargets();
	mSwapChain->Resize(width, height);
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
    mScale = getViewScale(window.pNSView);

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
    PSOCreateInfo.GraphicsPipeline.SmplDesc.Count = 2;
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

//	mShapeRenderer = std::make_shared<ShapeRenderer>(ShapeRenderer(mDevice, mSwapChain));

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
    
    bvgCtx = bvg::DiligentContext(mWidth, mHeight, mDevice,
                                  mImmediateContext,
                                  mSwapChain->GetDesc().ColorBufferFormat,
                                  mSwapChain->GetDesc().DepthBufferFormat,
                                  2);
    
    std::string fontJson = LoadTextResource("Roboto/Roboto-Regular.json");
    
    {
        int width, height, numChannels;
        unsigned char* imageData = stbi_load("assets/Roboto/Roboto-Regular.png",
                                             &width, &height, &numChannels, 0);
        bvgCtx.loadFontFromMemory(fontJson, "roboto-regular", imageData, width, height, numChannels);
        stbi_image_free(imageData);
    }
    
    recreateRenderTargets();
    
    // Halfedge 3D editor
    
    model = createCubeModel();
    model.originalMesh.status(*model.originalMesh.edges_begin()).set_selected(true);
    model.originalMesh.status(*model.originalMesh.faces_begin()).set_selected(true);
    model.isFlatShaded = true;
    {
        int width, height, numChannels;
        unsigned char* imageData = stbi_load("assets/matcap.png",
                                             &width, &height, &numChannels, 0);
        
        Texture matcap = Texture(mDevice, Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB,
                                 imageData, width, height, numChannels);
        RendererCreateOptions options;
        options.sampleCount = 2;
        stbi_image_free(imageData);
        editor = std::make_shared<Editor>(mDevice, mSwapChain, matcap, options);
        editor->model = &model;
        model.invalidate(mDevice, mImmediateContext);
    }
}

void App::recreateRenderTargets() {
    Diligent::SwapChainDesc SCDesc = mSwapChain->GetDesc();
    
    renderTarget = RenderTarget(mDevice, SCDesc.ColorBufferFormat,
                                SCDesc.DepthBufferFormat, mWidth * mScale, mHeight * mScale, 2);
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
//	mFont = ui::LoadFont(mDevice, fontFnt, atlases);
//	mTextRenderer = std::make_shared<TextRenderer>(TextRenderer(mDevice, mSwapChain, mFont));
}

float App::getTime()
{
	return (float)SDL_GetTicks() / 1000.0f;
}

App::~App()
{
}

void App::doUI() {
    beginAffine(ctx, glm::translate(glm::mat3(1.0f), glm::vec2(80)));
    {
        elements::Label(ctx, L"Hello world!");
    }
    endAffine(ctx);
}
