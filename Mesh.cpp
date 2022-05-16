//
//  Mesh.cpp
//  MyProject
//
//  Created by Dmitry on 13.05.2022.
//

#include "Mesh.h"
#include <OpenMesh/Core/Utils/Predicates.hh>
#include <unordered_map>

std::list<PolyMesh::FaceHandle> getSelectedFaces(PolyMesh& mesh) {
    std::list<PolyMesh::FaceHandle> faces;
    std::unordered_map<PolyMesh::FaceHandle, bool> facesMap;
    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected())) {
        for (auto fh : mesh.vf_range(vh)) {
            bool allFaceVerticesSelected = true;
            for (auto fvh : mesh.fv_range(fh)) {
                if(!fvh.selected()) {
                    allFaceVerticesSelected = false;
                    break;
                }
            }
            if(allFaceVerticesSelected) {
                if(facesMap.find(fh) == facesMap.end()) {
                    faces.push_back(fh);
                    facesMap[fh] = true;
                }
            }
        }
    }
    return faces;
}

struct Duplicate {
    std::list<PolyMesh::VertexHandle> newVerts;
    std::unordered_map<PolyMesh::VertexHandle, PolyMesh::VertexHandle> originalCopyPairs;
};

Duplicate duplicate(PolyMesh& mesh) {
    std::list<PolyMesh::FaceHandle> selectedFaces = getSelectedFaces(mesh);
    Duplicate dup;
    
    // Copy vertices
    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected())) {
        PolyMesh::VertexHandle newVert = mesh.add_vertex(mesh.point(vh));
        dup.newVerts.push_back(newVert);
        dup.originalCopyPairs[vh] = newVert;
    }
    
    // Copy faces
    for(auto selFace : selectedFaces) {
        std::vector<PolyMesh::VertexHandle> newFaceVertices;
        for(auto fvh : mesh.fv_range(selFace)) {
            newFaceVertices.push_back(dup.originalCopyPairs[fvh]);
        }
        mesh.add_face(newFaceVertices);
    }
    
    return dup;
}

bool isHalfEdgeSelectionBoundary(PolyMesh::HalfedgeHandle heh, PolyMesh& mesh) {
    /*
         Case 1:       Case 2:
     
        D        D    S        S
         <------^      <------^
         |      |      |//////|
         |      |      |//////|
         V---C-->      V---C-->
        S        S    S        S
         <---T--^      <---T--^
         |//////|      |      |
         |//////|      |      |
         V------>      V------>
        S        S    D        D
     */
    PolyMesh::VertexHandle pointingVert = mesh.to_vertex_handle(heh);
    
    const bool isPointingVertSelected = mesh.status(pointingVert).selected();
//    const bool isVertexInBoundary = mesh.is_boundary(pointingVert);
//
//    if(isPointingVertSelected && isVertexInBoundary)
//        return true;
    
    PolyMesh::VertexHandle leftVert =
        mesh.to_vertex_handle(mesh.next_halfedge_handle(heh));
    
    PolyMesh::VertexHandle rightVert =
        mesh.from_vertex_handle(mesh.prev_halfedge_handle(mesh.opposite_halfedge_handle(heh)));
    
    const bool isLeftVertSelected = mesh.status(leftVert).selected();
    const bool isRightVertSelected = mesh.status(rightVert).selected();
    
    // If the current edge are between the perpendicular edge
    // with selected vertex and the one with not selected
    const bool case1 = isLeftVertSelected && !isRightVertSelected;
//    const bool case2 = !isLeftVertSelected && isRightVertSelected;
    return isPointingVertSelected && (case1);// || case2);
}

PolyMesh::HalfedgeHandle findSelectionBoundary(PolyMesh& mesh) {
    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected())) {
        PolyMesh::VertexOHalfedgeCWIter vhIt;
        for (vhIt = mesh.voh_cwiter(vh); vhIt.is_valid(); vhIt++) {
            if(isHalfEdgeSelectionBoundary(*vhIt, mesh)) {
                return *vhIt;
            }
        }
    }
    return PolyMesh::HalfedgeHandle();
}

void SelectionBoundaryIter::next() {
    this->wasNext = true;
    PolyMesh::VertexHandle vert = mesh.to_vertex_handle(cur);
    PolyMesh::VertexOHalfedgeCWIter vhIt;
    for (vhIt = mesh.voh_cwiter(vert); vhIt.is_valid(); vhIt++) {
        if(isHalfEdgeSelectionBoundary(*vhIt, mesh)) {
            cur = *vhIt;
            return;
        }
    }
    cur = PolyMesh::HalfedgeHandle();
}

SelectionBoundaryIter::SelectionBoundaryIter(PolyMesh::HalfedgeHandle heh, PolyMesh& mesh):
    start(heh), cur(heh), mesh(mesh) {}

void SelectionBoundaryIter::operator++(int n) {
    next();
}

PolyMesh::HalfedgeHandle SelectionBoundaryIter::operator->() {
    return cur;
}

PolyMesh::HalfedgeHandle SelectionBoundaryIter::current() {
    return cur;
}

bool SelectionBoundaryIter::isEnd() {
    return (cur == this->start && this->wasNext) || cur == PolyMesh::HalfedgeHandle();
}

bool SelectionBoundaryIter::isNotEnd() {
    return !this->isEnd();
}

std::list<std::list<PolyMesh::HalfedgeHandle>> findAllSelectionBoundaries(PolyMesh& mesh) {
    std::list<std::list<PolyMesh::HalfedgeHandle>> boundaries;
    std::unordered_map<PolyMesh::HalfedgeHandle, bool> knownHalfedges;
    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected())) {
        PolyMesh::VertexOHalfedgeCWIter hehIt;
        for (hehIt = mesh.voh_cwiter(vh); hehIt.is_valid(); hehIt++) {
            if(isHalfEdgeSelectionBoundary(*hehIt, mesh)) {
                if(knownHalfedges.find(*hehIt) == knownHalfedges.end()) {
                    SelectionBoundaryIter itSb = SelectionBoundaryIter(*hehIt, mesh);
                    std::list<PolyMesh::HalfedgeHandle> boundary;
                    for(; itSb.isNotEnd(); itSb++) {
                        boundary.push_back(itSb.current());
                        knownHalfedges[itSb.current()] = true;
                    }
                    boundaries.push_back(boundary);
                }
            }
        }
    }
    return boundaries;
}

void extrude(PolyMesh& mesh) {
    // Separate selected faces
    Duplicate top = duplicate(mesh);
    
    // Delete vertices inside foundation boundaries
    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected())) {
        if(!mesh.is_boundary(top.originalCopyPairs[vh])) {
            mesh.delete_vertex(vh);
        }
    }
    
    // Move top inside
    glm::vec2 centroid;
    int numVertices = 0;
    for (auto vh : top.newVerts) {
        numVertices++;
        PolyMesh::Point point = mesh.point(vh);
        centroid += glm::vec2(point[0], point[1]);
    }
    centroid /= (float)numVertices;
    for(auto vh : top.newVerts) {
        PolyMesh::Point newPoint = mesh.point(vh);
        glm::vec2 pos = glm::vec2(newPoint[0], newPoint[1]);
        glm::vec2 relativePos = pos - centroid;
        relativePos *= 0.5f;
        glm::vec2 newPos = centroid + relativePos;
        newPoint[0] = newPos.x;
        newPoint[1] = newPos.y;
        mesh.set_point(vh, newPoint);
    }
    
    std::list<PolyMesh::HalfedgeHandle> selectBound;
    for(SelectionBoundaryIter itSb = SelectionBoundaryIter(findSelectionBoundary(mesh), mesh);
        itSb.isNotEnd(); itSb++) {
        selectBound.push_back(itSb.current());
        mesh.status(itSb.current()).set_selected(true);
    }
    
   // Delete bottom faces
   for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected())) {
       for(auto fh : mesh.vf_range(vh)) {
           bool isAllFaceVertsSelected = true;
           for(auto vfh : mesh.fv_range(fh)) {
               if(!vfh.selected()) {
                   isAllFaceVertsSelected = false;
                   break;
               }
           }
           if(isAllFaceVertsSelected) {
               mesh.delete_face(fh);
           }
       }
   }
    
    // Bridge selection boundary to separated faces
    for(PolyMesh::HalfedgeHandle heh : selectBound) {
        PolyMesh::VertexHandle edgeBottomOrigin = mesh.from_vertex_handle(heh);
        PolyMesh::VertexHandle edgeBottomEnd = mesh.to_vertex_handle(heh);
        PolyMesh::VertexHandle edgeTopOrigin = top.originalCopyPairs[edgeBottomOrigin];
        PolyMesh::VertexHandle edgeTopEnd = top.originalCopyPairs[edgeBottomEnd];
        std::vector<PolyMesh::VertexHandle> faceVerts = {
            edgeBottomOrigin, edgeBottomEnd, edgeTopEnd, edgeTopOrigin
//            edgeTopOrigin, edgeTopEnd, edgeBottomEnd, edgeBottomOrigin
        };
        mesh.add_face(faceVerts);
    }
    
    // Change selection to the top of extrusion
    std::list<PolyMesh::VertexHandle> oldSel;
    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected())) {
        oldSel.push_back(vh);
    }
    for(auto vh: oldSel) {
        mesh.status(vh).set_selected(false);
    }
    for(auto vh : top.newVerts) {
        mesh.status(vh).set_selected(true);
    }
}

MeshViewer::MeshViewer(PolyMesh* mesh):
    mesh(mesh) {}

void MeshViewer::draw(bvg::Context& ctx, bool isMouseDown, float mouseX, float mouseY) {
    if(mesh == nullptr)
        return;
    
    if(currentTransform != CurrentTransform::None) {
        float deltaX = mouseX - lastMouseX;
        float deltaY = mouseY - lastMouseY;
        
        glm::vec2 centroid;
        int numVertices = 0;
        for (auto vh : mesh->vertices().filtered(OpenMesh::Predicates::Selected())) {
            numVertices++;
            PolyMesh::Point point = mesh->point(vh);
            centroid += glm::vec2(point[0], point[1]);
        }
        centroid /= (float)numVertices;
        
//        float maxDistanceCentroid;
//        for (auto vh : mesh->vertices().filtered(OpenMesh::Predicates::Selected())) {
//            PolyMesh::Point point = mesh->point(vh);
//            glm::vec2 pos = glm::vec2(point[0], point[1]);
//            float distance = glm::distance(centroid, pos);
//            if(distance > maxDistanceCentroid)
//                maxDistanceCentroid = distance;
//        }
        
        for (auto vh : mesh->vertices().filtered(OpenMesh::Predicates::Selected())) {
            switch (currentTransform) {
                case CurrentTransform::Move:
                {
                    PolyMesh::Point newPoint = mesh->point(vh);
                    newPoint[0] += deltaX;
                    newPoint[1] += deltaY;
                    mesh->set_point(vh, newPoint);
                }
                    break;
                 
                case CurrentTransform::Scale:
                {
                    PolyMesh::Point newPoint = mesh->point(vh);
                    glm::vec2 pos = glm::vec2(newPoint[0], newPoint[1]);
                    glm::vec2 relativePos = pos - centroid;
                    relativePos *= 1.0f + deltaX * 0.01f;
                    glm::vec2 newPos = centroid + relativePos;
                    newPoint[0] = newPos.x;
                    newPoint[1] = newPos.y;
                    mesh->set_point(vh, newPoint);
                }
                    break;
                    
                    
                default:
                    break;
            }
        }
    }
    
    bvg::Style halfEdgeStyle = bvg::SolidColor(bvg::colors::Black);
    bvg::Style boundaryHalfEdgeStyle = bvg::SolidColor(bvg::Color(1.0f, 0.0f, 0.0f));
    bvg::Style debugHalfEdgeStyle = bvg::SolidColor(bvg::Color(0.0f, 1.0f, 0.0f));
    bvg::Style selectedVertexStyle = bvg::SolidColor(bvg::Color(1.0f, 0.5f, 0.0f));
    
    ctx.fillStyle = bvg::SolidColor(bvg::colors::Black);
    ctx.strokeStyle = bvg::SolidColor(bvg::colors::Black);
    ctx.lineWidth = 2.0f;
    PolyMesh::VertexIter vIt, vEnd(mesh->vertices_end());
    for (vIt=mesh->vertices_begin(); vIt!=vEnd; vIt++) {
        PolyMesh::Point vertPoint = mesh->point(*vIt);
        glm::vec2 vertPos = glm::vec2(vertPoint[0], vertPoint[1]);
        
        ctx.beginPath();
        ctx.arc(vertPos.x, vertPos.y, 14.0f, 0.0f, M_PI * 2.0f);
        
        bool isMouseInside = ctx.isPointInsideFill(mouseX, mouseY);
        if(isMouseInside) {
            ctx.fillStyle = bvg::SolidColor(bvg::Color(0.2f, 0.2f, 0.2f));
            if(isMouseDown) {
                if(!shiftPressed) {
                    for (auto vh : mesh->vertices().filtered(OpenMesh::Predicates::Selected())) {
                        PolyMesh::StatusInfo& vertStatus = mesh->status(vh);
                        vertStatus.set_selected(false);
                    }
                }
                PolyMesh::StatusInfo& vertStatus = mesh->status(*vIt);
                vertStatus.set_selected(true);
            }
        } else {
            ctx.fillStyle = bvg::SolidColor(bvg::colors::Black);
        }
        
        if(vIt->selected()) {
            ctx.fillStyle = selectedVertexStyle;
        }
        
        ctx.beginPath();
        ctx.arc(vertPos.x, vertPos.y, 8.0f, 0.0f, M_PI * 2.0f);
        ctx.convexFill();
        
//        if(vert->edge == nullptr)
//            continue;
//        int i = 0;
        
        PolyMesh::VertexOHalfedgeCWIter hehIt;
        for (hehIt = mesh->voh_cwiter( *vIt ); hehIt.is_valid(); hehIt++) {
            
            PolyMesh::Point originPoint = mesh->point(hehIt->from());
            glm::vec2 originPos = glm::vec2(originPoint[0], originPoint[1]);
            
            PolyMesh::Point destPoint = mesh->point(hehIt->to());
            glm::vec2 destPos = glm::vec2(destPoint[0], destPoint[1]);
            
            glm::vec2 start = originPos;
            glm::vec2 end = destPos;
            glm::vec2 dir = glm::normalize(end - start);
            glm::vec2 dirPerp = glm::vec2(dir.y, -dir.x);
            glm::vec2 offset = dirPerp * -4.0f;
            start += offset + dir * 12.0f;
            end += offset - dir * 12.0f;
            glm::vec2 arrowPoint = end - dir * 12.0f - dirPerp * 8.0f;
            ctx.beginPath();
            ctx.moveTo(start.x, start.y);
            ctx.lineTo(end.x, end.y);
            ctx.lineTo(arrowPoint.x, arrowPoint.y);
            
            ctx.strokeStyle = halfEdgeStyle;
            if(!hehIt->is_boundary()) {
                ctx.strokeStyle = halfEdgeStyle;
            } else {
                ctx.strokeStyle = boundaryHalfEdgeStyle;
            }
            
            if(hehIt->selected()) {
                ctx.strokeStyle = debugHalfEdgeStyle;
            }
            ctx.stroke();
            
//            i++;
//            if(i == 10)
//                break;
        }
    }
    lastMouseX = mouseX;
    lastMouseY = mouseY;
}
