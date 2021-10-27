#pragma once

#ifdef __APPLE__
#define PLATFORM_MACOS 1
#else 
#define PLATFORM_WIN32 1
#endif
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Common/interface/RefCntAutoPtr.hpp"

struct ModifyEngineInitInfoAttribs
{
    const Diligent::RENDER_DEVICE_TYPE DeviceType;
    const Diligent::IEngineFactory* Factory;
    Diligent::EngineCreateInfo& EngineCI;
    Diligent::SwapChainDesc& SCDesc;
};

void ModifyEngineInitInfo(const ModifyEngineInitInfoAttribs& Attribs);
