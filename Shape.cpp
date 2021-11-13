#include <Shape.h>
#include "Graphics/GraphicsTools/interface/CommonlyUsedStates.h"
#include "Graphics/GraphicsTools/interface/MapHelper.hpp"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <Tube.h>
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

void PopulateShapeBuffers(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice,
                          std::vector<ShapeVertex>& vertices,
                          std::vector<int> indices, Shape& shape) {
    Diligent::BufferDesc VertBuffDesc;
    VertBuffDesc.Name = "Shape vertex buffer";
    VertBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
    VertBuffDesc.Size = vertices.size() * sizeof(ShapeVertex);
    Diligent::BufferData VBData;
    VBData.pData = vertices.data();
    VBData.DataSize = vertices.size() * sizeof(ShapeVertex);
    renderDevice->CreateBuffer(VertBuffDesc, &VBData, &shape.vertexBuffer);

    Diligent::BufferDesc IndBuffDesc;
    IndBuffDesc.Name = "Shape index buffer";
    IndBuffDesc.Usage = Diligent::USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags = Diligent::BIND_INDEX_BUFFER;
    IndBuffDesc.Size = indices.size() * sizeof(int);
    Diligent::BufferData IBData;
    IBData.pData = indices.data();
    IBData.DataSize = indices.size() * sizeof(int);
    renderDevice->CreateBuffer(IndBuffDesc, &IBData, &shape.indexBuffer);

    shape.vertices = vertices;
    shape.indices = indices;
    shape.numIndices = (int)indices.size();
}

void PopulateShapeDimensions(MinMax2 bounds, Shape& shape) {
    shape.width = bounds.maxX - bounds.minX;
    shape.height = bounds.maxY - bounds.minY;
    shape.offset = glm::vec2(-bounds.minX, -bounds.minY);
    shape.bounds = bounds;
}

Shape CreateFill(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice,
	tube::Path path, bool isConvex)
{
	Shape fill;
	tube::Shape shape = path.toShape();
	MinMax2 bounds = CalculateBoundingBox2D(shape.verts);
	std::vector<int> indices = CreateIndicesConvex(shape.verts.size());
	std::vector<glm::vec2> UV = GenerateUV(bounds, shape.verts);
	std::vector<ShapeVertex> vertices;
	vertices.resize(shape.verts.size());
	for (int i = 0; i < vertices.size(); i++) {
		vertices[i].position = shape.verts[i];
		vertices[i].texCoord = UV[i];
	}
    PopulateShapeDimensions(bounds, fill);
    PopulateShapeBuffers(renderDevice, vertices, indices, fill);
	return fill;
}

Shape CreateStroke(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice,
                   tube::Builder& builder)
{
    Shape stroke;
    tube::Tube tube = builder.apply();
    std::vector<ShapeVertex> vertices;
    vertices.resize(tube.vertices.size());
    for (int i = 0; i < vertices.size(); i++) {
        vertices[i].position = tube.vertices[i];
        vertices[i].texCoord = tube.texCoords[i];
    }
    PopulateShapeDimensions(CalculateBoundingBox2D(tube.vertices), stroke);
    PopulateShapeBuffers(renderDevice, vertices, tube.indices, stroke);
    return stroke;
}

ShapeRenderer::ShapeRenderer(
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
		ShaderCI.Source = ShapePSSource;
		renderDevice->CreateShader(ShaderCI, &pPS);

		Diligent::BufferDesc CBDesc;
		CBDesc.Name = "PS constants CB";
		CBDesc.Size = sizeof(ShapePSConstants);
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

void ShapeRenderer::draw(
	Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context,
	glm::mat4 modelViewProjection, Shape& shape, glm::vec3 color, float opacity)
{
	{
		Diligent::MapHelper<glm::mat4> CBConstants(context, mVSConstants,
			Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
		*CBConstants = glm::transpose(modelViewProjection);
	}
	{
		Diligent::MapHelper<ShapePSConstants> CBConstants(context, mPSConstants,
			Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD);
		ShapePSConstants constants;
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

bool Shape::containsPoint(glm::vec2 point)
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

float Shape::intersectsRay(glm::vec2 rayOrigin, glm::vec2 rayDirection) {
    float distance = NoIntersection;
    for (size_t i = 0; i < this->vertices.size() - 1; i++) {
        glm::vec2 lineStart = this->vertices[i].position;
        glm::vec2 lineEnd = this->vertices[i + 1LL].position;
        distance = fminf(distance, raySegmentIntersection(rayOrigin, rayDirection, lineStart, lineEnd));
    }
    // First and last vertices
    distance = fminf(distance, raySegmentIntersection(rayOrigin, rayDirection,
                                                      this->vertices[0].position,
                                                      this->vertices.back().position));
    return distance;
}

void ShapeMesh::add(ShapeMesh& b) {
    int plusVertices = this->vertices.size();
    int plusIndices = this->indices.size();
    this->vertices.resize(this->vertices.size() + b.vertices.size());
    this->indices.resize(this->indices.size() + b.indices.size());
    for(int i = 0; i < b.vertices.size(); i++)
        this->vertices[i + plusVertices] = b.vertices[i];
    for(int i = 0; i < b.indices.size(); i++) {
        TriangeIndices tri = b.indices[i];
        tri.a += plusVertices;
        tri.b += plusVertices;
        tri.c += plusVertices;
        this->indices[i + plusIndices] = tri;
    }
}

ShapeMesh strokePolyline(std::vector<glm::vec2>& points, const float diameter) {
    float radius = diameter / 2.0f;
    size_t numPoints = points.size();
    ShapeMesh mesh;
    
    if(points.size() < 2)
        return mesh;
    
    // Calculate arrays size
    mesh.vertices.resize(numPoints * 2);
    mesh.indices.resize(numPoints * 2 - 2);
    
    // For each point we add two points to the mesh and
    // connect them with previous two points if any
    for(size_t i = 0; i < numPoints; i++) {
        // Check for tips
        bool isStart = i == 0;
        bool isEnd = i == numPoints - 1;

        // Retrieve points
        glm::vec2 currentPoint = points[i];
        glm::vec2 backPoint = !isStart ? points[i - 1LL] : currentPoint;
        glm::vec2 nextPoint = !isEnd   ? points[i + 1LL] : currentPoint;
        
        // Calculate three diections
        glm::vec2 forwardDir  = glm::normalize(nextPoint - currentPoint);
        glm::vec2 backwardDir = glm::normalize(backPoint - currentPoint) * -1.0f;
        glm::vec2 meanDir;
        
        // Calculate mean of forward and backward directions
        if (isStart) meanDir = forwardDir;
        else if (isEnd) meanDir = backwardDir;
        else meanDir = (forwardDir + backwardDir) / 2.0f;
        
        // Perpendicular to mean direction
        glm::vec2 a = glm::vec2(meanDir.y, -meanDir.x) * radius;
        glm::vec2 b = -a;
        
        // Put put vectors into the right place
        a += currentPoint;
        b += currentPoint;
        
        // Add vectors to mesh vertices
        size_t currentVertexIndex = i * 2;
        size_t previousVertexIndex = currentVertexIndex - 2;
        mesh.vertices.at(currentVertexIndex) = a;
        mesh.vertices.at(currentVertexIndex + 1LL) = b;
        
        // Connect vertices with bridge of two trianges
        if(!isStart) {
            int idxA = currentVertexIndex;
            int idxB = currentVertexIndex + 1;
            int idxC = previousVertexIndex;
            int idxD = previousVertexIndex + 1;
            TriangeIndices m { idxA, idxB, idxC };
            TriangeIndices k { idxB, idxC, idxD };
            size_t index = i * 2LL - 2;
            mesh.indices.at(index) = m;
            mesh.indices.at(index + 1LL) = k;
        }
    }
    return mesh;
}

bool isCurvesCorrectForJoining(std::vector<glm::vec2>& a, std::vector<glm::vec2>& b) {
    if (a.size() < 2 || b.size() < 2)
        return false;
    return true;
}

ShapeMesh bevelJoin(std::vector<glm::vec2>& a, std::vector<glm::vec2>& b, const float diameter) {
    float radius = diameter / 2.0f;
    ShapeMesh mesh;
    
    if(!isCurvesCorrectForJoining(a, b))
        return mesh;
    
    glm::vec2 center = b.at(0);
    glm::vec2 dirA = glm::normalize(a.at(a.size() - 1) - a.at(a.size() - 2));
    glm::vec2 dirB = glm::normalize(b.at(1) - b.at(0));
    
    // --A   C-__
    //   |  /    --
    // --B  D-__
    //          --
    
    glm::vec2 Ad = glm::vec2(dirA.y, -dirA.x) * radius;
    glm::vec2 Bd = -Ad;
    glm::vec2 Cd = glm::vec2(dirB.y, -dirB.x) * radius;
    glm::vec2 Dd = -Cd;
    glm::vec2 A = Ad + center;
    glm::vec2 B = Bd + center;
    glm::vec2 C = Cd + center;
    glm::vec2 D = Dd + center;
    
    float angle = glm::orientedAngle(dirA, dirB);
    
    mesh.vertices.resize(3);
    mesh.vertices.at(0) = center;
    if(angle > 0) {
        mesh.vertices.at(1) = A;
        mesh.vertices.at(2) = C;
    }
    else {
        mesh.vertices.at(1) = B;
        mesh.vertices.at(2) = D;
    }
    TriangeIndices tri { 0, 1, 2 };
    mesh.indices.push_back(tri);
    
    return mesh;
}

std::vector<glm::vec2> quadraticBezier2D(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, int segments) {
    auto points = std::vector<glm::vec2>(segments);
    float step = 1.0f / (segments - 1);
    float t = 0.0f;
    for (int i = 0; i < segments; i++) {
        glm::vec2 q0 = glm::mix(p0, p1, t);
        glm::vec2 q1 = glm::mix(p1, p2, t);
        glm::vec2 r = glm::mix(q0, q1, t);
        points[i] = r;
        t += step;
    }
    return points;
}

std::vector<glm::vec2> cubicBezier2D(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, int segments) {
    auto points = std::vector<glm::vec2>(segments);
    float step = 1.0f / (segments - 1);
    float t = 0.0f;
    for (int i = 0; i < segments; i++) {
        glm::vec2 q0 = glm::mix(p0, p1, t);
        glm::vec2 q1 = glm::mix(p1, p2, t);
        glm::vec2 q2 = glm::mix(p2, p3, t);
        glm::vec2 r0 = glm::mix(q0, q1, t);
        glm::vec2 r1 = glm::mix(q1, q2, t);
        glm::vec2 b = glm::mix(r0, r1, t);
        points[i] = b;
        t += step;
    }
    return points;
}

std::vector<TriangeIndices> createIndicesConvex2D(size_t numVertices) {
    size_t amount = numVertices - 2;
    auto indices = std::vector<TriangeIndices>(amount);
    int k = 1;
    for (size_t i = 0; i < amount; i++)
    {
        indices[i].a = 0;
        
        // Connect current edge with next
        indices[i].b = k;
        indices[i].c = k + 1;
        k++;
    }
    return indices;
}

std::vector<glm::vec2> createArc(float startAngle, float endAngle, float radius, int segments, glm::vec2 offset) {
    auto arcVerts = std::vector<glm::vec2>(segments);
    float angle = startAngle;
    float arcLength = endAngle - startAngle;
    for (int i = 0; i <= segments - 1; i++) {
        float x = sin(angle) * radius;
        float y = cos(angle) * radius;

        arcVerts[i] = glm::vec2(x, y) + offset;
        angle += (arcLength / segments);
    }
    return arcVerts;
}

ShapeMesh roundJoin(std::vector<glm::vec2>& a, std::vector<glm::vec2>& b, const float diameter) {
    float radius = diameter / 2.0f;
    ShapeMesh mesh;
    
    if(!isCurvesCorrectForJoining(a, b))
        return mesh;
    
    glm::vec2 center = b.at(0);
    glm::vec2 dirA = glm::normalize(a.at(a.size() - 1) - a.at(a.size() - 2));
    glm::vec2 dirB = glm::normalize(b.at(1) - b.at(0));
    
    glm::vec2 Ad = glm::vec2(dirA.y, -dirA.x) * radius;
    glm::vec2 Bd = -Ad;
    glm::vec2 Cd = glm::vec2(dirB.y, -dirB.x) * radius;
    glm::vec2 Dd = -Cd;
    glm::vec2 A = Ad + center;
    glm::vec2 B = Bd + center;
    glm::vec2 C = Cd + center;
    glm::vec2 D = Dd + center;
    
    float angle = glm::orientedAngle(dirA, dirB);
    
    static const int numCurveSegments = 32;
    mesh.vertices.resize(1);
    mesh.vertices.at(0) = center;
    std::vector<glm::vec2> curve;
    float start = 0;
    float end = 0;
    if(angle > 0) {
        start = atan2f(Ad.x, Ad.y);
        end = atan2f(Cd.x, Cd.y) - 0.1f;
    }
    else {
        start = atan2f(Bd.x, Bd.y);
        end = atan2f(Dd.x, Dd.y) + 0.1f;
    }
    curve = createArc(start, end, radius, 32, center);
    mesh.vertices.insert(mesh.vertices.end(), curve.begin(), curve.end());
    std::vector<TriangeIndices> tris = createIndicesConvex2D(mesh.vertices.size());
    mesh.indices.insert(mesh.indices.end(), tris.begin(), tris.end());
    
    return mesh;
}

glm::vec2 EliminationLineLineIntersection(glm::vec2 v1, glm::vec2 v2, glm::vec2 v3, glm::vec2 v4)
{
    float x12 = v1.x - v2.x;
    float x34 = v3.x - v4.x;
    float y12 = v1.y - v2.y;
    float y34 = v3.y - v4.y;
    float c = x12 * y34 - y12 * x34;
    float a = v1.x * v2.y - v1.y * v2.x;
    float b = v3.x * v4.y - v3.y * v4.x;
    if(c != 0) {
        float x = (a * x34 - b * x12) / c;
        float y = (a * y34 - b * y12) / c;
        return glm::vec2(x, y);
    }
    else {
        // Lines are parallel
        return glm::vec2(0.0f);
    }
}

glm::vec2 LineLineIntersection(glm::vec2 v1, glm::vec2 v2, glm::vec2 v3, glm::vec2 v4) {
    return EliminationLineLineIntersection(v1, v2, v3, v4);
}

ShapeMesh miterJoin(std::vector<glm::vec2>& a, std::vector<glm::vec2>& b, const float diameter,
                    float miterLimitAngle) {
    float radius = diameter / 2.0f;
    ShapeMesh mesh;
    
    if(!isCurvesCorrectForJoining(a, b))
        return mesh;
    
    glm::vec2 center = b.at(0);
    glm::vec2 dirA = glm::normalize(a.at(a.size() - 1) - a.at(a.size() - 2));
    glm::vec2 dirB = glm::normalize(b.at(1) - b.at(0));
    
    float angle = glm::orientedAngle(dirA, dirB);
    
    if (fabsf(angle) > miterLimitAngle)
        return bevelJoin(a, b, diameter);
    
    glm::vec2 Ad = glm::vec2(dirA.y, -dirA.x) * radius;
    glm::vec2 Bd = -Ad;
    glm::vec2 Cd = glm::vec2(dirB.y, -dirB.x) * radius;
    glm::vec2 Dd = -Cd;
    glm::vec2 A = Ad + center;
    glm::vec2 B = Bd + center;
    glm::vec2 C = Cd + center;
    glm::vec2 D = Dd + center;
    
    mesh.vertices.resize(4);
    mesh.vertices.at(0) = center;
    if(angle > 0) {
        glm::vec2 I = LineLineIntersection(A, A + dirA, C, C + dirB);
        mesh.vertices.at(1) = A;
        mesh.vertices.at(2) = C;
        mesh.vertices.at(3) = I;
    }
    else {
        glm::vec2 I = LineLineIntersection(B, B + dirA, D, D + dirB);
        mesh.vertices.at(1) = B;
        mesh.vertices.at(2) = D;
        mesh.vertices.at(3) = I;
    }
    //   0
    //  / \
    // 1---2
    //  \ /
    //   3
    std::vector<TriangeIndices> tris = {
        { 0, 1, 2 },
        { 1, 2, 3 }
    };
    mesh.indices.insert(mesh.indices.end(), tris.begin(), tris.end());
    
    return mesh;
}

TwoPolylines dividePolyline(std::vector<glm::vec2>& points, float t) {
    TwoPolylines twoLines;
    if (t <= 0) {
        twoLines.second = points;
        return twoLines;
    }
    if (t >= 1) {
        twoLines.first = points;
        return twoLines;
    }
    float remapedT = t * (float)(points.size() - 1);
    int segmentIdx = (int)floorf(remapedT);
    float segmentT = remapedT - (float)segmentIdx;
    
    twoLines.first.resize(segmentIdx + 2);
    for (int i = 0; i < segmentIdx + 1; i++) {
        twoLines.first.at(i) = points.at(i);
    }
    twoLines.first.at(segmentIdx + 1) = glm::mix(points.at(segmentIdx), points.at(segmentIdx + 1), segmentT);
    
    twoLines.second.resize(points.size() - segmentIdx);
    for (int i = 1; i < twoLines.second.size(); i++){
        twoLines.second.at(i) = points.at(segmentIdx + i);
    }
    twoLines.second.at(0) = glm::mix(points.at(segmentIdx), points.at(segmentIdx + 1), segmentT);
    
    return twoLines;
}

MinMax2 CalculateBoundingBoxVec2(std::vector<glm::vec2>& verts) {
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

Shape CreateShapeFromMesh(Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice,
                   ShapeMesh& mesh)
{
    Shape stroke;
    std::vector<ShapeVertex> vertices;
    vertices.resize(mesh.vertices.size());
    for (int i = 0; i < vertices.size(); i++) {
        vertices[i].position = glm::vec3(mesh.vertices[i], 0.0f);
        vertices[i].texCoord = glm::vec2(0.0f);
    }
    std::vector<int> indices;
    indices.resize(mesh.indices.size() * 3);
    for (int i = 0; i < mesh.indices.size(); i++) {
        TriangeIndices tri = mesh.indices[i];
        int shiftedI = i * 3;
        indices[shiftedI] = tri.a;
        indices[shiftedI + 1LL] = tri.b;
        indices[shiftedI + 2LL] = tri.c;
    }
    PopulateShapeDimensions(CalculateBoundingBoxVec2(mesh.vertices), stroke);
    PopulateShapeBuffers(renderDevice, vertices, indices, stroke);
    return stroke;
}

float lengthOfPolyline(std::vector<glm::vec2>& points) {
    if (points.size() < 2)
        return 0.0f;

    glm::vec2 previousPoint = points.at(0);
    float length = 0.0f;
    for (size_t i = 1; i < points.size(); i++) {
        length += glm::distance(points.at(i), previousPoint);
        previousPoint = points.at(i);
    }
    return length;
}

std::vector<float> measurePolyline(std::vector<glm::vec2>& points) {
    std::vector<float> lengths;
    lengths.resize(points.size() - 1); // number of segments
    for (size_t i = 0; i < lengths.size(); i++) {
        lengths.at(i) = glm::distance(points.at(i), points.at(i + 1LL));
    }
    return lengths;
}

float tAtLength(float length, std::vector<float>& lengths) {
    size_t pointBeforeLength = 0;
    float previousLength = 0.0f;
    float currentLength = 0.0f;
    float localT = 0.0f;
    if(lengths.size() > 0) {
        localT = fminf(length, lengths.at(0));
    }
    for (size_t i = 0; i < lengths.size(); i++) {
        previousLength = currentLength;
        currentLength += lengths.at(i);
        if (length < currentLength) {
            pointBeforeLength = i;
            localT = (length - previousLength) / lengths.at(i);
            break;
        }
    }
    float t = ((float)pointBeforeLength + localT) / (float)lengths.size();
    return t;
}

std::vector<std::vector<glm::vec2>> dashedPolyline(std::vector<glm::vec2>& points,
                                                   float dashLength, float gapLength,
                                                   float offset) {
    std::vector<std::vector<glm::vec2>> lines;
    std::vector<glm::vec2> currentPath = points;
    
    float dashGapLength = dashLength + gapLength;
    float offsetTimes = floorf(fabsf(offset) / dashGapLength);
    float localOffset = fabsf(offset) - offsetTimes * dashGapLength;
    if(offset > 0) {
        std::vector<float> lengths = measurePolyline(currentPath);
        if(localOffset > gapLength) {
            float startDashLength = localOffset - gapLength;
            lines.push_back(dividePolyline(currentPath, tAtLength(startDashLength, lengths)).first);
        }
        currentPath = dividePolyline(currentPath, tAtLength(localOffset, lengths)).second;
    }
    else if(offset < 0) {
        std::vector<float> lengths = measurePolyline(currentPath);
        if(localOffset < dashLength) {
            float startDashLength = dashLength - localOffset;
            lines.push_back(dividePolyline(currentPath, tAtLength(startDashLength, lengths)).first);
        }
        currentPath = dividePolyline(currentPath, tAtLength(dashGapLength - localOffset,
                                                            lengths)).second;
    }
    
    static const int maxDashes = 999;
    for(int i = 0; i < maxDashes; i++) {
        std::vector<float> lengths = measurePolyline(currentPath);
        TwoPolylines twoPathes = dividePolyline(currentPath, tAtLength(dashLength, lengths));
        if(twoPathes.first.size() < 2)
            break;
        lines.push_back(twoPathes.first);
        if(twoPathes.second.size() < 2)
            break;
        std::vector<float> secondLengths = measurePolyline(twoPathes.second);
        currentPath = dividePolyline(twoPathes.second, tAtLength(gapLength, secondLengths)).second;
        if(currentPath.size() < 2)
            break;
    }
    return lines;
}
