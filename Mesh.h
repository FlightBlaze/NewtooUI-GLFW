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

typedef OpenMesh::PolyMesh_ArrayKernelT<> PolyMesh;

class SelectionBoundaryIter {
    PolyMesh::HalfedgeHandle start;
    PolyMesh::HalfedgeHandle cur;
    PolyMesh& mesh;
    bool wasNext = false;
    
    void next();
    
public:
    SelectionBoundaryIter(PolyMesh::HalfedgeHandle heh, PolyMesh& mesh);
    void operator++(int n);
    PolyMesh::HalfedgeHandle operator->();
    PolyMesh::HalfedgeHandle current();
    bool isNotEnd();
    bool isEnd();
};

void extrude(PolyMesh& mesh);

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
