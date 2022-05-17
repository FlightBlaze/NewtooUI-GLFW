//
//  Mesh.h
//  MyProject
//
//  Created by Dmitry on 13.05.2022.
//

#pragma once

#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include <blazevg.hh>
#include <list>
#include <unordered_map>

typedef OpenMesh::PolyMesh_ArrayKernelT<> PolyMesh;

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

void extrude(PolyMesh& mesh, bool debug = false);

enum class CurrentTransform {
    None,
    Move,
    Scale
};

class MeshViewer {
    float lastMouseX, lastMouseY;
    
public:
    PolyMesh* mesh;
    
    CurrentTransform currentTransform = CurrentTransform::None;
    bool shiftPressed = false;
    
    MeshViewer(PolyMesh* mesh);
    
    void draw(bvg::Context& ctx, bool isMouseDown, float mouseX, float mouseY);
};
