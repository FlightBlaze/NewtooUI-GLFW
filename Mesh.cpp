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
    PolyMesh::VertexHandle originVert = mesh.from_vertex_handle(heh);
    
    const bool isPointingVertSelected = mesh.status(pointingVert).selected();
    const bool isVertInBoundary = mesh.is_boundary(pointingVert);
    const bool isOriginInBoundary = mesh.is_boundary(originVert);
    const bool isHalfedgeIsBoundary = mesh.face_handle(heh) == PolyMesh::InvalidFaceHandle;
    const bool isHalfedgeIsBelongToSelectedFace =
        selectedFacesMap.find(mesh.face_handle(heh)) != selectedFacesMap.end();
    const bool isSelectedAnyFace = selectedFacesMap.size() > 0;
    
    if(!isSelectedAnyFace && isPointingVertSelected && isVertInBoundary && isHalfedgeIsBoundary)
        return true;
    
    if(!isHalfedgeIsBelongToSelectedFace)
        return false;
    
    if(isPointingVertSelected && isVertInBoundary &&
       isOriginInBoundary && !isHalfedgeIsBoundary) {
        return true;
    }
    
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
            PolyMesh::FaceHandle face = mesh.add_face(faceVerts);
            mesh.calc_normal(face);
        }
    }
    
    // Calculate vertex normals
    for (auto bound : selectBounds) {
        for(PolyMesh::HalfedgeHandle heh : bound) {
            PolyMesh::VertexHandle edgeBottomOrigin = mesh.from_vertex_handle(heh);
            PolyMesh::VertexHandle edgeBottomEnd = mesh.to_vertex_handle(heh);
            PolyMesh::VertexHandle edgeTopOrigin = top.originalCopyPairs[edgeBottomOrigin];
            PolyMesh::VertexHandle edgeTopEnd = top.originalCopyPairs[edgeBottomEnd];
            if(mesh.is_boundary(edgeBottomOrigin)) {
                mesh.calc_normal(edgeBottomOrigin);
                mesh.calc_normal(edgeTopOrigin);
            }
            mesh.calc_normal(edgeBottomEnd);
            mesh.calc_normal(edgeTopEnd);
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

std::list<PolyMesh::HalfedgeHandle> getRingHalfedges(PolyMesh::HalfedgeHandle heh, PolyMesh& mesh) {
    OpenMesh::SmartHalfedgeHandle start = OpenMesh::make_smart(heh, mesh);
    
    // Start halfedge must be inside the face
    if(start.face() == PolyMesh::InvalidFaceHandle)
        start = start.opp();
    
    /*
    
    ->| v-N-->
    --^ <----^
      L |    R
    ->| v-C-->
     
     */
    
    std::unordered_map<PolyMesh::HalfedgeHandle, bool> knownHalfedges;
    std::list<PolyMesh::HalfedgeHandle> ring;
    
    // Forward
    OpenMesh::SmartHalfedgeHandle he = start;
    do {
        ring.push_back(he);
        knownHalfedges[he] = true;
        mesh.status(he).set_selected(true);
        if(he.is_boundary()) {
            break;
        }
        he = he.next().next().opp();
    } while (he != start);
    
    // Backward
    he = start;
    do {
        he = he.opp().prev().prev();
        if(knownHalfedges.find(he) != knownHalfedges.end()) {
            break;
        }
        if(he.is_boundary()) {
            break;
        }
        ring.push_front(he);
        knownHalfedges[he] = true;
        mesh.status(he).set_selected(true);
    } while (he != start);
    
    return ring;
}

void selectEdgeLoop(PolyMesh& mesh) {
    std::vector<PolyMesh::VertexHandle> selVerts;
    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected()))
        selVerts.push_back(vh);
    
    if(selVerts.size() < 2)
        return;
    
    PolyMesh::HalfedgeHandle heh = mesh.find_halfedge(selVerts[0], selVerts[1]);
    if(heh == PolyMesh::InvalidHalfedgeHandle)
        return;
    
    for(auto lh : getLoopHalfedges(heh, mesh))
        mesh.status(lh).set_selected(true);
}

void loopCut(PolyMesh& mesh, bool debug) {
    std::vector<PolyMesh::VertexHandle> selVerts;
    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected()))
        selVerts.push_back(vh);
    
    if(selVerts.size() != 2)
        return;
    
    PolyMesh::HalfedgeHandle heh = mesh.find_halfedge(selVerts[0], selVerts[1]);
    if(heh == PolyMesh::InvalidHalfedgeHandle)
        return;
    
    std::list<PolyMesh::HalfedgeHandle> ring = getRingHalfedges(heh, mesh);
    
    if(debug) {
        for (auto he : ring) {
            mesh.status(he).set_selected(true);
        }
        return;
    }
    
    std::vector<PolyMesh::VertexHandle> A, B, C;
    
    for (auto he : ring) {
        PolyMesh::VertexHandle from = mesh.from_vertex_handle(he);
        PolyMesh::VertexHandle to = mesh.to_vertex_handle(he);
        PolyMesh::Point mean = (mesh.point(from) + mesh.point(to)) / 2.0f;
        A.push_back(from);
        B.push_back(mesh.add_vertex(mean));
        C.push_back(to);
    }
    // To connect start with end
    A.push_back(A.front());
    B.push_back(B.front());
    C.push_back(C.front());
    
    for (auto he : ring) {
        PolyMesh::FaceHandle face = mesh.face_handle(he);
        if(face != PolyMesh::InvalidFaceHandle) {
            mesh.delete_face(face, false);
        }
    }
    
//    return;
    
    for(int i = 0; i < A.size() - 1; i++) {
        /*
            a1 - b1 - c1
             |    |    |
            a2 - b2 - c2
         */
        PolyMesh::VertexHandle a1 = A.at(i);
        PolyMesh::VertexHandle a2 = A.at(i + 1);
        PolyMesh::VertexHandle b1 = B.at(i);
        PolyMesh::VertexHandle b2 = B.at(i + 1);
        PolyMesh::VertexHandle c1 = C.at(i);
        PolyMesh::VertexHandle c2 = C.at(i + 1);
        std::vector<PolyMesh::VertexHandle> face;
        face = {
            a1, b1, b2, a2
        };
        mesh.add_face(face);
        face = {
            b1, c1, c2, b2
        };
        mesh.add_face(face);
    }
    
    for(auto vh : selVerts)
        mesh.status(vh).set_selected(false);
    
    for(auto vh : B)
        mesh.status(vh).set_selected(true);
}

void swapFaceVertex(PolyMesh::FaceHandle face, PolyMesh::VertexHandle a,
                    PolyMesh::VertexHandle b, PolyMesh& mesh) {
    std::vector<PolyMesh::VertexHandle> newFaceVerts;
    for(auto fvh : mesh.fv_ccw_range(face))
        newFaceVerts.push_back(fvh != a? fvh : b);
    mesh.delete_face(face, false);
    mesh.add_face(newFaceVerts);
}

PolyMesh::VertexHandle rip(PolyMesh::HalfedgeHandle incomingHalfedge, PolyMesh& mesh,
                           bool isOnBoundary = false) {
    /*
        *        *        *
         ^------> ^------>
         |      C |      |
         |      | | face1|
         <------v ^------>
        *        &        *
         ^------> ^------>
         |      N |      |
         |      | | face2|
         <------v ^------>
        *        *        *
     */
    OpenMesh::SmartHalfedgeHandle heh = OpenMesh::make_smart(incomingHalfedge, mesh);
    
    PolyMesh::VertexHandle vh = mesh.to_vertex_handle(heh);
    PolyMesh::VertexHandle dupVert = mesh.add_vertex(mesh.point(vh));
    
    OpenMesh::SmartHalfedgeHandle nextHeh = heh.next().opp().next();
    if(isOnBoundary)
        nextHeh = heh.next();
    
    PolyMesh::FaceHandle leftFace1 = heh.face();
    PolyMesh::FaceHandle leftFace2 = nextHeh.face();
    
    std::list<PolyMesh::FaceHandle> vertFacesToSwapVert;
    for(auto vfh : mesh.vf_cw_range(vh)) {
        if(vfh != leftFace1 && vfh != leftFace2)
            vertFacesToSwapVert.push_back(vfh);
    }
    for(auto vfh : vertFacesToSwapVert) {
        swapFaceVertex(vfh, vh, dupVert, mesh);
    }
    
    return dupVert;
}

//std::list<std::list<PolyMesh::HalfedgeHandle>> traceSelection(PolyMesh& mesh) {
//    std::list<std::list<PolyMesh::HalfedgeHandle>> boundaries;
//    std::unordered_map<PolyMesh::HalfedgeHandle, bool> knownHalfedges;
//    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected())) {
//        PolyMesh::VertexOHalfedgeCWIter hehIt;
//        for (hehIt = mesh.voh_cwiter(vh); hehIt.is_valid(); hehIt++) {
//            if(mesh.status(mesh.to_vertex_handle(*hehIt)).selected()) {
//                if(knownHalfedges.find(*hehIt) == knownHalfedges.end()) {
//                    std::list<PolyMesh::HalfedgeHandle> boundary;
//                    PolyMesh::HalfedgeHandle cur = *hehIt;
//                    do {
//                        if(knownHalfedges.find(cur) != knownHalfedges.end())
//                            break;
//                        bool nextFound = false;
//                        PolyMesh::VertexOHalfedgeCWIter nextHehIt;
//                        for (nextHehIt = mesh.voh_cwiter(mesh.to_vertex_handle(cur));
//                             nextHehIt.is_valid(); nextHehIt++) {
//                            if(mesh.status(mesh.to_vertex_handle(*nextHehIt)).selected()) {
//                                nextFound = true;
//                                cur = *nextHehIt;
//                                break;
//                            }
//                        }
//                        if(!nextFound)
//                            break;
//                        boundary.push_back(cur);
//                        knownHalfedges[cur] = true;
//                    } while(cur != *hehIt);
//                    cleanSelectionBoundary(knownHalfedges, boundary, mesh);
//                    boundaries.push_back(boundary);
//                }
//            }
//        }
//    }
//    return boundaries;
//}

void openRegion(PolyMesh& mesh, bool debug) {
//    int numSel = 0;
//    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected())) {
//        numSel++;
//    }
//
//    if(numSel < 2)
//        return;
//
//    for(auto heh : mesh.halfedges())
//        mesh.status(heh).set_selected(false);
//
//    std::list<std::list<PolyMesh::HalfedgeHandle>> traced = traceSelection(mesh);
//    for(auto boundary : traced) {
//        for(auto heh : boundary) {
//            mesh.status(heh).set_selected(true);
//        }
//    }
//
//    return;
    
    std::list<PolyMesh::VertexHandle> dupVerts;
    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected()))
        dupVerts.push_back(rip(vh.in(), mesh));
    
    // Switch selection to duplicated verts
    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected()))
        mesh.status(vh).set_selected(false);
    for(auto vh : dupVerts)
        mesh.status(vh).set_selected(true);
}

//PolyMesh::Point getFaceCentroid(PolyMesh::FaceHandle face, PolyMesh& mesh) {
//    PolyMesh::Point point;
//    int valence = 0;
//    for(auto fvh : mesh.fv_cw_range(face)) {
//        point += mesh.point(fvh);
//        valence++;
//    }
//    point /= valence;
//    return point;
//}

PolyMesh::Point lerpPoint(PolyMesh::Point a, PolyMesh::Point b, float t) {
    return a + (b - a) * t;
}

// Quadratic bezier without end points
std::vector<PolyMesh::Point> quadraticBezierInBetween(PolyMesh::Point p0, PolyMesh::Point p1,
                                                      PolyMesh::Point p2, int numSegments) {
    auto points = std::vector<PolyMesh::Point>(numSegments);
    float step = 1.0f / (numSegments + 1);
    float t = step;
    for (int i = 0; i < numSegments; i++) {
        PolyMesh::Point q0 = lerpPoint(p0, p1, t);
        PolyMesh::Point q1 = lerpPoint(p1, p2, t);
        PolyMesh::Point r  = lerpPoint(q0, q1, t);
        points[i] = r;
        t += step;
    }
    return points;
}

std::vector<PolyMesh::VertexHandle> makeBevelSegments(PolyMesh::Point p0, PolyMesh::Point p1,
                                                      PolyMesh::Point p2, int numSegments,
                                                      PolyMesh& mesh) {
    std::vector<PolyMesh::Point> points = quadraticBezierInBetween(p0, p1, p2, numSegments);
    std::vector<PolyMesh::VertexHandle> vertices;
    vertices.reserve(points.size());
    for(auto point : points)
        vertices.push_back(mesh.add_vertex(point));
    return vertices;
}

void bevel(PolyMesh& mesh, int segments, float radius, bool debug) {
    std::vector<PolyMesh::VertexHandle> selVerts;
    for (auto vh : mesh.vertices().filtered(OpenMesh::Predicates::Selected()))
        selVerts.push_back(vh);
    
    if(selVerts.size() < 2)
        return;
    
    PolyMesh::HalfedgeHandle heh = mesh.find_halfedge(selVerts[0], selVerts[1]);
    if(heh == PolyMesh::InvalidHalfedgeHandle)
        return;
    
    std::list<PolyMesh::HalfedgeHandle> loop = getLoopHalfedges(heh, mesh);
    PolyMesh::HalfedgeHandle begin = mesh.prev_halfedge_handle(loop.front());
    if(mesh.is_boundary(begin) || mesh.is_boundary(mesh.opposite_halfedge_handle(begin)))
        loop.push_front(begin);
    
    // For debug
//    auto loopIt = loop.begin();
//    auto heh1 = *loopIt;
//    loopIt++;
//    auto heh2 = *loopIt;
//    loop.clear();
//    loop.push_back(heh1);
//    loop.push_back(heh2);
    
    std::unordered_map<PolyMesh::VertexHandle, PolyMesh::Point> leftDirections;
    std::unordered_map<PolyMesh::VertexHandle, PolyMesh::Point> rightDirections;
    std::list<std::vector<PolyMesh::VertexHandle>> bevelSegments;
    std::list<PolyMesh::VertexHandle> duplicates;
    
    for(auto loopIt = loop.begin(); loopIt != loop.end(); loopIt++) {
        OpenMesh::SmartHalfedgeHandle he;
        PolyMesh::VertexHandle vh;
        
        // First halfedge may be on boundary and rotated so we take next
        if(loopIt == loop.begin()) {
            he = OpenMesh::make_smart(*std::next(loopIt), mesh);
            vh = he.from();
        } else {
            he = OpenMesh::make_smart(*loopIt, mesh);
            vh = he.to();
        }
        
        /*
              | ^
              | |
         <----v <----
         */
        
        PolyMesh::Point toPoint = mesh.point(vh);
        PolyMesh::Point leftPoint = mesh.point(he.next().to());
        PolyMesh::Point rightPoint = mesh.point(he.opp().prev().from());
        PolyMesh::Point dirA = (leftPoint - toPoint).normalize();
        PolyMesh::Point dirB = -(rightPoint - toPoint).normalize();
//        PolyMesh::Point dirLeft = (dirA + dirB) / 2.0f;
        leftDirections[vh] = dirA;
        rightDirections[vh] = -dirB;
    }
        
    for(auto loopIt = loop.begin(); loopIt != loop.end(); loopIt++) {
        auto lh = OpenMesh::make_smart(*loopIt, mesh);
        PolyMesh::VertexHandle leftVert = mesh.to_vertex_handle(lh);
        PolyMesh::Point point = mesh.point(leftVert);
        PolyMesh::Point dirLeft = leftDirections[leftVert];
        PolyMesh::Point dirRight = rightDirections[leftVert];//-dirLeft;
        
        bool isOnBoundary = loopIt == loop.begin() && lh.opp().is_boundary();
        
        PolyMesh::VertexHandle dupVert = rip(lh, mesh, isOnBoundary);
        
        PolyMesh::Point leftPoint = point + dirLeft * radius;
        PolyMesh::Point rightPoint = point + dirRight * radius;
        
        mesh.set_point(leftVert, leftPoint);
        mesh.set_point(dupVert, rightPoint);
        
        bevelSegments.push_back(makeBevelSegments(leftPoint, point, rightPoint, segments, mesh));
        
        duplicates.push_back(dupVert);
    }
    
    if(bevelSegments.front().size() > 0) {
        auto dupIt = duplicates.begin();
        auto segIt = bevelSegments.begin();
        for(auto loopIt = loop.begin(); loopIt != loop.end(); loopIt++) {
            auto nextIt = std::next(loopIt);
            if(nextIt == loop.end())
                break;

            auto nextDupIt = std::next(dupIt);
            auto nextSegIt = std::next(segIt);
            
            PolyMesh::VertexHandle leftTop = mesh.to_vertex_handle(*loopIt);
            PolyMesh::VertexHandle leftBottom = mesh.to_vertex_handle(*nextIt);
            PolyMesh::VertexHandle rightTop = *dupIt;
            PolyMesh::VertexHandle rightBottom = *nextDupIt;
            
            mesh.add_face(leftTop, segIt->front(), nextSegIt->front(), leftBottom);
            mesh.add_face(rightTop, rightBottom, nextSegIt->back(), segIt->back());

            for(int i = 0; i < segIt->size() - 1; i++)
                mesh.add_face(segIt->at(i), segIt->at(i + 1),
                              nextSegIt->at(i + 1), nextSegIt->at(i));
            
            dupIt++;
            segIt++;
        }
    }
        
//    for(auto vh : selVerts)
//        mesh.status(vh).set_selected(false);
//
//    for(auto vh : duplicates)
//        mesh.status(vh).set_selected(true);
}

MeshViewer2D::MeshViewer2D(PolyMesh* mesh):
    mesh(mesh) {}

void MeshViewer2D::draw(bvg::Context& ctx, bool isMouseDown, float mouseX, float mouseY) {
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
    ctx.lineWidth = 1.0f;
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
        ctx.arc(vertPos.x, vertPos.y, 3.0f, 0.0f, M_PI * 2.0f);
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
            glm::vec2 offset = dirPerp * -2.0f;
            start += offset + dir * 8.0f;
            end += offset - dir * 8.0f;
            glm::vec2 arrowPoint = end - dir * 6.0f - dirPerp * 4.0f;
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
