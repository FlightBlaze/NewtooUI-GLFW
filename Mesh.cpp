//
//  Mesh.cpp
//  MyProject
//
//  Created by Dmitry on 13.05.2022.
//

#include "Mesh.h"
#include <OpenMesh/Core/Utils/Predicates.hh>

MeshViewer::MeshViewer(PolyMesh* mesh):
    mesh(mesh) {}

void MeshViewer::draw(bvg::Context& ctx, bool isMouseDown, float mouseX, float mouseY) {
    if(mesh == nullptr)
        return;
    
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
        
        PolyMesh::VertexEdgeCWIter veIt;
        for (veIt = mesh->ve_cwiter( *vIt ); veIt.is_valid(); veIt++) {
            
            PolyMesh::Point originPoint = mesh->point(veIt->v0());
            glm::vec2 originPos = glm::vec2(originPoint[0], originPoint[1]);
            
            PolyMesh::Point destPoint = mesh->point(veIt->v1());
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
//            if(veIt->face != nullptr) {
//                ctx.strokeStyle = halfEdgeStyle;
//            } else {
//                ctx.strokeStyle = boundaryHalfEdgeStyle;
//            }
            
//            if(edgeIt->isDebugSelected) {
//                ctx.strokeStyle = debugHalfEdgeStyle;
//            }
            ctx.stroke();
            
//            i++;
//            if(i == 10)
//                break;
        }
    }
}
