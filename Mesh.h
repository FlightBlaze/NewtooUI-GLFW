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
