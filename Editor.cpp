//
//  Editor.cpp
//  MyProject
//
//  Created by Dmitry on 21.05.2022.
//

#include "Editor.hpp"
#include "Graphics/GraphicsTools/interface/CommonlyUsedStates.h"
#include "Graphics/GraphicsTools/interface/MapHelper.hpp"

void* convertRGBToRGBA(char* imageData,
                       int width,
                       int height)
{
    char* newImage = new char[width * height * 4];
    for(size_t i = 0; i < width * height; i++) {
        size_t oldIndex = i * 3LL;
        size_t newIndex = i * 4LL;
        newImage[newIndex] = imageData[oldIndex]; // Red
        newImage[newIndex + 1LL] = imageData[oldIndex + 1LL]; // Green
        newImage[newIndex + 2LL] = imageData[oldIndex + 2LL]; // Blue
        newImage[newIndex + 3LL] = 255; // Alpha
    }
    return newImage;
}

Texture::Texture(DgRenderDevice renderDevice,
                 Diligent::TEXTURE_FORMAT colorBufferFormat,
                     void* imageData,
                     int width,
                     int height,
                     int numChannels)
{
    assert(imageData != nullptr);
    
    int RGBAStride = width * numChannels;
    if(numChannels == 3) {
        RGBAStride = width * 4;
        imageData = convertRGBToRGBA((char*)imageData, width, height);
    }

    Diligent::TextureSubResData SubRes;
    SubRes.Stride = RGBAStride;
    SubRes.pData = imageData;

    Diligent::TextureData TexData;
    TexData.NumSubresources = 1;
    TexData.pSubResources = &SubRes;

    Diligent::TextureDesc TexDesc;
    TexDesc.Type = Diligent::RESOURCE_DIM_TEX_2D;
    TexDesc.Width = width;
    TexDesc.Height = height;
    TexDesc.MipLevels = 1;
    TexDesc.Format = colorBufferFormat;
    TexDesc.BindFlags = Diligent::BIND_SHADER_RESOURCE;
    
    renderDevice->CreateTexture(TexDesc, &TexData, &texture);
    textureSRV = texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
    
    if(numChannels == 3) {
        delete[] (char*)imageData;
    }
}

Texture::Texture() {}

Model::Model() {
    originalMesh.request_face_status();
    originalMesh.request_edge_status();
    originalMesh.request_halfedge_status();
    originalMesh.request_vertex_status();
    originalMesh.request_halfedge_texcoords2D();
    originalMesh.request_vertex_texcoords2D();
    originalMesh.request_vertex_normals();
    originalMesh.request_vertex_colors();
    originalMesh.request_face_normals();
}

Model createCubeModel() {
    Model model;
    PolyMesh& mesh = model.originalMesh;
    
    PolyMesh::VertexHandle vhandle[8];

    vhandle[0] = mesh.add_vertex(PolyMesh::Point(-1, -1,  1));
    vhandle[1] = mesh.add_vertex(PolyMesh::Point( 1, -1,  1));
    vhandle[2] = mesh.add_vertex(PolyMesh::Point( 1,  1,  1));
    vhandle[3] = mesh.add_vertex(PolyMesh::Point(-1,  1,  1));
    vhandle[4] = mesh.add_vertex(PolyMesh::Point(-1, -1, -1));
    vhandle[5] = mesh.add_vertex(PolyMesh::Point( 1, -1, -1));
    vhandle[6] = mesh.add_vertex(PolyMesh::Point( 1,  1, -1));
    vhandle[7] = mesh.add_vertex(PolyMesh::Point(-1,  1, -1));

    std::vector<PolyMesh::VertexHandle>  face_vhandles;

    face_vhandles.clear();
    face_vhandles.push_back(vhandle[0]);
    face_vhandles.push_back(vhandle[1]);
    face_vhandles.push_back(vhandle[2]);
    face_vhandles.push_back(vhandle[3]);
    mesh.add_face(face_vhandles);

    face_vhandles.clear();
    face_vhandles.push_back(vhandle[7]);
    face_vhandles.push_back(vhandle[6]);
    face_vhandles.push_back(vhandle[5]);
    face_vhandles.push_back(vhandle[4]);
    mesh.add_face(face_vhandles);

    face_vhandles.clear();
    face_vhandles.push_back(vhandle[1]);
    face_vhandles.push_back(vhandle[0]);
    face_vhandles.push_back(vhandle[4]);
    face_vhandles.push_back(vhandle[5]);
    mesh.add_face(face_vhandles);

    face_vhandles.clear();
    face_vhandles.push_back(vhandle[2]);
    face_vhandles.push_back(vhandle[1]);
    face_vhandles.push_back(vhandle[5]);
    face_vhandles.push_back(vhandle[6]);
    mesh.add_face(face_vhandles);

    face_vhandles.clear();
    face_vhandles.push_back(vhandle[3]);
    face_vhandles.push_back(vhandle[2]);
    face_vhandles.push_back(vhandle[6]);
    face_vhandles.push_back(vhandle[7]);
    mesh.add_face(face_vhandles);

    face_vhandles.clear();
    face_vhandles.push_back(vhandle[0]);
    face_vhandles.push_back(vhandle[3]);
    face_vhandles.push_back(vhandle[7]);
    face_vhandles.push_back(vhandle[4]);
    mesh.add_face(face_vhandles);
    
    mesh.update_normals();
    
    return model;
}

void Model::invalidate(DgRenderDevice renderDevice) {
    if(!isFlatShaded) {
        renderMesh = originalMesh;
        auto rvhIt = renderMesh.vertices_begin();
        for(auto ovh : originalMesh.vertices()) {
            originalToRenderVerts[ovh] = *rvhIt;
            rvhIt++;
        }
    } else {
        renderMesh.clear();
        renderMesh.request_vertex_texcoords2D();
        renderMesh.request_vertex_normals();
        renderMesh.request_vertex_colors();
        renderMesh.request_face_normals();
        for(auto fh : originalMesh.faces()) {
            std::vector<PolyMesh::VertexHandle> faceVerts;
            for(auto vhOriginal : originalMesh.fv_cw_range(fh)) {
                PolyMesh::VertexHandle vhRender = renderMesh.add_vertex(originalMesh.point(vhOriginal));
                renderMesh.set_normal(vhRender, originalMesh.normal(vhOriginal));
                renderMesh.set_texcoord2D(vhRender, originalMesh.texcoord2D(vhOriginal));
                renderMesh.set_color(vhRender, originalMesh.color(vhOriginal));
                faceVerts.push_back(vhRender);
                originalToRenderVerts[vhOriginal] = vhRender;
            }
            renderMesh.add_face(faceVerts);
        }
        renderMesh.request_face_normals();
        renderMesh.update_normals();
        renderMesh.release_face_normals();
    }
    renderMesh.triangulate();
    
    if(renderDevice != nullptr) {
        populateRenderBuffers(renderDevice);
    }
}

glm::vec3 vec3FromPoint(PolyMesh::Point p) {
    return glm::vec3(p[0], p[1], p[2]);
}

glm::vec2 vec2FromTexCoord2D(PolyMesh::TexCoord2D co) {
    return glm::vec2(co[0], co[1]);
}

glm::vec4 vec4FromColor(PolyMesh::Color col) {
    return glm::vec4(col[0], col[1], col[2], 1.0f);
}

void Model::populateRenderBuffers(DgRenderDevice renderDevice) {
    auto indexProp = OpenMesh::getOrMakeProperty<PolyMesh::VertexHandle, int>(renderMesh, "index");
    int index = 0;
    for(auto vh : renderMesh.vertices()) {
        indexProp[vh] = index;
        index++;
    }
    
    int vertsSize = index;
    std::vector<RenderVertex> verts;
    verts.reserve(vertsSize);
    for(auto vh : renderMesh.vertices()) {
        RenderVertex vert;
        vert.pos = vec3FromPoint(renderMesh.point(vh));
        vert.normal = vec3FromPoint(renderMesh.normal(vh));
        vert.UV = vec2FromTexCoord2D(renderMesh.texcoord2D(vh));
        vert.color = vec4FromColor(renderMesh.color(vh));
        verts.push_back(vert);
    }
    
    int trisSize = 0;
    for(auto fh : renderMesh.faces()) {
        trisSize++;
    }
    std::vector<RenderTriange> tris;
    verts.reserve(trisSize);
    for(auto fh : renderMesh.faces()) {
        RenderTriange tri;
        int fvInd = 0;
        for(auto fvh : renderMesh.fv_cw_range(fh)) {
            int vInd = indexProp[fvh];
            switch(fvInd) {
                case 0:
                    tri.a = vInd;
                    break;
                case 1:
                    tri.b = vInd;
                    break;
                case 2:
                    tri.c = vInd;
                    break;
            }
            fvInd++;
        }
        tris.push_back(tri);
    }
    
    int linesSize = 0;
    for(auto vh : originalMesh.edges()) {
        linesSize++;
    }
    std::vector<RenderLine> lines;
    lines.reserve(linesSize);
    for(auto eh : originalMesh.edges()) {
        RenderLine line;
        line.a = indexProp[originalToRenderVerts[eh.v0()]];
        line.b = indexProp[originalToRenderVerts[eh.v1()]];
        lines.push_back(line);
    }
    
    Diligent::BufferDesc VertBuffDesc;
    VertBuffDesc.Name = "Vertex buffer";
    VertBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
    VertBuffDesc.Size = verts.size() * sizeof(RenderVertex);
    Diligent::BufferData VBData;
    VBData.pData = verts.data();
    VBData.DataSize = verts.size() * sizeof(RenderVertex);
    renderDevice->CreateBuffer(VertBuffDesc, &VBData, &vertexBuffer);

    Diligent::BufferDesc TriBuffDesc;
    TriBuffDesc.Name = "Triange index buffer";
    TriBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
    TriBuffDesc.BindFlags = Diligent::BIND_INDEX_BUFFER;
//    TriBuffDesc.CPUAccessFlags = Diligent::CPU_ACCESS_FLAGS::CPU_ACCESS_WRITE;
    TriBuffDesc.Size = tris.size() * sizeof(RenderTriange);
    Diligent::BufferData TBData;
    TBData.pData = tris.data();
    TBData.DataSize = tris.size() * sizeof(RenderTriange);
    renderDevice->CreateBuffer(TriBuffDesc, &TBData, &triangleBuffer);
    
    Diligent::BufferDesc LineBuffDesc;
    LineBuffDesc.Name = "Line index buffer";
    LineBuffDesc.Usage = Diligent::USAGE_IMMUTABLE; 
    LineBuffDesc.BindFlags = Diligent::BIND_INDEX_BUFFER;
//    LineBuffDesc.CPUAccessFlags = Diligent::CPU_ACCESS_FLAGS::CPU_ACCESS_WRITE;
    LineBuffDesc.Size = lines.size() * sizeof(RenderLine);
    Diligent::BufferData LBData;
    LBData.pData = lines.data();
    LBData.DataSize = lines.size() * sizeof(RenderLine);
    renderDevice->CreateBuffer(LineBuffDesc, &LBData, &lineBuffer);
    
    numTrisIndices = trisSize * 3;
    numLinesIndices = linesSize * 2;
}

ModelRenderer::ModelRenderer(DgRenderDevice renderDevice, DgSwapChain swapChain, Texture& matcap,
                             RendererCreateOptions options) {
    {
        Diligent::GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PSOCreateInfo.PSODesc.Name = "Surface PSO";
        PSOCreateInfo.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
        PSOCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
        PSOCreateInfo.GraphicsPipeline.SmplDesc.Count = options.sampleCount;
        PSOCreateInfo.GraphicsPipeline.RTVFormats[0] = swapChain->GetDesc().ColorBufferFormat;
        PSOCreateInfo.GraphicsPipeline.DSVFormat = swapChain->GetDesc().DepthBufferFormat;
        PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = Diligent::True;

//        Diligent::BlendStateDesc BlendState;
//        BlendState.RenderTargets[0].BlendEnable = Diligent::True;
//        BlendState.RenderTargets[0].SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
//        BlendState.RenderTargets[0].DestBlend = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
//        PSOCreateInfo.GraphicsPipeline.BlendDesc = BlendState;

        Diligent::ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.UseCombinedTextureSamplers = Diligent::True;

        // Create a vertex shader
        Diligent::RefCntAutoPtr<Diligent::IShader> pVS;
        {
            ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint = "main";
            ShaderCI.Desc.Name = "Surface vertex shader";
            ShaderCI.Source = RendererVSSource;
            renderDevice->CreateShader(ShaderCI, &pVS);

            Diligent::BufferDesc CBDesc;
            CBDesc.Name = "Surface VS constants CB";
            CBDesc.Size = sizeof(RendererVSConstants);
            CBDesc.Usage = Diligent::USAGE_DYNAMIC;
            CBDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
            CBDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
            renderDevice->CreateBuffer(CBDesc, nullptr, &surface.VSConsts);
        }

        // Create a pixel shader
        Diligent::RefCntAutoPtr<Diligent::IShader> pPS;
        {
            ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
            ShaderCI.EntryPoint = "main";
            ShaderCI.Desc.Name = "Surface pixel shader";
            ShaderCI.Source = RendererPSSource;
            renderDevice->CreateShader(ShaderCI, &pPS);

            Diligent::BufferDesc CBDesc;
            CBDesc.Name = "Surface PS constants CB";
            CBDesc.Size = sizeof(RendererPSConstants);
            CBDesc.Usage = Diligent::USAGE_DYNAMIC;
            CBDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
            CBDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
            renderDevice->CreateBuffer(CBDesc, nullptr, &surface.PSConsts);
        }

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        Diligent::LayoutElement LayoutElems[] =
        {
            // Attribute 0 - vertex position
            Diligent::LayoutElement{0, 0, 3, Diligent::VT_FLOAT32, Diligent::False},
            // Attribute 1 - normal
            Diligent::LayoutElement{1, 0, 3, Diligent::VT_FLOAT32, Diligent::False},
            // Attribute 2 - texture coordinate
            Diligent::LayoutElement{2, 0, 2, Diligent::VT_FLOAT32, Diligent::False},
            // Attribute 3 - color
            Diligent::LayoutElement{3, 0, 4, Diligent::VT_FLOAT32, Diligent::False},
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
        
        renderDevice->CreateGraphicsPipelineState(PSOCreateInfo, &surface.PSO);

        auto VSConstsVar = surface.PSO->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "Constants");
        if(VSConstsVar != nullptr)
            VSConstsVar->Set(surface.VSConsts);
        
        auto PSConstsVar = surface.PSO->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "Constants");
        if(PSConstsVar != nullptr)
            PSConstsVar->Set(surface.PSConsts);

        surface.PSO->CreateShaderResourceBinding(&surface.SRB, true);
        
        auto textureVar = surface.SRB->GetVariableByName(Diligent::SHADER_TYPE_PIXEL, "g_Texture");
        if(textureVar != nullptr)
            textureVar->Set(matcap.textureSRV);
    }
    
    {
        Diligent::GraphicsPipelineStateCreateInfo PSOCreateInfo;
        PSOCreateInfo.PSODesc.Name = "Wireframe PSO";
        PSOCreateInfo.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
        PSOCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
        PSOCreateInfo.GraphicsPipeline.SmplDesc.Count = options.sampleCount;
        PSOCreateInfo.GraphicsPipeline.RTVFormats[0] = swapChain->GetDesc().ColorBufferFormat;
        PSOCreateInfo.GraphicsPipeline.DSVFormat = swapChain->GetDesc().DepthBufferFormat;
        PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_LINE_LIST;
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
        PSOCreateInfo.GraphicsPipeline.RasterizerDesc.AntialiasedLineEnable = Diligent::True;
        PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = Diligent::True;

//        Diligent::BlendStateDesc BlendState;
//        BlendState.RenderTargets[0].BlendEnable = Diligent::True;
//        BlendState.RenderTargets[0].SrcBlend = Diligent::BLEND_FACTOR_SRC_ALPHA;
//        BlendState.RenderTargets[0].DestBlend = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
//        PSOCreateInfo.GraphicsPipeline.BlendDesc = BlendState;

        Diligent::ShaderCreateInfo ShaderCI;
        ShaderCI.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
        ShaderCI.UseCombinedTextureSamplers = Diligent::True;

        // Create a vertex shader
        Diligent::RefCntAutoPtr<Diligent::IShader> pVS;
        {
            ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
            ShaderCI.EntryPoint = "main";
            ShaderCI.Desc.Name = "Wireframe vertex shader";
            ShaderCI.Source = RendererVSSource;
            renderDevice->CreateShader(ShaderCI, &pVS);

            Diligent::BufferDesc CBDesc;
            CBDesc.Name = "Wireframe VS constants CB";
            CBDesc.Size = sizeof(RendererVSConstants);
            CBDesc.Usage = Diligent::USAGE_DYNAMIC;
            CBDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
            CBDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
            renderDevice->CreateBuffer(CBDesc, nullptr, &wireframe.VSConsts);
        }

        // Create a pixel shader
        Diligent::RefCntAutoPtr<Diligent::IShader> pPS;
        {
            ShaderCI.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
            ShaderCI.EntryPoint = "main";
            ShaderCI.Desc.Name = "Wireframe pixel shader";
            ShaderCI.Source = RendererWireframePSSource;
            renderDevice->CreateShader(ShaderCI, &pPS);

            Diligent::BufferDesc CBDesc;
            CBDesc.Name = "Wireframe PS constants CB";
            CBDesc.Size = sizeof(RendererPSConstants);
            CBDesc.Usage = Diligent::USAGE_DYNAMIC;
            CBDesc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
            CBDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
            renderDevice->CreateBuffer(CBDesc, nullptr, &wireframe.PSConsts);
        }

        PSOCreateInfo.pVS = pVS;
        PSOCreateInfo.pPS = pPS;

        Diligent::LayoutElement LayoutElems[] =
        {
            // Attribute 0 - vertex position
            Diligent::LayoutElement{0, 0, 3, Diligent::VT_FLOAT32, Diligent::False},
            // Attribute 1 - normal
            Diligent::LayoutElement{1, 0, 3, Diligent::VT_FLOAT32, Diligent::False},
            // Attribute 2 - texture coordinate
            Diligent::LayoutElement{2, 0, 2, Diligent::VT_FLOAT32, Diligent::False},
            // Attribute 3 - color
            Diligent::LayoutElement{3, 0, 4, Diligent::VT_FLOAT32, Diligent::False},
        };
        PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
        PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements = _countof(LayoutElems);
        PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

        renderDevice->CreateGraphicsPipelineState(PSOCreateInfo, &wireframe.PSO);

        auto VSConstsVar = wireframe.PSO->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "Constants");
        if(VSConstsVar != nullptr)
            VSConstsVar->Set(wireframe.VSConsts);
        
        auto PSConstsVar = wireframe.PSO->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "Constants");
        if(PSConstsVar != nullptr)
            PSConstsVar->Set(wireframe.PSConsts);

        wireframe.PSO->CreateShaderResourceBinding(&wireframe.SRB, true);
    }
}

Editor::Editor(DgRenderDevice renderDevice, DgSwapChain swapChain, Texture& matcap,
               RendererCreateOptions options):
    matcap(matcap), renderDevice(renderDevice),
    renderer(renderDevice, swapChain, this->matcap, options)
{
}

void ModelRenderer::draw(DgDeviceContext context,
          glm::mat4 modelViewProj, glm::mat4 modelView, Model& model) {
    
    RendererVSConstants VSConstants;
    VSConstants.MVP = glm::transpose(modelViewProj);
    VSConstants.normal = glm::transpose(glm::mat3(glm::transpose(glm::inverse(glm::mat4(1.0f)))));
    {
        Diligent::MapHelper<RendererVSConstants> CBConstants(context, surface.VSConsts,
            Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
        *CBConstants = VSConstants;
    }
    {
        Diligent::MapHelper<RendererVSConstants> CBConstants(context, wireframe.VSConsts,
            Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
        *CBConstants = VSConstants;
    }
    {
        Diligent::MapHelper<RendererPSConstants> CBConstants(context, surface.PSConsts,
            Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
        RendererPSConstants constants;
        constants.modelView = modelView;
//        constants.eye = glm::normalize(-eye);
        *CBConstants = constants;
    }

    Diligent::Uint64   offset = 0;
    Diligent::IBuffer* pBuffs[] = { model.vertexBuffer };
    context->SetVertexBuffers(0, 1, pBuffs, &offset,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
        Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
    
    // Surface
    {
        context->SetIndexBuffer(model.triangleBuffer, 0,
            Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        context->SetPipelineState(surface.PSO);

        context->CommitShaderResources(surface.SRB, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        Diligent::DrawIndexedAttribs DrawAttrs;
        DrawAttrs.IndexType = Diligent::VT_UINT32;
        DrawAttrs.NumIndices = model.numTrisIndices;
        DrawAttrs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;

        context->DrawIndexed(DrawAttrs);
    }
    
    // Wireframe
    {
        context->SetIndexBuffer(model.lineBuffer, 0,
            Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        context->SetPipelineState(wireframe.PSO);

        context->CommitShaderResources(wireframe.SRB, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        Diligent::DrawIndexedAttribs DrawAttrs;
        DrawAttrs.IndexType = Diligent::VT_UINT32;
        DrawAttrs.NumIndices = model.numLinesIndices;
        DrawAttrs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;

        context->DrawIndexed(DrawAttrs);
    }
}

void Editor::input(bool isMouseDown, float mouseX, float mouseY) {
    
}

void Editor::draw(DgDeviceContext context) {
    renderer.draw(context, viewProj, view, *model);
}
