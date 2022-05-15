//
//  Mesh.h
//  MyProject
//
//  Created by Dmitry on 13.05.2022.
//

#pragma once

#include <OpenMesh/Core/Mesh/PolyMesh_ArrayKernelT.hh>
#include <blazevg.hh>

typedef OpenMesh::PolyMesh_ArrayKernelT<> PolyMesh;

class MeshViewer {
public:
    PolyMesh* mesh;
    
    bool shiftPressed = false;
    
    MeshViewer(PolyMesh* mesh);
    
    void draw(bvg::Context& ctx, bool isMouseDown, float mouseX, float mouseY);
};
