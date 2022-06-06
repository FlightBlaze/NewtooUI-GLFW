//
//  Mesh.h
//  MyProject
//
//  Created by Dmitry on 13.05.2022.
//

#pragma once

#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/Utils/PropertyManager.hh>
#include <OpenMesh/Tools/Subdivider/Uniform/CatmullClarkT.hh>
#include <blazevg.hh>
#include <list>
#include <unordered_map>

typedef OpenMesh::PolyMesh_ArrayKernelT<> PolyMesh;

glm::vec3 vec3FromPoint(PolyMesh::Point p);
PolyMesh::Point vec3ToPoint(glm::vec3 v);

std::list<PolyMesh::FaceHandle> getSelectedFaces(PolyMesh& mesh);

class SelectionBoundaryIter {
    std::unordered_map<PolyMesh::FaceHandle, bool> selectedFacesMap;
    std::unordered_map<PolyMesh::HalfedgeHandle, int> traversedHalfedges;
    PolyMesh::HalfedgeHandle start;
    PolyMesh::HalfedgeHandle cur;
    PolyMesh& mesh;
    bool wasNext = false;
    int step = 0;
    
    void next();
    
public:
    SelectionBoundaryIter(PolyMesh::HalfedgeHandle heh, PolyMesh& mesh,
                          std::unordered_map<PolyMesh::FaceHandle, bool>& selectedFacesMap);
    void operator++(int n);
    PolyMesh::HalfedgeHandle operator->();
    PolyMesh::HalfedgeHandle current();
    bool isNotEnd();
    bool isEnd();
};

class EdgeLoopIter {
    PolyMesh::HalfedgeHandle start;
    PolyMesh::HalfedgeHandle cur;
    PolyMesh& mesh;
    bool wasNext = false;
    
    void next();
    
public:
    EdgeLoopIter(PolyMesh::HalfedgeHandle heh, PolyMesh& mesh);
    void operator++(int n);
    PolyMesh::HalfedgeHandle operator->();
    PolyMesh::HalfedgeHandle current();
    bool isNotEnd();
    bool isEnd();
};

void selectEdgeLoop(PolyMesh& mesh);

void extrude(PolyMesh& mesh, bool debug = false);
void loopCut(PolyMesh& mesh, bool debug = false);
void openRegion(PolyMesh& mesh, bool debug = false);
void bevel(PolyMesh& mesh, int segments = 0, float radius = 30.0f, bool debug = false);

bool rayTriangleIntersect(
    const glm::vec3 &orig, const glm::vec3 &dir,
    const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2,
    float &t);

void intersectMeshesOld(PolyMesh& a, PolyMesh& b);

enum class CurrentTransform {
    None,
    Move,
    Scale
};

class MeshViewer2D {
    float lastMouseX, lastMouseY;
    
public:
    PolyMesh* mesh;
    
    CurrentTransform currentTransform = CurrentTransform::None;
    bool shiftPressed = false;
    
    MeshViewer2D(PolyMesh* mesh);
    
    void draw(bvg::Context& ctx, bool isMouseDown, float mouseX, float mouseY);
};

struct CSGPolygon;
struct BSPNode;

struct CSGPlane {
    glm::vec3 normal;
    float w;
  
    static constexpr float Threshold = 1e-5;
    
    CSGPlane(glm::vec3 normal, float w);
    CSGPlane(glm::vec3 a, glm::vec3 b, glm::vec3 c);
    
    void splitPolygon(CSGPolygon& polygon,
                      std::list<CSGPolygon>& coplanarFront,
                      std::list<CSGPolygon>& coplanarBack,
                      std::list<CSGPolygon>& front,
                      std::list<CSGPolygon>& back);
    void flip();
};

//struct CSGSharedProps {
//
//};

struct CSGVertex {
    glm::vec3 pos, normal;
//    CSGSharedProps shared;
    
    struct DeleteInfo {
        int numUsers = 0;
    };
    DeleteInfo deleteInfo;
    PolyMesh::VertexHandle vh = PolyMesh::InvalidVertexHandle;
    
    CSGVertex* lerp(CSGVertex& other, float t);
    void flipNormal();
};

struct CSGPolygon {
    CSGPlane plane;
    std::vector<CSGVertex*> verts;
    
    CSGPolygon(std::vector<CSGVertex*>& verts);
    void flip();
    
    static std::list<CSGPolygon> extractFromMesh(PolyMesh& mesh);
    static PolyMesh toMesh(std::list<CSGPolygon> polygons);
};

struct BSPNode {
    CSGPlane* plane = nullptr;
    BSPNode* front = nullptr;
    BSPNode* back = nullptr;
    std::list<CSGPolygon> polygons;
    
    BSPNode();
    BSPNode(std::list<CSGPolygon>& polygons);
    std::list<CSGPolygon> allPolygons();
    std::list<CSGPolygon> clipPolygons(std::list<CSGPolygon>& polygons);
    void clipTo(BSPNode* node);
    void invert();
    void build(std::list<CSGPolygon>& polygons);
    
    static BSPNode* fromMesh(PolyMesh& mesh);
    static void deepDelete(std::list<BSPNode*> roots);
};

void unionBSP(BSPNode* a, BSPNode* b);
void subtractBSP(BSPNode* a, BSPNode* b);
void intersectBSP(BSPNode* a, BSPNode* b);

PolyMesh subtractMeshes(PolyMesh& a, PolyMesh& b);
