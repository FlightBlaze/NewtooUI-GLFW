//
//  HalfEdge.cpp
//  MyProject
//
//  Created by Dmitry on 05.05.2022.
//

#include "HalfEdge.h"

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
    const Face* curFace = this->cur->face;
    const Face* twinFace = this->cur->twin->face;
    for(auto it = VertexEdgeIter(cur->prev->vert); !it.isEnd(); it++) {
        const HalfEdge* twinEdge = it->twin;
        const Face* curEdgeFace = it->face;
        const Face* curTwinFace = twinEdge->face;
        
        // If this edge is not sharing the same
        // faces as the current edge
        if(curEdgeFace != curFace && curEdgeFace != twinFace &&
           curTwinFace != curFace && curTwinFace != twinFace) {
            this->cur = it.current();
            return;
        }
    }
    this->cur = nullptr;
}

EdgeLoopIter::EdgeLoopIter(HalfEdge* edge):
    start(edge), cur(edge) {}

void EdgeLoopIter::operator++() {
    this->next();
}

HalfEdge* EdgeLoopIter::operator->() {
    return this->cur;
}

bool EdgeLoopIter::isEnd() {
    return (this->cur == this->start && this->wasNext) || this->cur == nullptr;
}

bool HalfEdge::isSelectionBoundary() {
    /*
        D        D        D
         |<-----^ |<-----^
         |      | |      |
         |      | |      |
         V--C-->| V--N-->|
        S        S        S
         |<-----^ |<-----^
         |      | |      |
         |      | |      |
         V----->| V----->|
        S        S        S
     */
    const bool isPointingVertSelected = this->vert->isSelected;
    const bool isPrevHalfEdgeOriginVertSelected =
        this->prev->prev->vert->isSelected;
    
    // If the current edge are between the perpendicular edge
    // with selected vertex and the one with not selected
    return isPointingVertSelected && !isPrevHalfEdgeOriginVertSelected;
}

void SelBoundIter::next() {
    for (auto it = VertexEdgeIter(cur->prev->vert); !it.isEnd(); it++) {
        if(it->isSelectionBoundary()) {
            cur = it.current();
            return;
        }
    }
    cur = nullptr;
}

SelBoundIter::SelBoundIter(HalfEdge* edge):
    start(edge), cur(edge) {}

void SelBoundIter::operator++(int n) {
    next();
}

HalfEdge* SelBoundIter::operator->() {
    return cur;
}

HalfEdge* SelBoundIter::current() {
    return cur;
}

bool SelBoundIter::isEnd() {
    return (cur == this->start && this->wasNext)|| cur == nullptr;
}

std::list<Face*> Mesh::selectedFaces() {
    std::list<Face*> faces;
    for(auto vert : this->selectedVertices) {
        for (auto aroundVertex = VertexEdgeIter(vert);
             !aroundVertex.isEnd(); aroundVertex++){
            bool allVerticesSelected = true;
            for (auto aroundFace = FaceEdgeIter(aroundVertex->face);
                 !aroundFace.isEnd(); aroundFace++) {
                if(!aroundFace->vert->isSelected) {
                    allVerticesSelected = false;
                    break;
                }
            }
            if(allVerticesSelected) {
                faces.push_back(aroundVertex->face);
            }
        }
    }
    return faces;
}

std::list<Vertex*> Mesh::duplicate() {
    std::list<Vertex*> newVerts;
    std::list<Face*> selFaces = this->selectedFaces();
    std::unordered_map<Vertex*, Vertex*> originalCopyPair;
    for(auto itSel : this->selectedVertices) {
        Vertex* newVert = this->addVertex(itSel->pos);
        newVerts.push_back(newVert);
        originalCopyPair[itSel] = newVert;
    }
    for(auto itFace : selFaces) {
        std::vector<Vertex*> newFaceVertices;
        for (auto aroundFace = FaceEdgeIter(itFace);
             !aroundFace.isEnd(); aroundFace++) {
            newFaceVertices.push_back(originalCopyPair[aroundFace->vert]);
        }
        this->addFace(newFaceVertices);
    }
    return newVerts;
}

HalfEdge* Mesh::findSelectionBoundary() {
    for(auto vert : selectedVertices) {
        for(auto it = VertexEdgeIter(vert); !it.isEnd(); it++) {
            if(it->isSelectionBoundary()) {
                return it.current();
            }
        }
    }
    return nullptr;
}

void Mesh::extrude() {
    // Separate selected faces
    std::list<Vertex*> top = this->duplicate();
    std::unordered_map<Vertex*, Vertex*> originalCopyPair;
    auto itCopy = top.begin();
    for(auto itSel : this->selectedVertices) {
        if(itSel->isSelected) {
            originalCopyPair[itSel] = *itCopy;
        } else {
            this->deleteVertex(itSel);
        }
        itCopy++;
    }
    // Bridge selection boundary to separated faces
    SelBoundIter itSb = SelBoundIter(findSelectionBoundary());
    for(SelBoundIter itSb = SelBoundIter(findSelectionBoundary());
        !itSb.isEnd(); itSb++) {
        Vertex* edgeBottomOrigin = itSb->prev->vert;
        Vertex* edgeBottomEnd = itSb->vert;
        Vertex* edgeTopOrigin = originalCopyPair[edgeBottomOrigin];
        Vertex* edgeTopEnd = originalCopyPair[edgeBottomEnd];
        std::vector<Vertex*> faceVerts = {
            edgeBottomOrigin, edgeBottomEnd, edgeTopEnd, edgeTopOrigin};
        this->addFace(faceVerts);
    }
    // Change selection to separated
    std::list<Vertex*> oldSel = this->selectedVertices;
    this->deselectAll();
    for(auto it : oldSel) {
        this->selectOne(originalCopyPair[it]);
    }
}

void Mesh::deselectOne(Vertex* vert) {
    vert->isSelected = false;
    this->selectedVertices.remove(vert);
}

void Mesh::deselectAll() {
    for(auto vert : this->selectedVertices) {
        vert->isSelected = false;
    }
    this->selectedVertices.clear();
}

void Mesh::selectOne(Vertex* vert) {
    vert->isSelected = true;
    this->selectedVertices.push_back(vert);
}

void Mesh::select(std::vector<Vertex*> vertices) {
    for(auto vert : vertices) {
        selectOne(vert);
    }
}

void Mesh::deleteVertex(Vertex* vert) {
    std::list<HalfEdge*> halfEdgesToDelete;
    HalfEdge* aroundVertexStart = vert->edge;
    HalfEdge* aroundVertexCur = aroundVertexStart;
    for (auto aroundVertex = VertexEdgeIter(vert);
         !aroundVertex.isEnd(); aroundVertex++) {
        for (auto aroundFace = FaceEdgeIter(aroundVertex->face);
             !aroundFace.isEnd(); aroundFace++) {
            halfEdgesToDelete.push_back(aroundFace.current());
        }
    }
    for(auto edge : halfEdgesToDelete) {
        Vertex* origin = edge->prev->vert;
        if(origin->edge == edge) {
            origin->edge = edge->twin;
        }
    }
    for(auto edge : halfEdgesToDelete) {
        if(edge->twin != nullptr) {
            edge->twin->twin = nullptr;
        }
        delete edge;
    }
}

void VertexEdgeIter::next() {
    this->cur = this->cur->twin->next;
    this->wasNext = true;
}

VertexEdgeIter::VertexEdgeIter(Vertex* vert):
    start(vert->edge), cur(vert->edge) {}

void VertexEdgeIter::operator++(int n) {
    next();
}

HalfEdge* VertexEdgeIter::operator->() {
    return cur;
}

HalfEdge* VertexEdgeIter::current() {
    return cur;
}

bool VertexEdgeIter::isEnd() {
    return cur == this->start && this->wasNext;
}

void FaceEdgeIter::next() {
    this->cur = this->cur->next;
    this->wasNext = true;
}

FaceEdgeIter::FaceEdgeIter(Face* face):
    start(face->edge), cur(face->edge) {}

void FaceEdgeIter::operator++(int n) {
    next();
}

HalfEdge* FaceEdgeIter::operator->() {
    return cur;
}

HalfEdge* FaceEdgeIter::current() {
    return cur;
}

bool FaceEdgeIter::isEnd() {
    return cur == this->start && this->wasNext;
}

Vertex* Mesh::addVertex(glm::vec2 pos) {
    // TODO: addVertex
}

Face* Mesh::addFace(std::vector<Vertex*> vertices) {
    // TODO: addFace
}
