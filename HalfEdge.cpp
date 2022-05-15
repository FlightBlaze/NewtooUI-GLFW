//
//  HalfEdge.cpp
//  MyProject
//
//  Created by Dmitry on 05.05.2022.
//

#include "HalfEdge.h"

#include <iostream>

using namespace HE;

HalfEdge::HalfEdge():
    next(nullptr), twin(nullptr), prev(nullptr),
    vert(nullptr), face(nullptr) {}

Vertex::Vertex():
    edge(nullptr), isSelected(false) {}

Face::Face():
    edge(nullptr) {}

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
    this->wasNext = true;
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

std::list<Vertex*> Mesh::open(std::vector<Vertex*> vertices) {
    std::list<Vertex*> copiedVertices;
    for (auto vert : vertices) {
        copiedVertices.push_back(this->addVertex(vert->pos));
    }
    auto copyIt = copiedVertices.begin();
    for (auto vert : vertices) {
        if(vert->edge->face == nullptr)
            continue;
        std::vector<Vertex*> faceVerts;
        for (auto it = FaceEdgeIter(vert->edge->face); !it.isEnd(); it++) {
            if(it->vert == vert) {
                faceVerts.push_back(*copyIt);
            } else {
                faceVerts.push_back(it->vert);
            }
        }
        this->deleteFace(vert->edge->face);
        this->addFace(faceVerts);
        copyIt++;
    }
    return copiedVertices;
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
    if(vert->isSelected)
        return;
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
        halfEdgesToDelete.push_back(aroundVertex.current());
        halfEdgesToDelete.push_back(aroundVertex->twin);
        Face* face = aroundVertex->face;
        for (auto aroundFace = FaceEdgeIter(face);
             !aroundFace.isEnd(); aroundFace++) {
            aroundFace->face = nullptr;
//            halfEdgesToDelete.push_back(aroundFace.current());
        }
        delete face;
    }
    for(auto edge : halfEdgesToDelete) {
        Vertex* origin = edge->prev->vert;
        if(origin->edge == edge) {
            origin->edge = edge->twin;
        }
    }
    for(auto edge : halfEdgesToDelete) {
//        if(edge->twin != nullptr) {
//            edge->twin->twin = nullptr;
//        }
        delete edge;
    }
    this->vertices.remove(vert);
    delete vert;
}

void Mesh::deleteFace(Face* face) {
    // TODO: deleteFace
}

void VertexEdgeIter::next() {
    if(this->cur->twin == nullptr) {
        this->cur = nullptr;
        this->wasNext = true;
        return;
    }
    
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
    return (cur == this->start && this->wasNext) || cur == nullptr;
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
    Vertex* vert = new Vertex();
    vert->pos = pos;
    vert->edge = nullptr;
    this->vertices.push_back(vert);
    return vert;
}

Face* Mesh::addFace(std::vector<Vertex*> vertices) {
    /*
     
    *       *       *       *
     |-----| |-----| |-----|
     |     | |     | |     |
     |-----| |-----| |-----|
    *       &   >   *       *
     ^----->         |-----|
     |     |^       v|     |
     <-----v         |-----|
    *       *   <   *       *
     |-----| |-----| |-----|
     |     | |     | |     |
     |-----  |-----| |-----|
    *       *       *       *
     
     */

    Face* face = new Face();
    std::vector<HalfEdge*> halfEdges;
    for(int i = 0; i < vertices.size(); i++) {
        HalfEdge* he = new HalfEdge();
        int nextVertInd = i < vertices.size() - 1? i + 1 : 0;
        he->vert = vertices[nextVertInd];
        he->face = face;
        halfEdges.push_back(he);
    }
    // ------------------------------------------------------------
    std::list<std::pair<HalfEdge*, HalfEdge*>> twins;
    for(int i = 0; i < halfEdges.size(); i++) {
        HalfEdge* current = halfEdges[i];
        Vertex* origin = vertices[i];
        Vertex* destination = current->vert;
        if(destination->edge != nullptr) {
            /*
                |     ^ |     ^
                |     | |     |
                v--T--> v----->
               &       *       *
                <--C--^ <-----^
                |     | |     |
                v     | v     |
             */
            for (auto aroundVert = VertexEdgeIter(destination);
                 !aroundVert.isEnd(); aroundVert++) {
                
                if(aroundVert->vert == origin) {
//                    twins.push_back(std::make_pair(aroundVert.current(), current));
                    delete halfEdges[i];
                    halfEdges[i] = aroundVert->twin;
                    halfEdges[i]->face = face;
                    halfEdges[i]->next = nullptr;
                    halfEdges[i]->prev = nullptr;
                    break;
                }
            }
        }
    }
        
    for(auto pair : twins) {
//        HalfEdge* twin = pair.first;
//        HalfEdge* current = pair.second;
//
//        if(twin->twin != nullptr) {
//            delete twin->twin;
//        }
//        twin->twin = current;
//        current->twin = twin;
    }
    
    for(int i = 0; i < halfEdges.size(); i++) {
        HalfEdge* current = halfEdges[i];
        Vertex* origin = vertices[i];
        Vertex* destination = current->vert;
        
        int nextInd = i < halfEdges.size() - 1? i + 1 : 0;
        int prevInd = i > 0? i - 1 : halfEdges.size() - 1;
        current->next = halfEdges[nextInd];
        current->prev = halfEdges[prevInd];
        
        if(current->twin == nullptr) {
            HalfEdge* twin = new HalfEdge();
            current->twin = twin;
            twin->twin = current;
            twin->vert = origin;
            twin->next = nullptr;
            twin->prev = nullptr;
            twin->face = nullptr;
        }
    }
    // ------------------------------------------------------------
    for(int i = 0; i < halfEdges.size(); i++) {
        HalfEdge* current = halfEdges[i];
        HalfEdge* twin = current->twin;
        if(twin->prev == nullptr || twin->next == nullptr) {
            /*
                  <------
                *
                  ------>
               | ^
               | |
               v |
             */
            twin->prev = current->next->twin;
            twin->next = current->prev->twin;
        }
    }
    for(int i = 0; i < halfEdges.size(); i++) {
        if(vertices[i]->edge == nullptr) {
            vertices[i]->edge = halfEdges[i];
        }
    }
    face->edge = halfEdges[0];
    if(this->faces.size() == 0) {
        face->edge->isDebugSelected = true;
    }
    this->faces.push_back(face);
    return face;
}

Face* Mesh::addFace2(std::vector<Vertex*> vertices) {
    /*
     
    *       *       *       *
     |-----| |-----| |-----|
     |     | |     | |     |
     |-----| |-----| |-----|
    *       &   >   *       *
     ^----->         |-----|
     |     |^       v|     |
     <-----v         |-----|
    *       *   <   *       *
     |-----| |-----| |-----|
     |     | |     | |     |
     |-----  |-----| |-----|
    *       *       *       *
     
     */

    Face* face = new Face();
    std::vector<HalfEdge*> halfEdges;
    for(int i = 0; i < vertices.size(); i++) {
        HalfEdge* he = new HalfEdge();
        int nextVertInd = i < vertices.size() - 1? i + 1 : 0;
        he->vert = vertices[nextVertInd];
        he->face = face;
        halfEdges.push_back(he);
    }
    
    for(int i = 0; i < halfEdges.size(); i++) {
        HalfEdge* current = halfEdges[i];
        Vertex* origin = vertices[i];
        Vertex* destination = current->vert;

        int nextInd = i < halfEdges.size() - 1? i + 1 : 0;
        int prevInd = i > 0? i - 1 : halfEdges.size() - 1;
        current->next = halfEdges[nextInd];
        current->prev = halfEdges[prevInd];
    }
    
    // ------------------------------------------------------------
    std::list<std::pair<HalfEdge*, int>> twins;
    for(int i = 0; i < halfEdges.size(); i++) {
        HalfEdge* current = halfEdges[i];
        Vertex* origin = vertices[i];
        Vertex* destination = current->vert;
        if(destination->edge != nullptr) {
            /*
                |     ^ |     ^
                |     | |     |
                v--T--> v----->
               &       *       *
                <--C--^ <-----^
                |     | |     |
                v     | v     |
             */
            
            for (auto aroundVert = VertexEdgeIter(destination);
                 !aroundVert.isEnd(); aroundVert++) {
                
                // Here are an error
                if(aroundVert->vert == origin) {
                    twins.push_back(std::make_pair(aroundVert.current(), i));
                    break;
                }
            }
        }
    }
    
//    std::cout << "Num twins: " << twins.size() << std::endl; // 1
    
    for(auto pair : twins) {
        HalfEdge* twin = pair.first;
//        HalfEdge* current = pair.second;
        int i = pair.second;
        
        if(twin->twin != nullptr) {
            delete halfEdges[i];
            halfEdges[i] = twin->twin;
            halfEdges[i]->face = face;
            halfEdges[i]->next = nullptr;
            halfEdges[i]->prev = nullptr;
            
            twin->twin = halfEdges[i];
            halfEdges[i]->twin = twin;
        }
//
//        if(twin->twin != nullptr) {
//            delete twin->twin;
//        }
    }
    // ------------------------------------------------------------
    for(int i = 0; i < halfEdges.size(); i++) {
        HalfEdge* current = halfEdges[i];
        Vertex* origin = vertices[i];
        if(current->twin == nullptr) {
            HalfEdge* twin = new HalfEdge();
            current->twin = twin;
            twin->twin = current;
            twin->vert = origin;
            twin->face = nullptr;
            twin->prev = current->next->twin;
            twin->next = current->prev->twin;
        }
    }
    for(int i = 0; i < halfEdges.size(); i++) {
        if(vertices[i]->edge == nullptr) {
            vertices[i]->edge = halfEdges[i];
        }
    }
    face->edge = halfEdges[0];
    if(this->faces.size() == 0) {
        face->edge->isDebugSelected = true;
    }
    this->faces.push_back(face);
    return face;
}

HalfEdge* Mesh::findDebugSelectedHalfEdge() {
    for(auto vert : this->vertices) {
        for(auto edgeIt = VertexEdgeIter(vert); !edgeIt.isEnd(); edgeIt++) {
            if(edgeIt->isDebugSelected)
                return edgeIt.current();
        }
    }
    return nullptr;
}

MeshViewer::MeshViewer(Mesh *mesh):
    mesh(mesh) {}

void MeshViewer::draw(bvg::Context& ctx) {
    if(mesh == nullptr)
        return;
    
    bvg::Style halfEdgeStyle = bvg::SolidColor(bvg::colors::Black);
    bvg::Style boundaryHalfEdgeStyle = bvg::SolidColor(bvg::Color(1.0f, 0.0f, 0.0f));
    bvg::Style debugHalfEdgeStyle = bvg::SolidColor(bvg::Color(0.0f, 1.0f, 0.0f));
    
    
    ctx.fillStyle = bvg::SolidColor(bvg::colors::Black);
    ctx.strokeStyle = bvg::SolidColor(bvg::colors::Black);
    ctx.lineWidth = 2.0f;
    for(auto vert : mesh->vertices) {
        ctx.beginPath();
        ctx.arc(vert->pos.x, vert->pos.y, 8.0f, 0.0f, M_PI * 2.0f);
        ctx.convexFill();
        
        if(vert->edge == nullptr)
            continue;
        int i = 0;
        for(auto edgeIt = VertexEdgeIter(vert); !edgeIt.isEnd(); edgeIt++) {
            glm::vec2 start = vert->pos;
            glm::vec2 end = edgeIt->vert->pos;
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
            
            if(edgeIt->face != nullptr) {
                ctx.strokeStyle = halfEdgeStyle;
            } else {
                ctx.strokeStyle = boundaryHalfEdgeStyle;
            }
            if(edgeIt->isDebugSelected) {
                ctx.strokeStyle = debugHalfEdgeStyle;
            }
            ctx.stroke();
            
            i++;
            if(i == 10)
                break;
        }
    }
}
