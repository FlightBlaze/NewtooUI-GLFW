#include <Diligent.h>

void ModifyEngineInitInfo(const ModifyEngineInitInfoAttribs& Attribs)
{
	Attribs.EngineCI.Features = Diligent::DeviceFeatures{ Diligent::DEVICE_FEATURE_STATE_OPTIONAL };
	Attribs.EngineCI.Features.TransferQueueTimestampQueries = Diligent::DEVICE_FEATURE_STATE_DISABLED;
	// Attribs.SCDesc.DepthBufferFormat = Diligent::TEX_FORMAT_UNKNOWN;

	// TODO modify Direct3D12 engine init info with switch case
}
