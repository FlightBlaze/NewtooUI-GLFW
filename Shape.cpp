#include <Shape.h>
#include "Graphics/GraphicsTools/interface/CommonlyUsedStates.h"
#include "Graphics/GraphicsTools/interface/MapHelper.hpp"
#include <glm/gtx/transform.hpp>
#include <MathExtras.h>
#include <cmath>

#include <App.h>

std::vector<int> CreateIndicesConvex(size_t numVertices) {
	size_t amount = (numVertices - 2) * 3;
	auto indices = std::vector<int>(amount);
	int k = 1;
	for (size_t i = 0; i < amount; i += 3)
	{
		indices[i] = 0;

		// Connect current edge with next
		indices[i + 1] = k;
		indices[i + 2] = k + 1;
		k++;
	}
	return indices;
}

MinMax2 CalculateBoundingBox2D(std::vector<glm::vec3>& verts) {
	MinMax2 bounds;
	const static float BigValue = 10000.0f;
	bounds.minX = BigValue;
	bounds.maxX = -BigValue;
	bounds.minY = BigValue;
	bounds.maxY = -BigValue;
	for (auto& vert : verts) {
		bounds.minX = fminf(bounds.minX, vert.x);
		bounds.maxX = fmaxf(bounds.maxX, vert.x);
		bounds.minY = fminf(bounds.minY, vert.y);
		bounds.maxY = fmaxf(bounds.maxY, vert.y);
	}
	return bounds;
}

// Generates UV for vertices projected on XY plane
std::vector<glm::vec2> GenerateUV(MinMax2 bounds, std::vector<glm::vec3> verts) {
	std::vector<glm::vec2> UV;
	UV.resize(verts.size());
	for (size_t i = 0; i < verts.size(); i++) {
		glm::vec3& vert = verts[i];
		UV[i] = glm::vec2(
			remapf(bounds.minX, bounds.maxX, 0.0f, 1.0f, vert.x),
			remapf(bounds.minX, bounds.maxX, 0.0f, 1.0f, vert.y));
	}
	return UV;
}

Fill CreateFill(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice,
	tube::Path path, bool isConvex)
{
	Fill fill;
	tube::Shape shape = path.toShape();
	MinMax2 bounds = CalculateBoundingBox2D(shape.verts);
	fill.width = bounds.maxX - bounds.minX;
	fill.height = bounds.maxY - bounds.minY;
	fill.offset = glm::vec2(-bounds.minX + fill.width / 2.0f, -bounds.minY + fill.height / 2.0f);
	std::vector<int> indices = CreateIndicesConvex(shape.verts.size());
	std::vector<glm::vec2> UV = GenerateUV(bounds, shape.verts);
	std::vector<ShapeVertex> vertices;
	vertices.resize(shape.verts.size());
	for (int i = 0; i < vertices.size(); i++) {
		vertices[i].position = shape.verts[i];
		vertices[i].texCoord = UV[i];
	}

	Diligent::BufferDesc VertBuffDesc;
	VertBuffDesc.Name = "Shape vertex buffer";
	VertBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
	VertBuffDesc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
	VertBuffDesc.Size = vertices.size() * sizeof(ShapeVertex);
	Diligent::BufferData VBData;
	VBData.pData = vertices.data();
	VBData.DataSize = vertices.size() * sizeof(ShapeVertex);
	renderDevice->CreateBuffer(VertBuffDesc, &VBData, &fill.vertexBuffer);

	Diligent::BufferDesc IndBuffDesc;
	IndBuffDesc.Name = "Shape index buffer";
	IndBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
	IndBuffDesc.BindFlags = Diligent::BIND_INDEX_BUFFER;
	IndBuffDesc.Size = indices.size() * sizeof(int);
	Diligent::BufferData IBData;
	IBData.pData = indices.data();
	IBData.DataSize = indices.size() * sizeof(int);
	renderDevice->CreateBuffer(IndBuffDesc, &IBData, &fill.indexBuffer);

	fill.vertices = vertices;
	fill.indices = indices;
	fill.bounds = bounds;
	fill.numIndices = (int)indices.size();
	return fill;
}

SolidFillRenderer::SolidFillRenderer(
	Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice,
	Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapChain)
{
	Diligent::GraphicsPipelineStateCreateInfo PSOCreateInfo;
	PSOCreateInfo.PSODesc.Name = "Text PSO";
	PSOCreateInfo.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
	PSOCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
	PSOCreateInfo.GraphicsPipeline.RTVFormats[0] = swapChain->GetDesc().ColorBufferFormat;
	PSOCreateInfo.GraphicsPipeline.DSVFormat = swapChain->GetDesc().DepthBufferFormat;
	PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
	PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = Diligent::False;

	Diligent::BlendStateDesc BlendState;
	BlendState.RenderTargets[0].BlendEnable = Diligent::True;
	BlendState.RenderTargets[0].SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
	BlendState.RenderTargets[0].DestBlend = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
	PSOCreateInfo.GraphicsPipeline.BlendDesc = BlendState;

	Diligent::ShaderCreateInfo ShaderCI;
	ShaderCI.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
	ShaderCI.UseCombinedTextureSamplers = Diligent::True;

	// Create a vertex shader
	Diligent::RefCntAutoPtr<Diligent::IShader> pVS;
	{
		ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
		ShaderCI.EntryPoint = "main";
		ShaderCI.Desc.Name = "Solid fill vertex shader";
		ShaderCI.Source = ShapeVSSource;
		renderDevice->CreateShader(ShaderCI, &pVS);

		Diligent::BufferDesc CBDesc;
		CBDesc.Name = "VS constants CB";
		CBDesc.Size = sizeof(glm::mat4);
		CBDesc.Usage = Diligent::USAGE_DYNAMIC;
		CBDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
		CBDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
		renderDevice->CreateBuffer(CBDesc, nullptr, &mVSConstants);
	}

	// Create a pixel shader
	Diligent::RefCntAutoPtr<Diligent::IShader> pPS;
	{
		ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
		ShaderCI.EntryPoint = "main";
		ShaderCI.Desc.Name = "Solid fill pixel shader";
		ShaderCI.Source = SolidFillPSSource;
		renderDevice->CreateShader(ShaderCI, &pPS);

		Diligent::BufferDesc CBDesc;
		CBDesc.Name = "PS constants CB";
		CBDesc.Size = sizeof(SolidFillPSConstants);
		CBDesc.Usage = Diligent::USAGE_DYNAMIC;
		CBDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
		CBDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
		renderDevice->CreateBuffer(CBDesc, nullptr, &mPSConstants);
	}

	PSOCreateInfo.pVS = pVS;
	PSOCreateInfo.pPS = pPS;

	Diligent::LayoutElement LayoutElems[] =
	{
		// Attribute 0 - vertex position
		Diligent::LayoutElement{0, 0, 3, Diligent::VT_FLOAT32, Diligent::False},
		// Attribute 1 - texture coordinate
		Diligent::LayoutElement{1, 0, 2, Diligent::VT_FLOAT32, Diligent::False}
	};
	PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
	PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements = _countof(LayoutElems);
	PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

	renderDevice->CreateGraphicsPipelineState(PSOCreateInfo, &mPSO);

	mPSO->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "Constants")->Set(mVSConstants);
	mPSO->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "Constants")->Set(mPSConstants);

	mPSO->CreateShaderResourceBinding(&mSRB, true);
}

void SolidFillRenderer::draw(
	Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context,
	glm::mat4 modelViewProjection, Fill& shape, glm::vec3 color, float opacity)
{
	{
		Diligent::MapHelper<glm::mat4> CBConstants(context, mVSConstants,
			Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
		*CBConstants = glm::transpose(modelViewProjection);
	}
	{
		Diligent::MapHelper<SolidFillPSConstants> CBConstants(context, mPSConstants,
			Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
		SolidFillPSConstants constants;
		constants.color = color;
		constants.opacity = opacity;
		*CBConstants = constants;
	}

	Diligent::Uint64   offset = 0;
	Diligent::IBuffer* pBuffs[] = { shape.vertexBuffer };
	context->SetVertexBuffers(0, 1, pBuffs, &offset,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
		Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
	context->SetIndexBuffer(shape.indexBuffer, 0,
		Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	context->SetPipelineState(mPSO);

	context->CommitShaderResources(mSRB, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

	Diligent::DrawIndexedAttribs DrawAttrs;
	DrawAttrs.IndexType = Diligent::VT_UINT32;
	DrawAttrs.NumIndices = shape.numIndices;
	DrawAttrs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;

	context->DrawIndexed(DrawAttrs);
}

bool Fill::containsPoint(glm::vec2 point)
{
	for (size_t i = 0; i < this->indices.size(); i += 3) {
		glm::vec2 a = glm::vec2(this->vertices[this->indices[i + 0LL]].position);
		glm::vec2 b = glm::vec2(this->vertices[this->indices[i + 1LL]].position);
		glm::vec2 c = glm::vec2(this->vertices[this->indices[i + 2LL]].position);
		if (isPointInTriange(a, b, c, point))
			return true;
	}
	return false;
}
