//
//  Gizmos.cpp
//  MyProject
//
//  Created by Dmitry on 03.12.2021.
//

#include <Gizmos.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/closest_point.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/intersect.hpp>
#include <iostream>
#include <MathExtras.h>

namespace gizmo {

void buildLineTriangleTip(bvg::Context& ctx, float sx, float sy, float ex, float ey, float size) {
    glm::vec2 endPos = glm::vec2(ex, ey);
    glm::vec2 endDir = glm::normalize(glm::vec2(ex - sx, ey - sy));
    glm::vec2 endLeft = glm::vec2(endDir.y, -endDir.x);
    glm::vec2 endRight = -endLeft;
    glm::vec2 A = endLeft * size + endPos;
    glm::vec2 B = endDir * size * 1.667f + endPos;
    glm::vec2 C = endRight * size + endPos;
    ctx.beginPath();
    ctx.moveTo(A.x, A.y);
    ctx.lineTo(B.x, B.y);
    ctx.lineTo(C.x, C.y);
    ctx.closePath();
}

void drawArrow(bvg::Context& ctx, float sx, float sy, float ex, float ey, float size) {
    ctx.beginPath();
    ctx.moveTo(sx, sy);
    ctx.lineTo(ex, ey);
    ctx.stroke();
    buildLineTriangleTip(ctx, sx, sy, ex, ey, size);
    ctx.convexFill();
}

void drawLineWithSquare(bvg::Context& ctx, float sx, float sy, float ex, float ey, float size) {
    ctx.beginPath();
    ctx.moveTo(sx, sy);
    ctx.lineTo(ex, ey);
    ctx.stroke();
    glm::vec2 endPos = glm::vec2(ex, ey);
    glm::vec2 endDir = glm::normalize(glm::vec2(ex - sx, ey - sy));
    glm::vec2 endLeft = glm::vec2(endDir.y, -endDir.x);
    glm::vec2 endRight = -endLeft;
    glm::vec2 endAdvancePos = endDir * size * 1.667f + endPos;
    glm::vec2 A = endLeft * size + endPos;
    glm::vec2 B = endRight * size + endPos;
    glm::vec2 C = endRight * size + endAdvancePos;
    glm::vec2 D = endLeft * size + endAdvancePos;
    ctx.beginPath();
    ctx.moveTo(A.x, A.y);
    ctx.lineTo(B.x, B.y);
    ctx.lineTo(C.x, C.y);
    ctx.lineTo(D.x, D.y);
    ctx.closePath();
    ctx.convexFill();
}

glm::vec3 worldToScreenSpace(bvg::Context& ctx, glm::vec3 vec, glm::mat4& viewproj) {
    glm::vec4 projected = viewproj * glm::vec4(vec, 1.0f);
    projected /= projected.w;
    float x = (projected.x + 1.0f) * ctx.width / 2.0f;
    float y = (1.0f - projected.y) * ctx.height / 2.0f;
    return glm::vec3(x, y, projected.z);
}

glm::vec3 screenToWorldSpace(bvg::Context& ctx, glm::vec3 vec, glm::mat4& viewproj) {
    // screen coords to clip coords
    float normalizedX = vec.x / (ctx.width * 0.5f) - 1.0f;
    float normalizedY = vec.y / (ctx.height * 0.5f) - 1.0f;
    auto clip = glm::vec4(normalizedX, -normalizedY, vec.z, 1.0f);
    glm::mat4 invVP = glm::inverse(viewproj);
    auto world = invVP * clip;
    world /= world.w;
    return glm::vec3(world);
}

void drawGizmoArrow(bvg::Context& ctx, glm::mat4& viewproj,
                    glm::vec3 origin, glm::vec3 end,
                    bvg::Color color,
                    bool forScaling = false) {
    bvg::Style style = bvg::SolidColor(color);
    ctx.strokeStyle = style;
    ctx.fillStyle = style;
    ctx.lineWidth = 8;
    
    glm::vec3 originS = worldToScreenSpace(ctx, origin, viewproj);
    glm::vec3 endS = worldToScreenSpace(ctx, end, viewproj);
    
    // If arrow tip is too near to the camera then
    // then don't render the arrow
    if(originS.z >= 1.0f || endS.z >= 1.0f)
        return;
    
    if(forScaling)
        drawLineWithSquare(ctx, originS.x, originS.y, endS.x, endS.y, 12);
    else
        drawArrow(ctx, originS.x, originS.y, endS.x, endS.y, 16);
}

bool isMouseOverGizmoArrow(bvg::Context& ctx, glm::mat4& viewproj,
                    glm::vec3 origin, glm::vec3 end,
                    glm::vec2 mouse,
                    bool forScaling = false) {
    bool overlapsLine;
    bool overlapsTip;
    ctx.lineWidth = 14;
    
    glm::vec3 originS = worldToScreenSpace(ctx, origin, viewproj);
    glm::vec3 endS = worldToScreenSpace(ctx, end, viewproj);
    if(originS.z >= 1.0f || endS.z >= 1.0f)
        return;
    
    ctx.beginPath();
    ctx.moveTo(originS.x, originS.y);
    ctx.lineTo(endS.x, endS.y);
    overlapsLine = ctx.isPointInsideStroke(mouse.x, mouse.y);
    buildLineTriangleTip(ctx, originS.x, originS.y, endS.x, endS.y, 16);
    overlapsTip = ctx.isPointInsideConvexFill(mouse.x, mouse.y);
    return overlapsLine || overlapsTip;
}

void drawGizmoCenter(bvg::Context& ctx, glm::mat4& viewproj,
                     glm::vec3 center, bvg::Color color) {
    glm::vec2 centerS = worldToScreenSpace(ctx, center, viewproj);
    ctx.fillStyle = bvg::SolidColor(color);
    ctx.beginPath();
    ctx.arc(centerS.x, centerS.y, 4.0f, 0.0f, M_PI * 2.0f);
    ctx.convexFill();
    
    bvg::Color fillColor = color;
    fillColor.a *= 0.5f;
//    fillColor = bvg::Color::lerp(fillColor, bvg::colors::Black, 0.2f);
    ctx.fillStyle = bvg::SolidColor(fillColor);
    ctx.strokeStyle = bvg::SolidColor(color);
    ctx.beginPath();
    ctx.arc(centerS.x, centerS.y, 16.0f, 0.0f, M_PI * 2.0f);
    ctx.convexFill();
    ctx.lineWidth = 4.0f;
    ctx.stroke();
}

bool isMouseOverGizmoCenter(bvg::Context& ctx, glm::mat4& viewproj,
                            glm::vec3 center, glm::vec2 mouse) {
    glm::vec3 centerS = worldToScreenSpace(ctx, center, viewproj);
    if(centerS.z >= 1.0f)
        return;
    ctx.beginPath();
    ctx.arc(centerS.x, centerS.y, 16.0f, 0.0f, M_PI * 2.0f);
    return ctx.isPointInsideConvexFill(mouse.x, mouse.y);
}

void drawGizmoUniformScale(bvg::Context& ctx, glm::mat4& viewproj,
                     glm::vec3 center, bvg::Color color) {
    glm::vec2 centerS = worldToScreenSpace(ctx, center, viewproj);
    ctx.fillStyle = bvg::SolidColor(color);
    ctx.beginPath();
    ctx.arc(centerS.x, centerS.y, 4.0f, 0.0f, M_PI * 2.0f);
    ctx.convexFill();
    
    bvg::Color fillColor = color;
    fillColor.a *= 0.5f;
    ctx.fillStyle = bvg::SolidColor(fillColor);
    ctx.strokeStyle = bvg::SolidColor(color);
    ctx.beginPath();
    ctx.rect(centerS.x - 14.0f, centerS.y - 14.0f, 28.0f, 28.0f);
    ctx.convexFill();
    ctx.lineWidth = 4.0f;
    ctx.stroke();
}

void drawGizmoPlane(bvg::Context& ctx, glm::mat4& viewproj,
                    glm::mat4& model, bvg::Color color) {
    std::vector<glm::vec3> vertices = {
        glm::vec3(-1.0f, 0.0f, -1.0f),
        glm::vec3(1.0f, 0.0f, -1.0f),
        glm::vec3(1.0f, 0.0f, 1.0f),
        glm::vec3(-1.0f, 0.0f, 1.0f)
    };
    
    for(int i = 0; i < vertices.size(); i++) {
        glm::vec3 proj = worldToScreenSpace(ctx, model * glm::vec4(vertices.at(i), 1.0f), viewproj);
        
        // If one of the vertices is too near to the
        //camera then then don't render the plane
        if(proj.z >= 1.0f)
            return;
        
        vertices.at(i) = proj;
    }
    glm::vec3& first = vertices.at(0);
    ctx.beginPath();
    ctx.moveTo(first.x, first.y);
    for(int i = 1; i < vertices.size(); i++) {
        glm::vec3& v = vertices.at(i);
        ctx.lineTo(v.x, v.y);
    }
    ctx.closePath();
    ctx.lineWidth = 4.0f;
    bvg::Color fillColor = color;
    fillColor.a *= 0.5f;
    ctx.fillStyle = bvg::SolidColor(fillColor);
    ctx.strokeStyle = bvg::SolidColor(color);
    ctx.convexFill();
    ctx.stroke();
}

std::vector<glm::vec3> createCircle(const int segments,
                                    float startAngle = 0.0f,
                                    float endAngle = glm::radians(360.0f)) {
    auto arcVerts = std::vector<glm::vec3>(segments);
    const float radius = 1.0f;
    float angle = startAngle;
    const float arcLength = endAngle - startAngle;
    for (int i = 0; i <= segments - 1; i++) {
        float x = sin(angle) * radius;
        float z = cos(angle) * radius;

        arcVerts.at(i) = glm::vec3(x, 0.0f, z);
        angle += arcLength / (segments - 1);
    }
    return arcVerts;
}

struct LineSegment {
    glm::vec3 start;
    glm::vec3 end;
    bool declined = false;
};

void buildGizmoOrbit(bvg::Context& ctx, glm::mat4& viewproj,
                     glm::mat4& model, glm::vec3 viewDir,
                     float lineWidth, bool isSelected, int segments = 99)
{
    std::vector<glm::vec3> vertices = createCircle(segments);
    for(int i = 0; i < vertices.size(); i++) {
        vertices.at(i) = model * glm::vec4(vertices.at(i), 1.0f);
    }
    
    glm::vec3 center = model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec3 centerUp = model * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    glm::vec3 up = glm::normalize(centerUp - center);
    
    std::vector<LineSegment> lineSegments(vertices.size() - 1);
    for(int i = 0; i < vertices.size() - 1; i++) {
        LineSegment& ls = lineSegments.at(i);
        ls.start = vertices.at(i);
        ls.end = vertices.at(i + 1);
        if(!isSelected) {
            glm::vec3 dir = glm::normalize(ls.start - ls.end);
            glm::vec3 perpDir = glm::cross(dir, up);
            float codir = glm::dot(perpDir, viewDir);
            if(codir < 0.0f)
                ls.declined = true;
        }
    }
    
    for(int i = 0; i < lineSegments.size(); i++) {
        LineSegment& ls = lineSegments.at(i);
        glm::vec3 projStart = worldToScreenSpace(ctx, ls.start, viewproj);
        glm::vec3 projEnd = worldToScreenSpace(ctx, ls.end, viewproj);
        
        // If one of the vertices is too near to the
        //camera then then don't render the plane
        if(projStart.z >= 1.0f || projEnd.z >= 1.0f)
            return;
        
        ls.start = projStart;
        ls.end = projEnd;
    }
    
    ctx.lineWidth = lineWidth;
    
    LineSegment& first = lineSegments.front();
    ctx.beginPath();
    ctx.moveTo(first.start.x, first.start.y);
    
    bool prevDeclined = false;
    for(int i = 0; i < lineSegments.size(); i++) {
        LineSegment& ls = lineSegments.at(i);
        if(ls.declined) {
            prevDeclined = true;
            continue;
        }
        if(prevDeclined) {
            ctx.moveTo(ls.start.x, ls.start.y);
            ctx.lineTo(ls.end.x, ls.end.y);
        } else {
            ctx.lineTo(ls.end.x, ls.end.y);
        }
        prevDeclined = false;
    }
}

void drawGizmoOrbitBins(bvg::Context& ctx, glm::mat4& viewproj,
                        glm::mat4& model, bvg::Color color,
                        glm::vec3 viewDir,
                        float lineWidth, int bins)
{
    ctx.lineWidth = lineWidth;
    std::vector<glm::vec3> vertices = createCircle(bins);
    for(int i = 0; i < vertices.size(); i++) {
        glm::vec3 vertex = vertices.at(i);
        glm::vec3 start = model * glm::vec4(vertex, 1.0f);
        start += start * 0.1f;
        glm::vec3 end = start + start * 0.2f;
        start = worldToScreenSpace(ctx, start, viewproj);
        end = worldToScreenSpace(ctx, end, viewproj);
        ctx.beginPath();
        ctx.moveTo(start.x, start.y);
        ctx.lineTo(end.x, end.y);
        ctx.stroke();
    }
}

void drawGizmoOrbit(bvg::Context& ctx, glm::mat4& viewproj,
                    glm::mat4& model, bvg::Color color, glm::vec3 viewDir,
                    bool isSelected) {
    buildGizmoOrbit(ctx, viewproj, model, viewDir, 5.0f, isSelected);
    ctx.strokeStyle = bvg::SolidColor(color);
    ctx.stroke();
}

bool isMouseOverGizmoOrbit(bvg::Context& ctx, glm::mat4& viewproj,
                                      glm::mat4& model, glm::vec3 viewDir,
                                      glm::vec2 mouse) {
    buildGizmoOrbit(ctx, viewproj, model, viewDir, 10.0f, false);
    return ctx.isPointInsideStroke(mouse.x, mouse.y);
}

void drawGizmoArcball(bvg::Context& ctx, glm::vec3 center, glm::mat4& viewproj,
                      float radius, bvg::Color color) {
    color.a *= 0.5f;
    glm::vec3 centerS = worldToScreenSpace(ctx, center, viewproj);
    ctx.fillStyle = bvg::SolidColor(color);
    ctx.beginPath();
    ctx.arc(centerS.x, centerS.y, radius, 0.0f, M_PI * 2.0f);
    ctx.convexFill();
}

glm::vec3 vertexLookAtEye(glm::vec3 v, glm::vec3& eye, glm::vec3& center) {
    v = glm::quat(glm::vec3(M_PI_2, 0.0f, 0.0f)) * v;
    v = glm::quatLookAt(glm::normalize(center - eye), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::vec4(v, 1.0f);
    return v;
}

void buildGizmoScreenSpaceOrbit(bvg::Context& ctx, glm::vec3 center, glm::mat4& viewproj,
                                float radius, glm::vec3 eye, int segments,
                                float lineWidth = 5.0f) {
    std::vector<glm::vec3> vertices = createCircle(segments);
    for(int i = 0; i < vertices.size(); i++) {
        glm::vec3& v = vertices.at(i);
        v *= radius;
        v = vertexLookAtEye(v, eye, center);
        v += center;
    }
    
    for(int i = 0; i < vertices.size(); i++) {
        glm::vec3& v = vertices.at(i);
        v = worldToScreenSpace(ctx, v, viewproj);
    }
    
    glm::vec3 centerS = worldToScreenSpace(ctx, center, viewproj);
    ctx.lineWidth = lineWidth;
    ctx.beginPath();
    glm::vec3& first = vertices.front();
    ctx.moveTo(first.x, first.y);
    for(int i = 1; i < vertices.size(); i++) {
        glm::vec3& v = vertices.at(i);
        ctx.lineTo(v.x, v.y);
    }
}

void drawGizmoScreenSpaceOrbit(bvg::Context& ctx, glm::vec3 center, glm::mat4& viewproj,
                               float radius, bvg::Color color, glm::vec3 eye) {
    buildGizmoScreenSpaceOrbit(ctx, center, viewproj, radius, eye, 48, 5.0f);
    ctx.strokeStyle = bvg::SolidColor(color);
    ctx.stroke();
}

void drawGizmoScreenSpaceOrbitPie(bvg::Context& ctx, glm::vec3 center, glm::mat4& viewproj,
                                  float radius, float startAngle, float endAngle,
                                  bvg::Color color, glm::vec3 eye) {
    std::vector<glm::vec3> vertices = createCircle(24, startAngle, endAngle);
    for(int i = 0; i < vertices.size(); i++) {
        glm::vec3& v = vertices.at(i);
        v *= radius;
        v = vertexLookAtEye(v, eye, center);
        v += center;
        v = worldToScreenSpace(ctx, v, viewproj);
    }
    
    glm::vec3 centerS = worldToScreenSpace(ctx, center, viewproj);
    ctx.beginPath();
    ctx.moveTo(centerS.x, centerS.y);
    for(int i = 0; i < vertices.size(); i++) {
        glm::vec3& v = vertices.at(i);
        ctx.lineTo(v.x, v.y);
    }
    color.a *= 0.333f;
    ctx.fillStyle = bvg::SolidColor(color);
    ctx.convexFill();
}

float signedDistancePlanePoint(glm::vec3 planePoint, glm::vec3 planeNormal, glm::vec3 point) {
    return glm::dot(planeNormal, point - planePoint);
}

glm::vec3 closestPointOnPlane(glm::vec3 point, glm::vec3 planePoint, glm::vec3 planeNormal) {
    float distance = signedDistancePlanePoint(planePoint, planeNormal, point);
    return point - planeNormal * distance;
}

Ray screenPointToRay(bvg::Context& ctx, glm::vec2 point, glm::mat4& viewproj) {
    Ray ray = Ray();

    // screen coords to clip coords
    float normalizedX = point.x / (ctx.width * 0.5f) - 1.0f;
    float normalizedY = point.y / (ctx.height * 0.5f) - 1.0f;

    glm::mat4 invVP = glm::inverse(viewproj);

    glm::vec4 originClipSpace { normalizedX, -normalizedY, -1, 1 };
    glm::vec4 destClipSpace { normalizedX, -normalizedY, 1, 1 };
    glm::vec4 originClipSpaceWS = invVP * originClipSpace;
    glm::vec4 destClipSpaceWS = invVP * destClipSpace;
    glm::vec3 originClipSpaceWS3 = glm::vec3(originClipSpaceWS) / originClipSpaceWS.w;
    glm::vec3 destClipSpaceWS3 = glm::vec3(destClipSpaceWS) / destClipSpaceWS.w;

    ray.origin = originClipSpaceWS3;
    ray.direction = glm::normalize(destClipSpaceWS3 - originClipSpaceWS3);

    return ray;
}

glm::vec3 intersectRayPlane(Ray ray, glm::vec3 planePoint, glm::vec3 planeNormal) {
    float distance;
    bool intersects = glm::intersectRayPlane(ray.origin, ray.direction, planePoint,
                                             planeNormal, distance);
    return ray.origin + ray.direction * distance;
}

glm::vec3 projectScreenPointOnPlane(bvg::Context& ctx, glm::vec2 point, glm::mat4& viewproj,
                                    glm::vec3 planePoint, glm::vec3 planeNormal) {
    Ray ray = screenPointToRay(ctx, point, viewproj);
    return intersectRayPlane(ray, planePoint, planeNormal);
}

float angleOnOrbit(bvg::Context& ctx, glm::mat4& model, glm::mat4& viewproj,
                   glm::vec3 projectedPoint) {
    glm::vec3 center = model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec3 centerUpX = model * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    glm::vec3 centerUp = model * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    glm::vec3 up = glm::normalize(centerUp - center);
    glm::vec3 A = glm::normalize(centerUpX - center);
    glm::vec3 B = glm::normalize(projectedPoint - center);
    float angle = glm::orientedAngle(A, B, up);
    return angle + M_PI_2;
}

void drawGizmoOrbitPie(bvg::Context& ctx, glm::mat4& model, glm::mat4& viewproj,
                       float startAngle, float endAngle,
                       bvg::Color color, glm::vec3 eye) {
    glm::vec3 center = model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    
    std::vector<glm::vec3> vertices = createCircle(24, startAngle, endAngle);
    for(int i = 0; i < vertices.size(); i++) {
        glm::vec3& v = vertices.at(i);
        v = model * glm::vec4(v, 1.0f);
        v = worldToScreenSpace(ctx, v, viewproj);
    }
    glm::vec3 centerS = worldToScreenSpace(ctx, center, viewproj);
    
//    ctx.fillStyle = bvg::SolidColor(color);
//    ctx.lineWidth = 3.0f;
//
//    glm::vec3 aS = worldToScreenSpace(ctx, centerUpX, viewproj);
//    ctx.beginPath();
//    ctx.arc(aS.x, aS.y, 5.0f, 0.0f, M_PI * 2.0f);
//    ctx.convexFill();
//    ctx.beginPath();
//    ctx.moveTo(centerS.x, centerS.y);
//    ctx.lineTo(aS.x, aS.y);
//    ctx.stroke();
//
////    std::cout << projectedMouse.y << std::endl;
////    std::cout << glm::distance(projectedMouse, eye) << std::endl;
//    glm::vec3 bS = worldToScreenSpace(ctx, projectedMouse, viewproj);
//    ctx.beginPath();
//    ctx.arc(bS.x, bS.y, 5.0f, 0.0f, M_PI * 2.0f);
//    ctx.convexFill();
//    ctx.beginPath();
//    ctx.moveTo(centerS.x, centerS.y);
//    ctx.lineTo(bS.x, bS.y);
//    ctx.stroke();
//
//    glm::vec3 nS = worldToScreenSpace(ctx, centerUp, viewproj);
//    ctx.beginPath();
//    ctx.arc(nS.x, nS.y, 5.0f, 0.0f, M_PI * 2.0f);
//    ctx.convexFill();
//    ctx.beginPath();
//    ctx.moveTo(centerS.x, centerS.y);
//    ctx.lineTo(nS.x, nS.y);
//    ctx.stroke();
    
    ctx.beginPath();
    ctx.moveTo(centerS.x, centerS.y);
    for(int i = 0; i < vertices.size(); i++) {
        glm::vec3& v = vertices.at(i);
        ctx.lineTo(v.x, v.y);
    }
    color.a *= 0.333f;
    ctx.fillStyle = bvg::SolidColor(color);
    ctx.convexFill();
}

bool isMouseOverGizmoScreenSpaceOrbit(bvg::Context& ctx, glm::vec3 center, glm::mat4& viewproj,
                               float radius, glm::vec3 eye, glm::vec2 mouse) {
    buildGizmoScreenSpaceOrbit(ctx, center, viewproj, radius, eye, 16, 10.0f);
    return ctx.isPointInsideStroke(mouse.x, mouse.y);
}

enum class DrawingType {
    Other,
    Arrow,
    Center
};

class Drawing {
public:
    Drawing();
    
    virtual void draw(bvg::Context& ctx, GizmoState& state);
    virtual bool mouseEvent(bvg::Context& ctx, GizmoState& state, glm::mat4& model,
                            bool isMouseDown, float mouseX, float mouseY);
    
    DrawingType type = DrawingType::Other;
    float distanceToEye = 0;
};

Drawing::Drawing()
{
}

void Drawing::draw(bvg::Context& ctx, GizmoState& state)
{
}

bool Drawing::mouseEvent(bvg::Context& ctx, GizmoState& state, glm::mat4& model,
                         bool isMouseDown, float mouseX, float mouseY) {
    return false;
}

class PlaneDrawing : public Drawing {
public:
    PlaneDrawing(glm::mat4& viewproj, glm::mat4& model,
                 bvg::Color color, glm::vec3 eye,
                 glm::vec3 target);
    
    glm::mat4& viewproj;
    glm::mat4& model;
    bvg::Color color;
    
    void draw(bvg::Context& ctx, GizmoState& state);
};

PlaneDrawing::PlaneDrawing(glm::mat4& viewproj, glm::mat4& model,
                           bvg::Color color, glm::vec3 eye,
                           glm::vec3 target):
viewproj(viewproj),
model(model),
color(color)
{
    glm::vec3 center = model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec3 centerUp = model * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    glm::vec3 planeDir = glm::normalize(centerUp - center);
    glm::vec3 viewDir = glm::normalize(center - eye);
    float codir = fabsf(glm::dot(planeDir, viewDir));
    this->color.a *= fminf(codir * 4.0f, 1.0f);
    
    this->distanceToEye = glm::distance(center, eye);
}

void PlaneDrawing::draw(bvg::Context& ctx, GizmoState& state) {
    if(state.selectedControl == Control::Arrow ||
       state.selectedControl == Control::Center) {
        color.a *= 0.5f;
    }
    drawGizmoPlane(ctx, viewproj, model, color);
}

class ArrowDrawing : public Drawing {
public:
    ArrowDrawing(glm::mat4& viewproj,
                 glm::vec3 origin, glm::vec3 end,
                 bvg::Color color,
                 Axis axis,
                 glm::vec3 eye,
                 glm::vec3 target,
                 bool forScaling = false);
    
    glm::mat4& viewproj;
    glm::vec3 origin;
    glm::vec3 end;
    bvg::Color color;
    Axis axis;
    bool forScaling;
    
    void draw(bvg::Context& ctx, GizmoState& state);
    bool mouseEvent(bvg::Context& ctx, GizmoState& state, glm::mat4& model,
                    bool isMouseDown, float mouseX, float mouseY);
};

ArrowDrawing::ArrowDrawing(glm::mat4& viewproj,
                           glm::vec3 origin, glm::vec3 end,
                           bvg::Color color,
                           Axis axis,
                           glm::vec3 eye,
                           glm::vec3 target,
                           bool forScaling):
viewproj(viewproj),
origin(origin),
end(end),
color(color),
forScaling(forScaling),
axis(axis)
{
    glm::vec3 center = (origin + end) / 2.0f;
    glm::vec3 arrowDir = glm::normalize(end - origin);
    glm::vec3 viewDir = glm::normalize(center - eye);
    float codir = 1.0f - fabsf(glm::dot(arrowDir, viewDir));
    this->color.a *= fminf(codir * 20.0f, 1.0f);
    
    type = DrawingType::Arrow;
    this->distanceToEye = glm::distance(center, eye);
}

void ArrowDrawing::draw(bvg::Context& ctx, GizmoState& state) {
    if((state.selectedControl == Control::Arrow
         && state.axis != axis) ||
       state.selectedControl == Control::Center) {
        color.a *= 0.5f;
    }
    drawGizmoArrow(ctx, viewproj, origin, end, color, forScaling);
}

glm::vec3 projectMouseOnLine(bvg::Context& ctx, glm::vec3 mouse, glm::mat4& viewproj,
                             glm::vec3 lineOrigin, glm::vec3 lineDir)
{
    glm::vec3 world = screenToWorldSpace(ctx, mouse, viewproj);
    return glm::closestPointOnLine(world, lineOrigin - lineDir * 1000.0f,
                                   lineOrigin + lineDir * 1000.0f);
}

bool ArrowDrawing::mouseEvent(bvg::Context& ctx, GizmoState& state, glm::mat4& model,
                              bool isMouseDown, float mouseX, float mouseY)
{
    float z = worldToScreenSpace(ctx, origin, viewproj).z;
    glm::vec3 mouse = glm::vec3(mouseX, mouseY, z);
    glm::vec3 lineDir = glm::normalize(end - origin);
    
    glm::vec3 projectedPoint = projectMouseOnLine(ctx, mouse, viewproj, origin, lineDir);
    
    if(state.selectedControl == Control::Arrow &&
       state.axis == axis) {
        glm::vec3 differenceOld = state.offset - state.translation;
        glm::vec3 differenceNew = projectedPoint - state.translation;
        model = glm::translate(differenceNew) *
            glm::translate(-differenceOld) * model;
        state.offset = projectedPoint;
    }
    
    if(isMouseOverGizmoArrow(ctx, viewproj, origin, end, mouse)) {
        if(!isMouseDown) {
            state.controlOverMouse = Control::Arrow;
            return true;
        }
        if(state.controlOverMouse == Control::Arrow &&
           state.selectedControl != Control::Arrow) {
            state.selectedControl = Control::Arrow;
            state.axis = axis;
            state.offset = projectedPoint;
        }
        return true;
    }
}

class CenterDrawing : public Drawing {
public:
    CenterDrawing(glm::mat4& viewproj,
                 glm::vec3 center, bvg::Color color,
                 glm::vec3 eye);
    
    glm::mat4& viewproj;
    glm::vec3 center;
    bvg::Color color;
    
    void draw(bvg::Context& ctx, GizmoState& state);
    bool mouseEvent(bvg::Context& ctx, GizmoState& state, glm::mat4& model,
                    bool isMouseDown, float mouseX, float mouseY);
};

CenterDrawing::CenterDrawing(glm::mat4& viewproj,
                           glm::vec3 center, bvg::Color color,
                           glm::vec3 eye):
viewproj(viewproj),
center(center),
color(color)
{
    type = DrawingType::Center;
    this->distanceToEye = glm::distance(this->center, eye);
}

void CenterDrawing::draw(bvg::Context& ctx, GizmoState& state) {
    bvg::Color currentColor = this->color;
    if(state.selectedControl == Control::Center) {
        currentColor = bvg::Color::lerp(this->color, bvg::colors::White, 0.5f);
    }
    else if(state.selectedControl == Control::Arrow) {
        currentColor.a *= 0.5f;
    }
    drawGizmoCenter(ctx, viewproj, center, currentColor);
}

glm::vec3 mouseAsWorldPoint(bvg::Context& ctx, GizmoState& state, glm::mat4& viewproj,
                            float mouseX, float mouseY) {
    return screenToWorldSpace(ctx, glm::vec3(mouseX, mouseY, state.z), viewproj);
}

bool CenterDrawing::mouseEvent(bvg::Context& ctx, GizmoState& state, glm::mat4& model,
                               bool isMouseDown, float mouseX, float mouseY) {
    if(state.selectedControl == Control::Center) {
        model = glm::translate(mouseAsWorldPoint(ctx, state, viewproj, mouseX, mouseY) +
                               state.offset) *
                glm::translate(-state.translation) * model;
    }
    
    if(isMouseOverGizmoCenter(ctx, viewproj, center, glm::vec2(mouseX, mouseY))) {
        if(!isMouseDown) {
            state.controlOverMouse = Control::Center;
            return true;
        }
        if(state.controlOverMouse == Control::Center &&
           state.selectedControl != Control::Center) {
            state.selectedControl = Control::Center;
            state.z = worldToScreenSpace(ctx, this->center, this->viewproj).z;
            state.offset = this->center - mouseAsWorldPoint(ctx, state, viewproj, mouseX, mouseY);
        }
        return true;
    }
    return false;
}

class OrbitDrawing : public Drawing {
public:
    OrbitDrawing(glm::mat4& viewproj, glm::mat4& model,
                 Axis axis, bvg::Color color,
                 glm::vec3 eye, glm::vec3 target);
    
    glm::mat4& viewproj;
    glm::mat4& model;
    bvg::Color color;
    glm::vec3 viewDir;
    glm::vec3 eye;
    Axis axis;
    
    void draw(bvg::Context& ctx, GizmoState& state);
    bool mouseEvent(bvg::Context& ctx, GizmoState& state, glm::mat4& model,
                    bool isMouseDown, float mouseX, float mouseY);
};

OrbitDrawing::OrbitDrawing(glm::mat4& viewproj, glm::mat4& model,
                           Axis axis, bvg::Color color,
                           glm::vec3 eye, glm::vec3 target):
viewproj(viewproj),
model(model),
color(color),
axis(axis),
eye(eye)
{
    glm::vec3 center = model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    viewDir = glm::normalize(center - eye);
    this->distanceToEye = glm::distance(center, eye);
}

void OrbitDrawing::draw(bvg::Context& ctx, GizmoState& state) {
    if(state.selectedControl == Control::Orbit &&
       state.axis != axis &&
       state.startAngle != state.angle) {
        color.a *= 0.5f;
    }
    else if(state.selectedControl == Control::SSOrbit &&
            state.startAngle != state.angle) {
        color.a *= 0.5;
    }
    bool isCurrentSelected = state.selectedControl == Control::Orbit &&
        state.axis == axis && state.startAngle != state.angle;
    
//    glm::vec3 center = this->model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
//    glm::vec3 centerUp = this->model * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
//    glm::vec3 up = glm::normalize(centerUp - center);
//
//    glm::mat4 newModel = glm::translate(state.translation) *
//        glm::toMat4(glm::quatLookAt(up, glm::vec3(0.0f, 1.0f, 0.0f))) *
//        glm::scale(glm::vec3(2.0f));
    
    drawGizmoOrbit(ctx, viewproj, model, color, viewDir, isCurrentSelected);
    if(isCurrentSelected) {
        float startAngle = angleOnOrbit(ctx, model, viewproj, state.startPiePoint);
        float endAngle = angleOnOrbit(ctx, model, viewproj, state.piePoint);
        
        drawGizmoOrbitPie(ctx, model, viewproj,
                          startAngle,
                          endAngle,
                          color, eye);
            
        glm::vec3 basis;
        switch (axis) {
            case Axis::X:
                basis = glm::vec3(0.0f, -1.0f, 0.0f);
                break;
            case Axis::Y:
                basis = glm::vec3(1.0f, 0.0f, 0.0f);
                break;
            case Axis::Z:
                basis = glm::vec3(0.0f, 0.0f, -1.0f);
                break;
        }
        
//        glm::vec3 center = this->model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
//        glm::vec3 centerUp = this->model * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
//        glm::vec3 up = glm::normalize(centerUp - center);
//
//        float side = glm::dot(up, viewDir);
//        float relativeAngle = state.startAngle - state.angle;
//        if(side > 0.0f)
//            relativeAngle = -relativeAngle;
//
//        glm::mat4 binsModel =
//            glm::translate(state.translation) *
//            glm::toMat4(state.rotation) *
//            glm::rotate(relativeAngle, basis) *
//            glm::toMat4(glm::inverse(state.rotation)) *
//            glm::translate(-state.translation) *
//            model;
    
        drawGizmoOrbitBins(ctx, viewproj, model, color, viewDir,
                           3.0f, 16);
    }
}

bool OrbitDrawing::mouseEvent(bvg::Context& ctx, GizmoState& state,
                                         glm::mat4& model, bool isMouseDown,
                                         float mouseX, float mouseY) {
    glm::vec3 center = this->model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec3 centerUp = this->model * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    glm::vec2 centerS = worldToScreenSpace(ctx, center, viewproj);
    glm::vec3 up = glm::normalize(centerUp - center);
    glm::vec2 mouse = glm::vec2(mouseX, mouseY);
    glm::vec2 relativeMouse = mouse - centerS;
    relativeMouse = glm::rotate(glm::mat3(1.0f), -state.angle) * glm::vec3(relativeMouse, 1.0f);
    
    float relativeAngle = -atan2f(relativeMouse.x, relativeMouse.y);
    float side = glm::dot(up, viewDir);
    float orientedRelativeAngle = side > 0.0f ? -relativeAngle : relativeAngle;
    float angle = state.angle + relativeAngle;
    
    if(state.selectedControl == Control::Orbit &&
       state.axis == axis) {
        float currentAngle = 0.0f;
        glm::vec3 basis;
        switch (axis) {
            case Axis::X:
                basis = glm::vec3(0.0f, -1.0f, 0.0f);
                currentAngle = glm::yaw(state.rotation);
                break;
            case Axis::Y:
                basis = glm::vec3(1.0f, 0.0f, 0.0f);
                currentAngle = glm::pitch(state.rotation);
                break;
            case Axis::Z:
                basis = glm::vec3(0.0f, 0.0f, -1.0f);
                currentAngle = glm::roll(state.rotation);
                break;
        }
        float bins = floorf(currentAngle / M_PI_4);
        float discreteAngle = bins * M_PI_4;
        model =
            glm::translate(state.translation) *
            glm::toMat4(state.rotation) *
            glm::rotate(orientedRelativeAngle, basis) *
            glm::toMat4(glm::inverse(state.rotation)) *
            glm::translate(-state.translation) *
            model;
        state.angle = angle;
        state.piePoint = projectScreenPointOnPlane(ctx, mouse, viewproj, center, up);
    }
    
    if(isMouseOverGizmoOrbit(ctx, viewproj, this->model, viewDir, mouse)) {
        if(!isMouseDown) {
            state.controlOverMouse = Control::Orbit;
            return true;
        }
        if(state.controlOverMouse == Control::Orbit &&
           state.selectedControl != Control::Orbit) {
            state.selectedControl = Control::Orbit;
            state.axis = axis;
            state.angle = angle;
            state.startAngle = angle;
            state.startPiePoint = projectScreenPointOnPlane(ctx, mouse, viewproj, center, up);
            return true;
        }
    }
    return false;
}

class ArcballDrawing : public Drawing {
public:
    ArcballDrawing(glm::mat4& viewproj, glm::vec3 center,
                   float radius, bvg::Color color, glm::vec3 eye);
    
    glm::mat4& viewproj;
    glm::vec3 center;
    bvg::Color color;
    float radius;
    
    void draw(bvg::Context& ctx, GizmoState& state);
};

ArcballDrawing::ArcballDrawing(glm::mat4& viewproj, glm::vec3 center,
                               float radius, bvg::Color color, glm::vec3 eye):
viewproj(viewproj),
color(color),
radius(radius),
center(center)
{
    this->distanceToEye = glm::distance(center, eye);
}

void ArcballDrawing::draw(bvg::Context& ctx, GizmoState& state) {
    bvg::Color currentColor = color;
    currentColor.a = 0.0f;
    drawGizmoArcball(ctx, center, viewproj, radius, currentColor);
}

class ScreenSpaceOrbitDrawing : public Drawing {
public:
    ScreenSpaceOrbitDrawing(glm::mat4& viewproj, glm::vec3 center,
                   float radius, bvg::Color color, glm::vec3 eye);
    
    glm::mat4& viewproj;
    glm::vec3 center;
    bvg::Color color;
    float radius;
    glm::vec3 eye;
    
    void draw(bvg::Context& ctx, GizmoState& state);
    bool mouseEvent(bvg::Context& ctx, GizmoState& state, glm::mat4& model,
                    bool isMouseDown, float mouseX, float mouseY);
};

ScreenSpaceOrbitDrawing::ScreenSpaceOrbitDrawing(glm::mat4& viewproj, glm::vec3 center,
                               float radius, bvg::Color color, glm::vec3 eye):
viewproj(viewproj),
color(color),
radius(radius),
center(center),
eye(eye)
{
    this->distanceToEye = glm::distance(center, eye);
}

void ScreenSpaceOrbitDrawing::draw(bvg::Context& ctx, GizmoState& state) {
    if(state.selectedControl == Control::Orbit &&
       state.startAngle != state.angle) {
        color.a *= 0.5f;
    }
    if(state.selectedControl == Control::SSOrbit) {
        float arcLength = -state.startAngle + state.angle;
        float fullArcs;
        if(arcLength > 0)
            fullArcs = floorf(arcLength / glm::radians(360.0f));
        else
            fullArcs = ceilf(arcLength / glm::radians(360.0f));
        float disreteArcLength = glm::radians(360.0f) * fullArcs;
        
        for(int i = 0; i < (int)fabsf(fullArcs); i++) {
            drawGizmoScreenSpaceOrbitPie(ctx, center, viewproj, radius,
                                         0.0f, M_PI * 2.0f,
                                         color, eye);
        }
        
        drawGizmoScreenSpaceOrbitPie(ctx, center, viewproj, radius,
                                     -state.startAngle, -state.angle + disreteArcLength,
                                     color, eye);
    }
    drawGizmoScreenSpaceOrbit(ctx, center, viewproj, radius, color, eye);
}

bool ScreenSpaceOrbitDrawing::mouseEvent(bvg::Context& ctx, GizmoState& state,
                                         glm::mat4& model, bool isMouseDown,
                                         float mouseX, float mouseY) {
    glm::vec2 centerS = worldToScreenSpace(ctx, center, viewproj);
    glm::vec2 mouse = glm::vec2(mouseX, mouseY);
    glm::vec2 relativeMouse = mouse - centerS;
    relativeMouse = glm::rotate(glm::mat3(1.0f), -state.angle) * glm::vec3(relativeMouse, 1.0f);
    
    float relativeAngle = -atan2f(relativeMouse.x, relativeMouse.y);
    float angle = state.angle + relativeAngle;
    
    if(state.selectedControl == Control::SSOrbit) {
        glm::vec3 viewDir = center - eye;
        model =
            glm::translate(state.translation) *
            glm::rotate(relativeAngle, viewDir) *
            glm::translate(-state.translation) *
            model;
        state.angle = angle;
    }
    
    if(isMouseOverGizmoScreenSpaceOrbit(ctx, center, viewproj, radius, eye, mouse)) {
        if(!isMouseDown) {
            state.controlOverMouse = Control::SSOrbit;
            return true;
        }
        if(state.controlOverMouse == Control::SSOrbit &&
           state.selectedControl != Control::SSOrbit) {
            state.selectedControl = Control::SSOrbit;
            state.angle = angle;
            state.startAngle = angle;
        }
        return true;
    }
    return false;
}

class UniformScaleDrawing : public Drawing {
public:
    UniformScaleDrawing(glm::mat4& viewproj,
                 glm::vec3 center, bvg::Color color,
                 glm::vec3 eye);
    
    glm::mat4& viewproj;
    glm::vec3 center;
    bvg::Color color;
    
    void draw(bvg::Context& ctx, GizmoState& state);
};

UniformScaleDrawing::UniformScaleDrawing(glm::mat4& viewproj,
                           glm::vec3 center, bvg::Color color,
                           glm::vec3 eye):
viewproj(viewproj),
center(center),
color(color)
{
    type = DrawingType::Center;
    this->distanceToEye = glm::distance(this->center, eye);
}

void UniformScaleDrawing::draw(bvg::Context& ctx, GizmoState& state) {
    bvg::Color currentColor = this->color;
    if(state.selectedControl == Control::UniformScale) {
        currentColor = bvg::Color::lerp(this->color, bvg::colors::White, 0.5f);
    }
    drawGizmoUniformScale(ctx, viewproj, center, currentColor);
}

struct DrawingWrapper {
    DrawingWrapper(Drawing* drawing);
    
    Drawing* drawing;
    
    bool operator > (const DrawingWrapper& other) const;
};

DrawingWrapper::DrawingWrapper(Drawing* drawing):
drawing(drawing)
{
}

bool DrawingWrapper::operator > (const DrawingWrapper& other) const {
    if(this->drawing->type == DrawingType::Arrow &&
       other.drawing->type == DrawingType::Center) {
        return true;
    }
    if(this->drawing->type == DrawingType::Center &&
       other.drawing->type == DrawingType::Arrow) {
        return false;
    }
    return this->drawing->distanceToEye > other.drawing->distanceToEye;
}

void drawButton(bvg::Context& ctx, bvg::Color color, float buttonSize, float padding) {
    ctx.fillStyle = bvg::SolidColor(color);
    ctx.beginPath();
    ctx.rect(0, 0, buttonSize, buttonSize, 14);
    ctx.convexFill();
}

void drawMoveButton(bvg::Context& ctx, bvg::Color color, float buttonSize, float padding,
                    glm::vec3 origin, glm::vec3 direction,
                    glm::mat4 viewproj, glm::vec3 eye, glm::vec3 target) {
    bvg::Color buttonColor = color;
    buttonColor.a = 0.333f;
    drawButton(ctx, buttonColor, buttonSize, padding);
    bvg::Style iconStyle = bvg::SolidColor(color);
    ctx.strokeStyle = iconStyle;
    ctx.fillStyle = iconStyle;
    ctx.lineWidth = 2.0f;
    float arrowLength = 24;
    glm::vec2 center = glm::vec2(buttonSize) / 2.0f;
    glm::vec2 targetS = worldToScreenSpace(ctx, target + origin, viewproj);
    glm::vec2 vectorS = worldToScreenSpace(ctx, target + origin + direction, viewproj);
    glm::vec2 arrowDirection = glm::normalize(vectorS - targetS);
    glm::vec2 start = center - arrowDirection * arrowLength / 2.0f;
    glm::vec2 end = center + arrowDirection * (arrowLength / 2.0f - 12);
    drawArrow(ctx, start.x, start.y, end.x, end.y, 8);
    ctx.translate(buttonSize + padding, 0.0f);
}

void drawRotateButton(bvg::Context& ctx, bvg::Color color, float buttonSize, float padding,
                    glm::vec3 origin, glm::mat4 model,
                    glm::mat4 viewproj, glm::vec3 eye, glm::vec3 target) {
    bvg::Color buttonColor = color;
    buttonColor.a = 0.333f;
    drawButton(ctx, buttonColor, buttonSize, padding);
    bvg::Style iconStyle = bvg::SolidColor(color);
    ctx.strokeStyle = iconStyle;
    ctx.fillStyle = iconStyle;
    ctx.lineWidth = 2.0f;
    float arrowLength = 24;
    glm::vec2 center = glm::vec2(buttonSize) / 2.0f;
    glm::vec3 targetS = worldToScreenSpace(ctx, target, viewproj);
    
    float radius = glm::distance(target, eye) / 48.0f;
    
//    std::vector<glm::vec3> vertices = createCircle(16, 0.0f, M_PI * 2.0f - M_PI_2);
    std::vector<glm::vec3> vertices = createCircle(16, M_PI * 2.0f - M_PI_2 * 0.667f, 0.0f);
    for(int i = 0; i < vertices.size(); i++) {
        glm::vec3& v = vertices.at(i);
        v *= radius;
        v = model * glm::vec4(v, 1.0f);
        v = worldToScreenSpace(ctx, v, viewproj) - targetS + glm::vec3(center, 0.0f);
    }
    
    ctx.beginPath();
    glm::vec3& first = vertices.front();
    ctx.moveTo(first.x, first.y);
    for(int i = 1; i < vertices.size(); i++) {
        glm::vec3& v = vertices.at(i);
        ctx.lineTo(v.x, v.y);
    }
    ctx.stroke();
    glm::vec3& beforelast = vertices.at(vertices.size() - 2);
    glm::vec3& last = vertices.at(vertices.size() - 1);
    glm::vec3 lastPointingFirst = beforelast + (first - beforelast) * 0.01f;;
    glm::vec3 end = glm::mix(last, lastPointingFirst, 0.9f);
    buildLineTriangleTip(ctx, beforelast.x, beforelast.y, end.x, end.y, 6);
    ctx.convexFill();
    
    ctx.translate(buttonSize + padding, 0.0f);
}

void drawGizmosNew(bvg::Context& ctx,  GizmoState& state, GizmoTool tool,
                   glm::mat4 viewproj, glm::mat4& model,
                   glm::vec3 eye, glm::vec3 target, glm::vec3 up,
                   bool isMouseDown, float mouseX, float mouseY) {
    glm::vec3 center = model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec2 centerS = worldToScreenSpace(ctx, center, viewproj);
    ctx.beginPath();
    ctx.clearTransform();
    ctx.translate(centerS.x, centerS.y);
    float buttonSize = 40.0f;
    float padding = 6.0f;
    float width = padding + (buttonSize + padding) * 3.0f;
    float height = buttonSize + padding * 2.0f;
    float offsetY = 60.0f;
    bvg::Color panelColor = bvg::Color(0.8f, 0.8f, 0.8f);
    ctx.fillStyle = bvg::SolidColor(panelColor);
    ctx.rect(-width / 2.0f, offsetY, width, height, 18);
    ctx.convexFill();
    ctx.translate(0.0f, offsetY - 15);
    ctx.beginPath();
    ctx.moveTo(-20, 15);
    ctx.cubicTo(-5, 15, -5, 0, 0, 0);
    ctx.cubicTo(5, 0, 5, 15, 20, 15);
    ctx.fill();
    ctx.clearTransform();
    ctx.translate(centerS.x, centerS.y);
    ctx.translate(-width / 2.0f + padding, offsetY + padding);
    bvg::Color XColor = bvg::Color(1.0f, 0.2f, 0.2f);
    bvg::Color YColor = bvg::Color(0.1f, 0.6f, 0.1f);
    bvg::Color ZColor = bvg::Color(0.2f, 0.2f, 1.0f);
    switch(tool) {
        case GizmoTool::Translate:
        {
            glm::vec3 X = state.rotation * glm::vec3(0, 0, 1);
            glm::vec3 Y = state.rotation * glm::vec3(0, 1, 0);
            glm::vec3 Z = state.rotation * glm::vec3(1, 0, 0);
            
            drawMoveButton(ctx, ZColor, buttonSize, padding, center, X, viewproj, eye, target);
            drawMoveButton(ctx, YColor, buttonSize, padding, center, Y, viewproj, eye, target);
            drawMoveButton(ctx, XColor, buttonSize, padding, center, Z, viewproj, eye, target);
        }
            break;
        case GizmoTool::Rotate:
        {
            glm::mat4 rotationMat = glm::toMat4(state.rotation);
            
            glm::mat4 XOrbitMat =
                rotationMat;
            glm::mat4 YOrbitMat =
                rotationMat *
                glm::rotate((float)M_PI_2, glm::vec3(0.0f, 0.0f, 1.0f));
            glm::mat4 ZOrbitMat =
                rotationMat *
                glm::rotate((float)M_PI_2, glm::vec3(1.0f, 0.0f, 0.0f));
            
            drawRotateButton(ctx, ZColor, buttonSize, padding, center,
                             ZOrbitMat, viewproj, eye, target);
            drawRotateButton(ctx, YColor, buttonSize, padding, center,
                             YOrbitMat, viewproj, eye, target);
            drawRotateButton(ctx, XColor, buttonSize, padding, center,
                             XOrbitMat, viewproj, eye, target);
        }
            break;
        case GizmoTool::Scale:
            break;
    }
};

void drawGizmos(bvg::Context& ctx,  GizmoState& state, GizmoTool tool,
                glm::mat4 viewproj, glm::mat4& model,
                glm::vec3 eye, glm::vec3 target, glm::vec3 up,
                bool isMouseDown, float mouseX, float mouseY)
{
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(model, state.scale, state.rotation, state.translation, skew, perspective);
    
    glm::mat4 rotationMat = glm::toMat4(state.rotation);
    
    float gizmoScale = 0.4f;
    gizmoScale *= glm::distance(state.translation, eye) / 15.0f;
    float arrowLength = 5.0f * gizmoScale;
    float planeSize = 1.0f * gizmoScale;
    float planeDistance = 3.0f * gizmoScale;
    float orbitRadius = 5.5f * gizmoScale;
    
    float eyeDistance = glm::distance(state.translation, eye);
    float minDistance = 7.5f;
    float visibleDistance = 10.0f;
    float opacity = remapf(minDistance, visibleDistance, 0.0f, 1.0f, eyeDistance);
    opacity = clampf(0.0f, opacity, 1.0f);
    
    // If gizmo is beind the camera
    if(worldToScreenSpace(ctx, state.translation, viewproj).z > 1.0f ||
       eyeDistance < 7.5f) {
        return;
    }
    
    if(!isMouseDown) {
        state.controlOverMouse = Control::None;
        state.selectedControl = Control::None;
    }
    
    state.mouse = glm::vec2(mouseX, mouseY);
    
    glm::vec3 viewForward = glm::normalize(target - eye);
    state.viewRight = glm::normalize(glm::cross(up, viewForward));
    state.viewUp = glm::cross(viewForward, state.viewRight);
    
    std::vector<DrawingWrapper> drawings;
    
    glm::vec3 center = glm::vec3(0.0f);
    center = state.translation + center;
    
    bvg::Color XColor = bvg::Color(1.0f, 0.2f, 0.2f);
    bvg::Color YColor = bvg::Color(0.2f, 1.0f, 0.2f);
    bvg::Color ZColor = bvg::Color(0.2f, 0.2f, 1.0f);
    bvg::Color centerColor = bvg::Color(0.75f, 0.75f, 0.75f);
    XColor.a *= opacity;
    YColor.a *= opacity;
    ZColor.a *= opacity;
    centerColor.a *= opacity;
    
    switch (tool) {
        case GizmoTool::Translate:
        {
            glm::vec3 XArrowEnd = glm::vec3(1.0f, 0.0f, 0.0f) * arrowLength;
            glm::vec3 YArrowEnd = glm::vec3(0.0f, 1.0f, 0.0f) * arrowLength;
            glm::vec3 ZArrowEnd = glm::vec3(0.0f, 0.0f, 1.0f) * arrowLength;
            XArrowEnd = state.translation + state.rotation * XArrowEnd;
            YArrowEnd = state.translation + state.rotation * YArrowEnd;
            ZArrowEnd = state.translation + state.rotation * ZArrowEnd;
            glm::mat4 XPlaneMat =
                glm::translate(state.translation) *
                rotationMat *
                glm::translate(glm::vec3(0.0f, 1.0f, 1.0f) * planeDistance) *
                glm::rotate((float)M_PI_2, glm::vec3(0.0f, 0.0f, 1.0f)) *
                glm::scale(glm::vec3(planeSize));
            glm::mat4 YPlaneMat =
                glm::translate(state.translation) *
                rotationMat *
                glm::translate(glm::vec3(1.0f, 0.0f, 1.0f) * planeDistance) *
                glm::scale(glm::vec3(planeSize));
            glm::mat4 ZPlaneMat =
                glm::translate(state.translation) *
                rotationMat *
                glm::translate(glm::vec3(1.0f, 1.0f, 0.0f) * planeDistance) *
                glm::rotate((float)M_PI_2, glm::vec3(1.0f, 0.0f, 0.0f)) *
                glm::scale(glm::vec3(planeSize));
            
            drawings = {
                DrawingWrapper(new ArrowDrawing(viewproj, center, XArrowEnd, XColor,
                                                Axis::X, eye, target)),
                DrawingWrapper(new ArrowDrawing(viewproj, center, YArrowEnd, YColor,
                                                Axis::Y, eye, target)),
                DrawingWrapper(new ArrowDrawing(viewproj, center, ZArrowEnd, ZColor,
                                                Axis::Z, eye, target)),
                DrawingWrapper(new PlaneDrawing(viewproj, XPlaneMat, XColor, eye, target)),
                DrawingWrapper(new PlaneDrawing(viewproj, YPlaneMat, YColor, eye, target)),
                DrawingWrapper(new PlaneDrawing(viewproj, ZPlaneMat, ZColor, eye, target)),
                DrawingWrapper(new CenterDrawing(viewproj, center, centerColor, eye))
            };
        }
            break;
        case GizmoTool::Rotate:
        {
            float arcballRadius = orbitRadius;
            float SSOrbitRadius = arcballRadius + 1.0f * gizmoScale;
            
            bvg::Color SSOrbitColor = bvg::Color(0.75f, 0.75f, 0.75f);
            bvg::Color ArcballColor = bvg::Color(0.8f, 0.8f, 0.8f, 0.5f);
            SSOrbitColor.a *= opacity;
            ArcballColor.a *= opacity;
            
            float XRot = glm::pitch(state.rotation);
            float YRot = glm::yaw(state.rotation);
            float ZRot = glm::roll(state.rotation);
            
//            std::cout << "Yaw: " << XRot << " Pitch: " << YRot << " Roll: " << ZRot << std::endl;
            
            glm::mat4 XOrbitMat =
                glm::translate(state.translation) *
                rotationMat *
//                glm::toMat4(glm::quat(glm::vec3(XRot, 0.0f, ZRot))) *
                glm::scale(glm::vec3(orbitRadius));
            glm::mat4 YOrbitMat =
                glm::translate(state.translation) *
                rotationMat *
                glm::rotate((float)M_PI_2, glm::vec3(0.0f, 0.0f, 1.0f)) *
                glm::scale(glm::vec3(orbitRadius));
            glm::mat4 ZOrbitMat =
                glm::translate(state.translation) *
                rotationMat *
                glm::rotate((float)M_PI_2, glm::vec3(1.0f, 0.0f, 0.0f)) *
                glm::scale(glm::vec3(orbitRadius));
            
            drawings = {
                DrawingWrapper(new ScreenSpaceOrbitDrawing(viewproj, center,
                                                           SSOrbitRadius,
                                                           SSOrbitColor, eye)),
                DrawingWrapper(new ArcballDrawing(viewproj, center, arcballRadius,
                                                  ArcballColor, eye)),
                DrawingWrapper(new OrbitDrawing(viewproj, XOrbitMat, Axis::X, XColor, eye, target)),
                DrawingWrapper(new OrbitDrawing(viewproj, YOrbitMat, Axis::Y, YColor, eye, target)),
                DrawingWrapper(new OrbitDrawing(viewproj, ZOrbitMat, Axis::Z, ZColor, eye, target))
            };
        }
            break;
        case GizmoTool::Scale:
        {
            glm::vec3 XLineEnd = glm::vec3(1.0f, 0.0f, 0.0f) * arrowLength;
            glm::vec3 YLineEnd = glm::vec3(0.0f, 1.0f, 0.0f) * arrowLength;
            glm::vec3 ZLineEnd = glm::vec3(0.0f, 0.0f, 1.0f) * arrowLength;
            XLineEnd = state.translation + state.rotation * XLineEnd;
            YLineEnd = state.translation + state.rotation * YLineEnd;
            ZLineEnd = state.translation + state.rotation * ZLineEnd;
            
            drawings = {
                DrawingWrapper(new ArrowDrawing(viewproj, center, XLineEnd, XColor,
                                                Axis::X, eye, target, true)),
                DrawingWrapper(new ArrowDrawing(viewproj, center, YLineEnd, YColor,
                                                Axis::Y, eye, target, true)),
                DrawingWrapper(new ArrowDrawing(viewproj, center, ZLineEnd, ZColor,
                                                Axis::Z, eye, target, true)),
                DrawingWrapper(new UniformScaleDrawing(viewproj, center, centerColor, eye))
            };
        }
            break;
    }
            
    if(tool != GizmoTool::Rotate)
        std::sort(drawings.begin(), drawings.end(), std::greater<DrawingWrapper>());
    
    state.isMouseOverControl = false;
    // From nearest to farest
    for(int i = drawings.size() - 1; i >= 0; i--) {
        Drawing* drawing = drawings.at(i).drawing;
        if(drawing->mouseEvent(ctx, state, model, isMouseDown, mouseX, mouseY)) {
            state.isMouseOverControl = true;
            break;
        }
    }
    
    for(auto drawingw : drawings)
        drawingw.drawing->draw(ctx, state);
    
    for(auto drawingw : drawings)
        delete drawingw.drawing;
    drawings.clear();
    
    drawGizmosNew(ctx, state, tool, viewproj, model,
                  eye, target, up,
                  isMouseDown, mouseX, mouseY);
    
    state.lastMouseX = mouseX;
    state.lastMouseY = mouseY;
}

} // namespace gizmo
