#include <RenderTarget.h>

#include "Graphics/GraphicsTools/interface/CommonlyUsedStates.h"

RenderTarget::RenderTarget(RenderTargetCreateInfo& createInfo)
{
	// Create vertex shader
	Diligent::ShaderCreateInfo ShaderCI;
	ShaderCI.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
	ShaderCI.UseCombinedTextureSamplers = Diligent::True;
	{
		ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
		ShaderCI.EntryPoint = "main";
		ShaderCI.Desc.Name = "Render target vertex shader";
		ShaderCI.Source = RenderTargetVSSource;
		createInfo.device->CreateShader(ShaderCI, &mVertexShader);
	}

	recreatePipelineState(createInfo);
	recreateTextures(createInfo);
}

void RenderTarget::recreatePipelineState(RenderTargetCreateInfo& createInfo)
{
	Diligent::GraphicsPipelineStateCreateInfo PSOCreateInfo;

	PSOCreateInfo.PSODesc.Name = "Render Target PSO";
	PSOCreateInfo.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
	PSOCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
	PSOCreateInfo.GraphicsPipeline.RTVFormats[0] = createInfo.swapChain->GetDesc().ColorBufferFormat;
	PSOCreateInfo.GraphicsPipeline.DSVFormat = createInfo.swapChain->GetDesc().DepthBufferFormat;
	PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_BACK;
	PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = Diligent::False;
	PSOCreateInfo.pVS = mVertexShader;
	PSOCreateInfo.pPS = createInfo.pixelShader;

	PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	Diligent::ShaderResourceVariableDesc variables[] =
	{
		{ Diligent::SHADER_TYPE_PIXEL, "g_Texture", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE }
	};
	PSOCreateInfo.PSODesc.ResourceLayout.Variables = variables;
	PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(variables);

	Diligent::ImmutableSamplerDesc samplers[] =
	{
		{ Diligent::SHADER_TYPE_PIXEL, "g_Texture", Diligent::Sam_LinearClamp }
	};
	PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers = samplers;
	PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(samplers);

	createInfo.device->CreateGraphicsPipelineState(PSOCreateInfo, &PSO);

	if (createInfo.uniformBuffer != nullptr) {
		uniformBuffer = createInfo.uniformBuffer;
		PSO->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "Constants")->Set(uniformBuffer);
	}
}

void RenderTarget::recreateTextures(RenderTargetCreateInfo& createInfo) {
	Diligent::TextureDesc colorDesc;
	colorDesc.Type = Diligent::RESOURCE_DIM_TEX_2D;
	colorDesc.Width = createInfo.width;
	colorDesc.Height = createInfo.height;
	colorDesc.MipLevels = 1;
	colorDesc.Format = RenderTargetFormat;
	colorDesc.BindFlags = Diligent::BIND_SHADER_RESOURCE | Diligent::BIND_RENDER_TARGET;
	createInfo.device->CreateTexture(colorDesc, nullptr, &color);
	colorRTV = color->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);

	Diligent::TextureDesc depthDesc = colorDesc;
	depthDesc.Format = DepthBufferFormat;
	depthDesc.BindFlags = Diligent::BIND_SHADER_RESOURCE | Diligent::BIND_DEPTH_STENCIL;
	createInfo.device->CreateTexture(depthDesc, nullptr, &depth);
	depthDSV = depth->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);

	PSO->CreateShaderResourceBinding(&SRB, true);
	SRB->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture")->
		Set(color->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE));
}

void RenderTarget::use(Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context)
{
	context->SetRenderTargets(1, &colorRTV, depthDSV,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void RenderTarget::clear(Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context,
	const float color[4])
{
	context->ClearRenderTarget(colorRTV, color,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
	context->ClearDepthStencil(depthDSV, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void RenderTarget::draw(Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context)
{
	context->SetPipelineState(PSO);
	context->CommitShaderResources(SRB, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::DrawAttribs drawAttrs;
	drawAttrs.NumVertices = 4;
	drawAttrs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
	context->Draw(drawAttrs);
}
