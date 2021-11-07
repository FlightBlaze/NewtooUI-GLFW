#include <TextRenderer.h>
#include "Graphics/GraphicsTools/interface/CommonlyUsedStates.h"
#include "Graphics/GraphicsTools/interface/MapHelper.hpp"
#include <glm/gtx/transform.hpp>

TextRenderer::TextRenderer(
	Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice,
	Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapChain,
	std::shared_ptr<ui::Font> font) :
	font(font)
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
		ShaderCI.Desc.Name = "Glyph vertex shader";
		ShaderCI.Source = GlyphVSSource;
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
		ShaderCI.Desc.Name = "Glyph pixel shader";
		ShaderCI.Source = GlyphPSSource;
		renderDevice->CreateShader(ShaderCI, &pPS);

		Diligent::BufferDesc CBDesc;
		CBDesc.Name = "PS constants CB";
		CBDesc.Size = sizeof(GlyphPSConstants);
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
		Diligent::LayoutElement{0, 0, 2, Diligent::VT_FLOAT32, Diligent::False},
		// Attribute 1 - texture coordinate
		Diligent::LayoutElement{1, 0, 2, Diligent::VT_FLOAT32, Diligent::False}
	};
	PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
	PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements = _countof(LayoutElems);
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

	renderDevice->CreateGraphicsPipelineState(PSOCreateInfo, &mPSO);

	mPSO->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "Constants")->Set(mVSConstants);
	mPSO->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "Constants")->Set(mPSConstants);

	mPageSRBs.resize(font->pages.size());
	for (int i = 0; i < mPageSRBs.size(); i++) {
		mPSO->CreateShaderResourceBinding(&mPageSRBs[i], true);
		mPageSRBs[i]->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture")->
			Set(font->pages[i].atlas->textureSRV);
	}

	Diligent::BufferDesc IndBuffDesc;
	IndBuffDesc.Name = "Gliph quad index buffer";
	IndBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
	IndBuffDesc.BindFlags = Diligent::BIND_INDEX_BUFFER;
	IndBuffDesc.Size = sizeof(GlyphQuadIndices);
	Diligent::BufferData IBData;
	IBData.pData = GlyphQuadIndices;
	IBData.DataSize = sizeof(GlyphQuadIndices);
	renderDevice->CreateBuffer(IndBuffDesc, &IBData, &mQuadIndexBuffer);
}

float TextRenderer::getScale(float sizePx) {
    return sizePx / (float)font->lineHeight;
}

void TextRenderer::draw(
	Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context,
	glm::mat4 modelViewProjection,
	const std::wstring& text,
	float sizePx,
    glm::vec3 color,
	float opacity)
{
	static const float smallSizePx = 32.0f;
	static const float maxExtraDist = 0.0f; // 5.0f
	float extraDistance = fmaxf((smallSizePx - sizePx) / sizePx * maxExtraDist, 0);

	float scale = getScale(sizePx);
	glm::vec2 pos = glm::vec2(0.0f, (float)font->baseline * scale);
	for (int i = 0; i < text.size(); i++)
	{
		int symbol = text[i];
		if (symbol == '\n')
		{
			pos.y += font->lineHeight * scale;
			continue;
		}

		ui::FontCharacter& character = this->font->characters[symbol];

		if (symbol == ' ')
		{
			pos.x += character.xAdvance * scale;
			continue;
		}

		glm::mat4 MVP = modelViewProjection * glm::scale(
			glm::translate(glm::identity<glm::mat4>(), glm::vec3(pos, 0.0f)),
			glm::vec3(scale));
		{
			Diligent::MapHelper<glm::mat4> CBConstants(context, mVSConstants,
				Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
			*CBConstants = glm::transpose(MVP);
		}
		{
			Diligent::MapHelper<GlyphPSConstants> CBConstants(context, mPSConstants,
				Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
			GlyphPSConstants constants;
			constants.color = color;
			constants.opacity = opacity;
			constants.distanceRange = (float)font->distanceRange + extraDistance;
			constants.atlasSize = glm::vec2(font->atlasWidth, font->atlasHeight);
			*CBConstants = constants;
		}

		Diligent::Uint64   offset = 0;
		Diligent::IBuffer* pBuffs[] = { character.vertexBuffer };
		context->SetVertexBuffers(0, 1, pBuffs, &offset,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
			Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
		context->SetIndexBuffer(mQuadIndexBuffer, 0,
			Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		context->SetPipelineState(mPSO);

		auto SRB = mPageSRBs[character.page];
		context->CommitShaderResources(SRB, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

		Diligent::DrawIndexedAttribs DrawAttrs;
		DrawAttrs.IndexType = Diligent::VT_UINT32;
		DrawAttrs.NumIndices = _countof(GlyphQuadIndices);
		DrawAttrs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;

		context->DrawIndexed(DrawAttrs);

		pos.x += character.xAdvance * scale;
	}
}

float TextRenderer::measureWidth(const std::wstring& text, float sizePx) {
    float width = 0.0f;
    float scale = getScale(sizePx);
    for (int i = 0; i < text.size(); i++)
    {
        int symbol = text[i];
        if (symbol == '\n')
            break;
        
        ui::FontCharacter& character = this->font->characters[symbol];
        width += character.xAdvance * scale;
    }
    return width;
}

float TextRenderer::measureHeight(float sizePx) {
    return this->font->lineHeight * getScale(sizePx);
}
