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

std::unordered_map<PolyMesh::FaceHandle, bool> getSelectedFacesMap(std::list<PolyMesh::FaceHandle> list) {
    std::unordered_map<PolyMesh::FaceHandle, bool> map;
    for (auto fh : list) {
        map[fh] = true;
    }
    return map;
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

bool isHalfEdgeSelectionBoundary(PolyMesh::HalfedgeHandle heh, PolyMesh& mesh,
                                 std::unordered_map<PolyMesh::FaceHandle, bool> selectedFacesMap) {
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
    const bool isVertInBoundary = mesh.is_boundary(pointingVert);
    const bool isHalfedgeIsBoundary = mesh.face_handle(heh) == PolyMesh::InvalidFaceHandle;
    const bool isHalfedgeIsBelongToSelectedFace =
        selectedFacesMap.find(mesh.face_handle(heh)) != selectedFacesMap.end();
    const bool isSelectedAnyFace = selectedFacesMap.size() > 0;
    
    if(!isSelectedAnyFace && isPointingVertSelected && isVertInBoundary && isHalfedgeIsBoundary)
        return true;
    
    if(!isHalfedgeIsBelongToSelectedFace)
        return false;
    
    if(isPointingVertSelected && isVertInBoundary && !isHalfedgeIsBoundary)
        return true;
    
    PolyMesh::VertexHandle leftFrontVert =
        mesh.to_vertex_handle(mesh.next_halfedge_handle(heh));
    PolyMesh::VertexHandle rightFrontVert =
        mesh.from_vertex_handle(mesh.prev_halfedge_handle(mesh.opposite_halfedge_handle(heh)));
    
    PolyMesh::VertexHandle leftBackVert =
        mesh.from_vertex_handle(mesh.prev_halfedge_handle(heh));
    PolyMesh::VertexHandle rightBackVert =
        mesh.to_vertex_handle(mesh.next_halfedge_handle(mesh.opposite_halfedge_handle(heh)));
    
    const bool isLeftFrontVertSelected = mesh.status(leftFrontVert).selected();
    const bool isRightFrontVertSelected = mesh.status(rightFrontVert).selected();
    
    const bool isLeftBackVertSelected = mesh.status(leftBackVert).selected();
    const bool isRightBackVertSelected = mesh.status(rightBackVert).selected();
    
    // If the current edge are between the perpendicular edge
    // with selected vertex and the one with not selected
    const bool case1_1 = isLeftFrontVertSelected && !isRightFrontVertSelected;
    const bool case1_2 = isLeftBackVertSelected && !isRightBackVertSelected;
//    const bool case2 = !isLeftVertSelected && isRightVertSelected;
    return isPointingVertSelected && (case1_1 || case1_2);//(case1 || case2);
}

PolyMesh::HalfedgeHandle findSelectionBoundary(PolyMesh& mesh) {
    std::unordered_map<PolyMesh::FaceHandle, bool> selFacesMap =
        getSelectedFacesMap(getSelectedFaces(mesh));
    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected())) {
        PolyMesh::VertexOHalfedgeCWIter vhIt;
        for (vhIt = mesh.voh_cwiter(vh); vhIt.is_valid(); vhIt++) {
            if(isHalfEdgeSelectionBoundary(*vhIt, mesh, selFacesMap)) {
                return *vhIt;
            }
        }
    }
    return PolyMesh::HalfedgeHandle();
}

void SelectionBoundaryIter::next() {
    this->wasNext = true;
    step++;
    PolyMesh::VertexHandle vert = mesh.to_vertex_handle(cur);
    PolyMesh::VertexOHalfedgeCWIter hehIt;
    for (hehIt = mesh.voh_cwiter(vert); hehIt.is_valid(); hehIt++) {
        if(traversedHalfedges[*hehIt] > 0)
            continue;
        if(isHalfEdgeSelectionBoundary(*hehIt, mesh, selectedFacesMap)) {
            cur = *hehIt;
            traversedHalfedges[*hehIt]++;
            return;
        }
    }
    cur = PolyMesh::HalfedgeHandle();
}

SelectionBoundaryIter::SelectionBoundaryIter(PolyMesh::HalfedgeHandle heh, PolyMesh& mesh,
                                             std::unordered_map<PolyMesh::FaceHandle, bool>& selectedFacesMap):
    start(heh), cur(heh), mesh(mesh), selectedFacesMap(selectedFacesMap) {}

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
    return (cur == this->start && this->wasNext) || cur == PolyMesh::HalfedgeHandle();// || step > 15;
}

bool SelectionBoundaryIter::isNotEnd() {
    return !this->isEnd();
}

bool isNotSameFace(PolyMesh::FaceHandle a, PolyMesh::FaceHandle b) {
    if(a == PolyMesh::InvalidFaceHandle || b == PolyMesh::InvalidFaceHandle)
        return true;
    
    return a != b;
}

void EdgeLoopIter::next() {
    this->wasNext = true;
    /*
     |<-------^|<-------^
     |        ||        |
     |   f3   N|   f4   |
     | Other  || Other  |
     V------->|V------->|
     |<-------^|<-------^
     |        ||        |
     |   f1   C|   f2   |
     |  Same  ||  Same  |
     V------->|V------->|
     */
    const PolyMesh::FaceHandle curFace = mesh.face_handle(cur);
    const PolyMesh::FaceHandle twinFace =
        mesh.face_handle(mesh.opposite_halfedge_handle(cur));
    
    PolyMesh::VertexHandle vert = mesh.to_vertex_handle(cur);
    
    PolyMesh::VertexOHalfedgeCWIter hehIt;
    for (hehIt = mesh.voh_cwiter(vert); hehIt.is_valid(); hehIt++) {
        const auto twinHeh = hehIt->opp();
        const PolyMesh::FaceHandle curHeFace = hehIt->face();
        const PolyMesh::FaceHandle curTwinFace = twinHeh.face();
        
        // If this edge is not sharing the same
        // faces as the current edge
        if(isNotSameFace(curHeFace, curFace) && isNotSameFace(curHeFace, twinFace) &&
           isNotSameFace(curTwinFace, curFace) && isNotSameFace(curTwinFace, twinFace)) {
            this->cur = *hehIt;
            return;
        }
    }
    this->cur = PolyMesh::InvalidHalfedgeHandle;
}

EdgeLoopIter::EdgeLoopIter(PolyMesh::HalfedgeHandle heh, PolyMesh& mesh):
    start(heh), cur(heh), mesh(mesh) {}

void EdgeLoopIter::operator++(int n) {
    next();
}

PolyMesh::HalfedgeHandle EdgeLoopIter::operator->() {
    return cur;
}

PolyMesh::HalfedgeHandle EdgeLoopIter::current() {
    return cur;
}

bool EdgeLoopIter::isEnd() {
    return (cur == this->start && this->wasNext) || cur == PolyMesh::InvalidHalfedgeHandle;
}

bool EdgeLoopIter::isNotEnd() {
    return !this->isEnd();
}

void cleanSelectionBoundary(std::unordered_map<PolyMesh::HalfedgeHandle, bool>& knownHalfedges,
                            std::list<PolyMesh::HalfedgeHandle>& boundary, PolyMesh& mesh) {
    for(auto hehIt = boundary.begin(); hehIt != boundary.end(); hehIt++) {
        bool doubleSelected =
            knownHalfedges.find(mesh.opposite_halfedge_handle(*hehIt)) != knownHalfedges.end() &&
            knownHalfedges.find(*hehIt) != knownHalfedges.end();
        if(doubleSelected) {
            /*
             ^------> ^------>
             |      T C      |
             |      | |      |
             <------v <------v
             */
//            auto sheh = OpenMesh::make_smart(*hehIt, mesh);
//
//            bool isBetweenSelectedVerts =
//                (sheh.next().to().selected() || sheh.opp().prev().from().selected()) &&
//                (sheh.prev().from().selected() || sheh.opp().next().to().selected());
//
//            if(!isBetweenSelectedVerts) {
//                mesh.status(mesh.opposite_halfedge_handle(*hehIt)).set_selected(false);
//            }
            boundary.erase(hehIt);
        }
    }
}

std::list<std::list<PolyMesh::HalfedgeHandle>> findAllSelectionBoundaries(PolyMesh& mesh) {
    std::unordered_map<PolyMesh::FaceHandle, bool> selFacesMap =
        getSelectedFacesMap(getSelectedFaces(mesh));
    std::list<std::list<PolyMesh::HalfedgeHandle>> boundaries;
    std::unordered_map<PolyMesh::HalfedgeHandle, bool> knownHalfedges;
    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected())) {
        PolyMesh::VertexOHalfedgeCWIter hehIt;
        for (hehIt = mesh.voh_cwiter(vh); hehIt.is_valid(); hehIt++) {
            if(isHalfEdgeSelectionBoundary(*hehIt, mesh, selFacesMap)) {
                if(knownHalfedges.find(*hehIt) == knownHalfedges.end()) {
                    SelectionBoundaryIter itSb = SelectionBoundaryIter(*hehIt, mesh, selFacesMap);
                    std::list<PolyMesh::HalfedgeHandle> boundary;
                    for(; itSb.isNotEnd(); itSb++) {
                        boundary.push_back(itSb.current());
                        knownHalfedges[itSb.current()] = true;
                    }
                    cleanSelectionBoundary(knownHalfedges, boundary, mesh);
                    boundaries.push_back(boundary);
                }
            }
        }
    }
    return boundaries;
}

void extrude(PolyMesh& mesh, bool debug) {
    // Find all selection boundaries
    std::list<std::list<PolyMesh::HalfedgeHandle>> selectBounds = findAllSelectionBoundaries(mesh);
    for (auto bound : selectBounds) {
        for (PolyMesh::HalfedgeHandle heh : bound)
            mesh.status(heh).set_selected(true);
    }
    
    if(debug)
        return;

    // Separate selected faces
    Duplicate top = duplicate(mesh);

    // Delete vertices inside foundation boundaries
    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected())) {
        if(!mesh.is_boundary(top.originalCopyPairs[vh])) {
            mesh.delete_vertex(vh);
        }
    }
//
//    // Move top inside
//    glm::vec2 centroid;
//    int numVertices = 0;
//    for (auto vh : top.newVerts) {
//        numVertices++;
//        PolyMesh::Point point = mesh.point(vh);
//        centroid += glm::vec2(point[0], point[1]);
//    }
//    centroid /= (float)numVertices;
//    for(auto vh : top.newVerts) {
//        PolyMesh::Point newPoint = mesh.point(vh);
//        glm::vec2 pos = glm::vec2(newPoint[0], newPoint[1]);
//        glm::vec2 relativePos = pos - centroid;
//        relativePos *= 0.5f;
//        glm::vec2 newPos = centroid + relativePos;
//        newPoint[0] = newPos.x;
//        newPoint[1] = newPos.y;
//        mesh.set_point(vh, newPoint);
//    }
    
   // Delete bottom faces
    std::list<PolyMesh::FaceHandle> facesToDelete;
    std::unordered_map<PolyMesh::FaceHandle, bool> knownFacesToDelete;
    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected())) {
        for(auto fh : mesh.vf_range(vh)) {
            if(knownFacesToDelete.find(fh) != knownFacesToDelete.end())
                continue;
            knownFacesToDelete[fh] = true;
            bool isAllFaceVertsSelected = true;
            for(auto vfh : mesh.fv_range(fh)) {
                if(!vfh.selected()) {
                    isAllFaceVertsSelected = false;
                    break;
                }
            }
            if(isAllFaceVertsSelected) {
                facesToDelete.push_back(fh);
            }
        }
    }
    for(auto fh : facesToDelete)
        mesh.delete_face(fh, false);
    
    // Bridge selection boundary to separated faces
    for (auto bound : selectBounds) {
        for(PolyMesh::HalfedgeHandle heh : bound) {
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

std::list<PolyMesh::HalfedgeHandle> getLoopHalfedges(PolyMesh::HalfedgeHandle heh, PolyMesh& mesh) {
    std::list<PolyMesh::HalfedgeHandle> loop;
    PolyMesh::HalfedgeHandle twin = mesh.opposite_halfedge_handle(heh);
    PolyMesh::HalfedgeHandle end = PolyMesh::InvalidHalfedgeHandle;
    for(auto hehIt = EdgeLoopIter(twin, mesh); hehIt.isNotEnd(); hehIt++) {
        end = hehIt.current();
    }
    PolyMesh::HalfedgeHandle start = mesh.opposite_halfedge_handle(end);
    for(auto hehIt = EdgeLoopIter(start, mesh); hehIt.isNotEnd(); hehIt++) {
        loop.push_back(hehIt.current());
    }
    return loop;
}

void loopCut(PolyMesh& mesh, bool debug) {
    std::vector<PolyMesh::VertexHandle> selVerts;
    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected()))
        selVerts.push_back(vh);
    
    if(selVerts.size() < 2)
        return;
    
    PolyMesh::HalfedgeHandle heh = mesh.find_halfedge(selVerts[0], selVerts[1]);
    if(heh == PolyMesh::InvalidHalfedgeHandle)
        return;
    
    OpenMesh::SmartHalfedgeHandle sheh = OpenMesh::make_smart(heh, mesh);
    
    /*
     
     |------^
     |      |
     v--C-->|
     
     */
    
    OpenMesh::SmartHalfedgeHandle left = sheh.prev().opp();
    OpenMesh::SmartHalfedgeHandle right = sheh.next();
    
    for(auto heh : getLoopHalfedges(left, mesh)) {
        mesh.status(heh).set_selected(true);
    }
    for(auto heh : getLoopHalfedges(right, mesh)) {
        mesh.status(heh).set_selected(true);
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
        if(vIt->deleted())
            continue;
        
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
            
            //mesh->texcoord2D(*hehIt)
            
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
