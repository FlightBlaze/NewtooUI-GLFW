//
//  HalfEdge.h
//  MyProject
//
//  Created by Dmitry on 05.05.2022.
//

#include <glm/glm.hpp>
#include <vector>
#include <list>
#include <unordered_map>

#pragma once

struct Vertex;
struct HalfEdge;
struct Face;

struct HalfEdge {
    HalfEdge *next, *twin, *prev;
    Vertex *vert;
    Face *face;
    
    bool isSelectionBoundary();
};

struct Vertex {
    HalfEdge *edge;
    glm::vec2 pos;
    bool isSelected;
};

struct Face {
    HalfEdge *edge;
};

struct Mesh {
    std::list<Vertex*> selectedVertices;
    std::list<Face*> selectedFaces();
    
    Vertex* addVertex(glm::vec2 pos);
    Face* addFace(std::vector<Vertex*> vertices);
    void deleteVertex(Vertex* vert);
    void deselectAll();
    void selectOne(Vertex* vert);
    void select(std::vector<Vertex*> vertices);
    std::list<Vertex*> duplicate();
    HalfEdge* findSelectionBoundary();
    void extrude();
    // TODO: Extrude
};

class EdgeLoopIter {
    HalfEdge* start;
    HalfEdge* cur;
    
    void next();
    
public:
    EdgeLoopIter(HalfEdge* edge);
    
    void operator++();
    HalfEdge* operator->();
    bool isEnd();
};

class SelBoundIter {
    HalfEdge* start;
    HalfEdge* cur;
    
    void next();
    
public:
    SelBoundIter(HalfEdge* edge);
    
    void operator++(int n);
    HalfEdge* operator->();
    HalfEdge* current();
    bool isEnd();
};

//struct Edge;
//struct Face;
//struct Mesh;
//
//struct Vertex {
//    std::vector<Edge*> edges;
//    glm::vec2 pos;
//};
//
//struct Edge {
//    Vertex *vert1, *vert2;
//    Face *face1, *face2;
//};
//
//struct Face {
//    std::vector<Edge*> edges;
//};
//
//struct EdgeFlow {
//    Vertex* tail;
//    Edge* edge;
//
//    Vertex* head() {
//        return edge->vert1 == tail? edge->vert2 : edge->vert1;
//    }
//
//    EdgeFlow flipped() {
//        EdgeFlow ef;
//        ef.tail = head();
//        ef.edge = edge;
//        return ef;
//    }
//
//    EdgeFlow aroundVertex(Face* f) {
//        //  *----*----*
//        //  | f3 | f4 |
//        //  *----*>>>>*
//        //  | f1 ^ f2 |
//        //  *----*----&
//
//        Vertex* h = head();
//        Edge* nextEdge = nullptr;
//        for (Edge *e : h->edges) {
//            if(e->face1 == f || e->face2 == f) {
//                nextEdge = e;
//                break;
//            }
//        }
//        EdgeFlow ef;
//        ef.edge = nextEdge;
//        ef.tail = h;
//        return ef;
//    }
//
//    EdgeFlow aroundFace(Face* f) {
//        //  *----*----*
//        //  | f3 | f4 |
//        //  *----*>>>>*
//        //  | f1 ^ f2 |
//        //  *----*----&
//
//        Vertex* h = head();
//        Edge* nextEdge = nullptr;
//        for (Edge *e : h->edges) {
//            if(e->face1 == f || e->face2 == f) {
//                nextEdge = e;
//                break;
//            }
//        }
//        EdgeFlow ef;
//        ef.edge = nextEdge;
//        ef.tail = h;
//        return ef;
//    }
//
//    EdgeFlow forward() {
//        //  *----*----*
//        //  | f3 ^ f4 |
//        //  *----*----*
//        //  | f1 ^ f2 |
//        //  *----*----*
//
//        Vertex* h = head();
//        Edge* nextEdge = nullptr;
//        if(h->edges.size() == 4) {
//            for (Edge *e : h->edges) {
//                if(e->face1 != edge->face1 && e->face1 != edge->face2 &&
//                   e->face2 != edge->face1 && e->face2 != edge->face2) {
//                    nextEdge = e;
//                    break;
//                }
//            }
//        }
//        EdgeFlow ef;
//        ef.edge = nextEdge;
//        ef.tail = h;
//        return ef;
//    }
//};
//
//struct Mesh {
//    std::vector<Vertex> vertices;
//    std::vector<Edge> edges;
//    std::vector<Face> face;
//};
