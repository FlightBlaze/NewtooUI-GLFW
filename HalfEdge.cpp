//
//  HalfEdge.cpp
//  MyProject
//
//  Created by Dmitry on 05.05.2022.
//

#include "HalfEdge.h"

void EdgeLoopIter::next() {
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
    HalfEdge* startEdge = this->cur;
    HalfEdge* curEdge = startEdge;
    do {
        curEdge = curEdge->twin->next;
        const HalfEdge* twinEdge = curEdge->twin;
        const Face* curEdgeFace = curEdge->face;
        const Face* curTwinFace = twinEdge->face;
        
        // If this edge is not sharing the same
        // faces as the current edge
        if(curEdgeFace != curFace && curEdgeFace != twinFace &&
           curTwinFace != curFace && curTwinFace != twinFace) {
            this->cur = curEdge;
            return;
        }
    } while (curEdge != startEdge);
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
    return this->cur == this->start || this->cur == nullptr;
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
    HalfEdge* startEdge = this->cur->twin;
    HalfEdge* curEdge = startEdge;
    do {
        curEdge = curEdge->twin->next;
        if(curEdge->isSelectionBoundary()) {
            cur = curEdge;
            return;
        }
    } while(curEdge != startEdge);
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
    return cur == this->start || cur == nullptr;
}

std::list<Face*> Mesh::selectedFaces() {
    std::list<Face*> faces;
    for(auto it : this->selectedVertices) {
        HalfEdge* aroundVertexStart = it->edge;
        HalfEdge* aroundVertexCur = aroundVertexStart;
        do {
            aroundVertexCur = aroundVertexCur->twin->next;
            HalfEdge* aroundFaceStart = aroundVertexCur;
            HalfEdge* aroundFaceCur = aroundFaceStart;
            bool allVerticesSelected = true;
            do {
                aroundFaceCur = aroundFaceCur->next;
                if(!aroundFaceCur->vert->isSelected) {
                    allVerticesSelected = false;
                    break;
                }
            } while(aroundFaceCur != aroundFaceStart);
            if(allVerticesSelected) {
                faces.push_back(aroundFaceCur->face);
            }
        } while(aroundVertexCur != aroundVertexStart);
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
        HalfEdge* aroundFaceStart = itFace->edge;
        HalfEdge* aroundFaceCur = aroundFaceStart;
        do {
            aroundFaceCur = aroundFaceCur->next;
            newFaceVertices.push_back(originalCopyPair[aroundFaceCur->vert]);
        } while(aroundFaceCur != aroundFaceStart);
        this->addFace(newFaceVertices);
    }
    return newVerts;
}

HalfEdge* Mesh::findSelectionBoundary() {
    for(auto it : selectedVertices) {
        HalfEdge* startEdge = it->edge;
        HalfEdge* curEdge = startEdge;
        do {
            curEdge = curEdge->twin->next;
            if(curEdge->isSelectionBoundary()) {
                return curEdge;
            }
        } while(curEdge != startEdge);
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
