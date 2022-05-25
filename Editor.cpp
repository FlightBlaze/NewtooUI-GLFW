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

void* convertRGBAToBGRA(char* imageData,
                       int width,
                       int height)
{
    char* newImage = new char[width * height * 4];
    for(size_t i = 0; i < width * height; i++) {
        size_t index = i * 4LL;
        newImage[index] = imageData[index + 2LL]; // Red -> Blue
        newImage[index + 1LL] = imageData[index + 1LL]; // Green
        newImage[index + 2LL] = imageData[index]; // Blue -> Red
        newImage[index + 3LL] = imageData[index + 3LL]; // Alpha
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
    
    bool isBGRA = false;
    if(colorBufferFormat == Diligent::TEX_FORMAT_BGRA8_UNORM ||
       colorBufferFormat == Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB) {
        isBGRA = true;
        void* newImageData = convertRGBAToBGRA((char*)imageData, width, height);
        if(numChannels == 3)
            delete[] (char*)imageData;
        imageData = newImageData;
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
    
    if(numChannels == 3 || isBGRA) {
        delete[] (char*)imageData;
    }
}

Texture::Texture() {}

Ray screenPointToRay(glm::vec2 pos, glm::vec2 screenDims, glm::mat4 viewProj)
{
    auto ray = Ray();

    // screen coords to clip coords
    float normalizedX = pos.x / (screenDims.x * 0.5f) - 1.0f;
    float normalizedY = pos.y / (screenDims.y * 0.5f) - 1.0f;

    glm::mat4 invVP = glm::inverse(viewProj);

    glm::vec4 originClipSpace { normalizedX, -normalizedY, -1.0f, 1.0f };
    glm::vec4 destClipSpace { normalizedX, -normalizedY, 1.0f, 1.0f };
    glm::vec4 originClipSpaceWS = invVP * originClipSpace;
    glm::vec4 destClipSpaceWS = invVP * destClipSpace;
    glm::vec3 originClipSpaceWS3 = glm::vec3(originClipSpaceWS) / originClipSpaceWS.w;
    glm::vec3 destClipSpaceWS3 = glm::vec3(destClipSpaceWS) / destClipSpaceWS.w;

    ray.origin = originClipSpaceWS3;
    ray.direction = glm::normalize(destClipSpaceWS3 - originClipSpaceWS3);

    return ray;
}

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

void Model::invalidate(DgRenderDevice renderDevice, DgDeviceContext context) {
    originalToRenderVerts.clear();
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
        renderMesh.request_vertex_status();
        renderMesh.request_face_status();
        renderMesh.request_edge_status();
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
            PolyMesh::FaceHandle newFace = renderMesh.add_face(faceVerts);
            if(fh.selected())
                renderMesh.status(newFace).set_selected(true);
        }
        renderMesh.request_face_normals();
        renderMesh.update_normals();
        renderMesh.release_face_normals();
    }
    renderMesh.triangulate();
    
    if(renderDevice != nullptr) {
        populateRenderBuffers(renderDevice, context);
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

void addQuad(std::vector<RenderTriange>& list, int offset, int v0, int v1, int v2, int v3) {
    /*
       v0     v1
     
       v3     v2
     */
    RenderTriange tri1;
    tri1.a = v0 + offset;
    tri1.b = v1 + offset;
    tri1.c = v3 + offset;
    RenderTriange tri2;
    tri2.a = v3 + offset;
    tri2.b = v1 + offset;
    tri2.c = v2 + offset;
    list.push_back(tri1);
    list.push_back(tri2);
}

void makeWireframe(std::vector<RenderVertex>& verts, std::vector<RenderTriange>& tris,
                   PolyMesh& originalMesh, float thickness = 0.02f,
                   std::vector<PolyMesh::EdgeHandle>* vertsEdges = nullptr) {
    int trisSize = 0;
    for(auto eh : originalMesh.edges()) {
        trisSize++;
    }
    trisSize *= 12;
    
    int vertsSize = 0;
    for(auto eh : originalMesh.edges()) {
        vertsSize += 2;
    }
    vertsSize *= 8;
    
    verts.reserve(vertsSize);
    if(vertsEdges != nullptr)
        vertsEdges->reserve(verts.size());
    int vertIndex = 0;
    
    tris.reserve(trisSize);
    for(auto eh : originalMesh.edges()) {
        bool isSelected = originalMesh.status(eh).selected();
        glm::vec4 color = isSelected? glm::vec4(1.0f, 0.5f, 0.0f, 1.0f) :
            glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
        glm::vec3 pos1 = vec3FromPoint(originalMesh.point(eh.v0()));
        glm::vec3 pos2 = vec3FromPoint(originalMesh.point(eh.v1()));
        // Fix edges overlapping at corners
        pos1 += glm::normalize(pos2 - pos1) * 0.012f;
        pos2 += glm::normalize(pos1 - pos2) * 0.012f;
        glm::vec3 normal1 = glm::normalize(vec3FromPoint(originalMesh.normal(eh.v0())));
        glm::vec3 normal2 = glm::normalize(vec3FromPoint(originalMesh.normal(eh.v1())));
        RenderTriange line;
        RenderVertex v1;
        v1.normal = glm::cross(glm::normalize(pos2 - pos1), normal1);
        v1.pos = pos1 + v1.normal * thickness;
        v1.color = color;
        RenderVertex v2;
        v2.normal = v1.normal;
        v2.pos = pos2 + v2.normal * thickness;
        v2.color = color;
        RenderVertex v3;
        v3.normal = -v1.normal;
        v3.pos = pos1 + v3.normal * thickness;
        v3.color = color;
        RenderVertex v4;
        v4.normal = -v2.normal;
        v4.pos = pos2 + v4.normal * thickness;
        v4.color = color;
        RenderVertex v5 = v1;
        v5.pos += normal1 * thickness * 2.0f;
        v5.color = color;
        RenderVertex v6 = v2;
        v6.pos += normal2 * thickness * 2.0f;
        v6.color = color;
        RenderVertex v7 = v3;
        v7.pos += normal1 * thickness * 2.0f;
        v7.color = color;
        RenderVertex v8 = v4;
        v8.pos += normal2 * thickness * 2.0f;
        v8.color = color;
        
        v1.pos -= normal1 * thickness * 2.0f;
        v2.pos -= normal2 * thickness * 2.0f;
        v3.pos -= normal1 * thickness * 2.0f;
        v4.pos -= normal2 * thickness * 2.0f;
        
        verts.push_back(v1);
        verts.push_back(v2);
        verts.push_back(v3);
        verts.push_back(v4);
        verts.push_back(v5);
        verts.push_back(v6);
        verts.push_back(v7);
        verts.push_back(v8);
        
        if(vertsEdges != nullptr) {
            for(int i = 0; i < 8; i++) {
                vertsEdges->push_back(eh);
            }
        }
        
        /*
          4___________5
          |\           \
          | 0 ----__-- 1
          6 |   _-   7 |
           \2 --______ 3
         */
        
        addQuad(tris, vertIndex, 0, 1, 3, 2); // back
        addQuad(tris, vertIndex, 6, 7, 5, 4); // front
        addQuad(tris, vertIndex, 4, 5, 1, 0); // top
        addQuad(tris, vertIndex, 3, 1, 5, 7); // right
        addQuad(tris, vertIndex, 6, 7, 3, 2); // bottom
        addQuad(tris, vertIndex, 4, 0, 2, 6); // left
        
        vertIndex+=8;
    }
}

void Model::populateRenderBuffers(DgRenderDevice renderDevice, DgDeviceContext context) {
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
        vert.color = glm::vec4(1.0f);//vec4FromColor(renderMesh.color(vh));
        verts.push_back(vert);
    }
    
    int trisSize = 0;
    for(auto fh : renderMesh.faces()) {
        trisSize++;
    }
    
    std::vector<RenderTriange> tris;
    verts.reserve(trisSize);
    for(auto fh : renderMesh.faces()) {
        bool isSelected = fh.selected();
        RenderTriange tri;
        int fvInd = 0;
        for(auto fvh : renderMesh.fv_cw_range(fh)) {
            int vInd = indexProp[fvh];
            if(isSelected) {
                verts.at(vInd).color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            }
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
    
    int edgesSize = 0;
    for(auto eh : renderMesh.edges()) {
        edgesSize++;
    }
    
    std::vector<RenderVertex> wfVerts;
    std::vector<RenderTriange> wfTris;
    makeWireframe(wfVerts, wfTris, originalMesh);
    
    // Selection wireframe for raycasting
    std::vector<RenderVertex> selWfRenderVerts;
    std::vector<PolyMesh::EdgeHandle> selWfRenderVertsEdges;
    selectionWireframeVerts.clear();
    selectionWireframeTris.clear();
    makeWireframe(selWfRenderVerts, selectionWireframeTris, originalMesh,
                  0.1f, &selWfRenderVertsEdges);
    
    selectionWireframeVerts.reserve(selWfRenderVerts.size() + vertsSize);
    for(auto i = 0; i <  selWfRenderVerts.size(); i++) {
        RenderVertex& renderVert = selWfRenderVerts.at(i);
        PolyMesh::EdgeHandle edge = selWfRenderVertsEdges.at(i);
        EdgeSelectionVertex selVert;
        selVert.pos = renderVert.pos;
        selVert.eh = edge;
        selectionWireframeVerts.push_back(selVert);
    }
    int surfaceIndicesOffset = selWfRenderVerts.size();
    for(auto i = 0; i < verts.size(); i++) {
        RenderVertex& renderVert = verts.at(i);
        EdgeSelectionVertex selVert;
        selVert.pos = renderVert.pos;
        selVert.eh = PolyMesh::InvalidEdgeHandle;
        selectionWireframeVerts.push_back(selVert);
    }
    selectionWireframeTris.reserve(trisSize);
    for(auto i = 0; i < tris.size(); i++) {
        RenderTriange tri = tris.at(i);
        tri.a += surfaceIndicesOffset;
        tri.b += surfaceIndicesOffset;
        tri.c += surfaceIndicesOffset;
        selectionWireframeTris.push_back(tri);
    }
        
    if(vertsSize != lastNumVerts || trisSize != lastNumTris ||
       wfVerts.size() != lastNumWireframeVerts) {
        
        vertexBuffer.Release();
        Diligent::BufferDesc VertBuffDesc;
        VertBuffDesc.Name = "Vertex buffer";
        VertBuffDesc.Usage = Diligent::USAGE_DEFAULT;
        VertBuffDesc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
        VertBuffDesc.Size = verts.size() * sizeof(RenderVertex);
        Diligent::BufferData VBData;
        VBData.pData = verts.data();
        VBData.DataSize = verts.size() * sizeof(RenderVertex);
        renderDevice->CreateBuffer(VertBuffDesc, &VBData, &vertexBuffer);
        
        triangleBuffer.Release();
        Diligent::BufferDesc TriBuffDesc;
        TriBuffDesc.Name = "Triange index buffer";
        TriBuffDesc.Usage = Diligent::USAGE_DEFAULT;
        TriBuffDesc.BindFlags = Diligent::BIND_INDEX_BUFFER;
    //    TriBuffDesc.CPUAccessFlags = Diligent::CPU_ACCESS_FLAGS::CPU_ACCESS_WRITE;
        TriBuffDesc.Size = tris.size() * sizeof(RenderTriange);
        Diligent::BufferData TBData;
        TBData.pData = tris.data();
        TBData.DataSize = tris.size() * sizeof(RenderTriange);
        renderDevice->CreateBuffer(TriBuffDesc, &TBData, &triangleBuffer);

        wireframeVertexBuffer.Release();
        Diligent::BufferDesc WfVertBuffDesc;
        WfVertBuffDesc.Name = "Wireframe vertex buffer";
        WfVertBuffDesc.Usage = Diligent::USAGE_DEFAULT;
        WfVertBuffDesc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
        WfVertBuffDesc.Size = wfVerts.size() * sizeof(RenderVertex);
        Diligent::BufferData WfVBData;
        WfVBData.pData = wfVerts.data();
        WfVBData.DataSize = wfVerts.size() * sizeof(RenderVertex);
        renderDevice->CreateBuffer(WfVertBuffDesc, &WfVBData, &wireframeVertexBuffer);
        
        wireframeTriangleBuffer.Release();
        Diligent::BufferDesc LineBuffDesc;
        LineBuffDesc.Name = "Wireframe triangle index buffer";
        LineBuffDesc.Usage = Diligent::USAGE_DEFAULT;
        LineBuffDesc.BindFlags = Diligent::BIND_INDEX_BUFFER;
        LineBuffDesc.Size = wfTris.size() * sizeof(RenderTriange);
        Diligent::BufferData LBData;
        LBData.pData = wfTris.data();
        LBData.DataSize = wfTris.size() * sizeof(RenderTriange);
        renderDevice->CreateBuffer(LineBuffDesc, &LBData, &wireframeTriangleBuffer);
    } else {
        context->UpdateBuffer(vertexBuffer, 0,
                              verts.size() * sizeof(RenderVertex),
                              verts.data(),
                              Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        
        context->UpdateBuffer(triangleBuffer, 0,
                              tris.size() * sizeof(RenderTriange),
                              tris.data(),
                              Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        
        context->UpdateBuffer(wireframeVertexBuffer, 0,
                              wfVerts.size() * sizeof(RenderVertex),
                              wfVerts.data(),
                              Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        
        context->UpdateBuffer(wireframeTriangleBuffer, 0,
                              wfTris.size() * sizeof(RenderTriange),
                              wfTris.data(),
                              Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
        
    numTrisIndices = trisSize * 3;
    numLinesIndices = wfTris.size() * 3;
    
    lastNumVerts = vertsSize;
    lastNumTris = trisSize;
    lastNumWireframeVerts = wfVerts.size();
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
            ShaderCI.Desc.Name = "Wireframe vertex shader";
            ShaderCI.Source = RendererWireframeVSSource;
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
//    VSConstants.MV = glm::transpose(modelView);
    VSConstants.normal = glm::transpose(glm::mat4(1.0f));
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
        constants.modelView = glm::transpose(modelView);
//        constants.eye = glm::normalize(-eye);
        *CBConstants = constants;
    }
    
    // Surface
    {
        Diligent::Uint64   offset = 0;
        Diligent::IBuffer* pBuffs[] = { model.vertexBuffer };
        context->SetVertexBuffers(0, 1, pBuffs, &offset,
            Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
            Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
        
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
        Diligent::Uint64   offset = 0;
        Diligent::IBuffer* pBuffs[] = { model.wireframeVertexBuffer };
        context->SetVertexBuffers(0, 1, pBuffs, &offset,
            Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
            Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
        
        context->SetIndexBuffer(model.wireframeTriangleBuffer, 0,
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

// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/ray-triangle-intersection-geometric-solution
bool rayTriangleIntersect(
    const glm::vec3 &orig, const glm::vec3 &dir,
    const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2,
    float &t)
{
    // compute plane's normal
    glm::vec3 v0v1 = v1 - v0;
    glm::vec3 v0v2 = v2 - v0;
    // no need to normalize
    glm::vec3 N = glm::cross(v0v1, v0v2);  //N
    float area2 = glm::length(N);
 
    // Step 1: finding P
 
    // check if ray and plane are parallel ?
    float NdotRayDirection = glm::dot(N, dir);
    if (fabs(NdotRayDirection) < 0.0001f)  //almost 0
        return false;  //they are parallel so they don't intersect !
 
    // compute d parameter using equation 2
    float d = glm::dot(-N, v0);
 
    // compute t (equation 3)
    t = -(glm::dot(N, orig) + d) / NdotRayDirection;
 
    // check if the triangle is in behind the ray
    if (t < 0.0f) return false;  //the triangle is behind
 
    // compute the intersection point using equation 1
    glm::vec3 P = orig + t * dir;
 
    // Step 2: inside-outside test
    glm::vec3 C;  //vector perpendicular to triangle's plane
 
    // edge 0
    glm::vec3 edge0 = v1 - v0;
    glm::vec3 vp0 = P - v0;
    C = glm::cross(edge0, vp0);
    if (glm::dot(N, C) < 0) return false;  //P is on the right side
 
    // edge 1
    glm::vec3 edge1 = v2 - v1;
    glm::vec3 vp1 = P - v1;
    C = glm::cross(edge1, vp1);
    if (glm::dot(N, C) < 0)  return false;  //P is on the right side
 
    // edge 2
    glm::vec3 edge2 = v0 - v2;
    glm::vec3 vp2 = P - v2;
    C = glm::cross(edge2, vp2);
    if (glm::dot(N, C) < 0) return false;  //P is on the right side;
 
    return true;  //this ray hits the triangle
}

void Editor::raycastEdges(glm::vec2 mouse, glm::vec2 screenDims,
                          DgRenderDevice renderDevice, DgDeviceContext context) {
    Ray ray = screenPointToRay(mouse, screenDims, viewProj);
    if(model == nullptr)
        return;
    std::vector<EdgeSelectionVertex>& verts = model->selectionWireframeVerts;
    float minDistance = 100000;
    EdgeSelectionVertex* closestVert = nullptr;
    for(auto i = 0; i < model->selectionWireframeTris.size(); i++) {
        RenderTriange tri = model->selectionWireframeTris.at(i);
        glm::vec3 v0 = verts.at(tri.a).pos;
        glm::vec3 v1 = verts.at(tri.b).pos;
        glm::vec3 v2 = verts.at(tri.c).pos;
        float distance = 0;
        if(rayTriangleIntersect(ray.origin, ray.direction, v0, v1, v2, distance)) {
            if(distance < minDistance) {
                minDistance = distance;
                closestVert = &verts.at(tri.a);
            }
        }
    }
    if(closestVert != nullptr) {
        if(closestVert->eh != PolyMesh::InvalidEdgeHandle) {
            if(!isShiftPressed) {
                for(auto eh : model->originalMesh.edges())
                    model->originalMesh.status(eh).set_selected(false);
            }
            PolyMesh::StatusInfo& edgeStatus = model->originalMesh.status(closestVert->eh);
            if(edgeStatus.selected())
                edgeStatus.set_selected(false);
            else
                edgeStatus.set_selected(true);
            model->invalidate(renderDevice, context);
        }
    } else {
        if(!isShiftPressed) {
            bool doInvalidate = false;
            for(auto eh : model->originalMesh.edges()) {
                if(eh.selected()) {
                    doInvalidate = true;
                    model->originalMesh.status(eh).set_selected(false);
                }
            }
            if(doInvalidate)
                model->invalidate(renderDevice, context);
        }
    }
}

void Editor::input(bool isMouseDown, float mouseX, float mouseY) {
    
}

void Editor::draw(DgDeviceContext context) {
    renderer.draw(context, viewProj, view, *model);
}
