//
//  Gizmos.cpp
//  MyProject
//
//  Created by Dmitry on 03.12.2021.
//

#include <Gizmos.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <MathExtras.h>

void drawArrow(bvg::Context& ctx, float sx, float sy, float ex, float ey, float size) {
    ctx.beginPath();
    ctx.moveTo(sx, sy);
    ctx.lineTo(ex, ey);
    ctx.stroke();
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
                    bvg::Color color) {
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
    
    drawArrow(ctx, originS.x, originS.y, endS.x, endS.y, 16);
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

void drawGizmoOrbit(bvg::Context& ctx, glm::mat4& viewproj,
                    glm::mat4& model, bvg::Color color, glm::vec3 viewDir) {
    
    std::vector<glm::vec3> vertices = createCircle(99);
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
        
        glm::vec3 dir = glm::normalize(ls.start - ls.end);
        glm::vec3 perpDir = glm::cross(dir, up);
        float codir = glm::dot(perpDir, viewDir);
        if(codir < 0.0f)
            ls.declined = true;
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
    
    ctx.lineWidth = 5.0f;
    ctx.strokeStyle = bvg::SolidColor(color);
    
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
    ctx.stroke();
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
    drawGizmoPlane(ctx, viewproj, model, color);
}

class ArrowDrawing : public Drawing {
public:
    ArrowDrawing(glm::mat4& viewproj,
                 glm::vec3 origin, glm::vec3 end,
                 bvg::Color color, glm::vec3 eye,
                 glm::vec3 target);
    
    glm::mat4& viewproj;
    glm::vec3 origin;
    glm::vec3 end;
    bvg::Color color;
    
    void draw(bvg::Context& ctx, GizmoState& state);
};

ArrowDrawing::ArrowDrawing(glm::mat4& viewproj,
                           glm::vec3 origin, glm::vec3 end,
                           bvg::Color color, glm::vec3 eye,
                           glm::vec3 target):
viewproj(viewproj),
origin(origin),
end(end),
color(color)
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
    drawGizmoArrow(ctx, viewproj, origin, end, color);
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
                 bvg::Color color, glm::vec3 eye,
                 glm::vec3 target);
    
    glm::mat4& viewproj;
    glm::mat4& model;
    bvg::Color color;
    glm::vec3 viewDir;
    
    void draw(bvg::Context& ctx, GizmoState& state);
};

OrbitDrawing::OrbitDrawing(glm::mat4& viewproj, glm::mat4& model,
                           bvg::Color color, glm::vec3 eye,
                           glm::vec3 target):
viewproj(viewproj),
model(model),
color(color)
{
    glm::vec3 center = model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    viewDir = glm::normalize(center - eye);
    this->distanceToEye = glm::distance(center, eye);
}

void OrbitDrawing::draw(bvg::Context& ctx, GizmoState& state) {
    drawGizmoOrbit(ctx, viewproj, model, color, viewDir);
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
    
    glm::vec3 viewForward = glm::normalize(target - eye);
    state.viewRight = glm::normalize(glm::cross(up, viewForward));
    state.viewUp = glm::cross(viewForward, state.viewRight);
    
    std::vector<DrawingWrapper> drawings;
    
    glm::vec3 center = glm::vec3(0.0f);
    center = state.translation + center;
    
    bvg::Color XColor = bvg::Color(1.0f, 0.2f, 0.2f);
    bvg::Color YColor = bvg::Color(0.2f, 1.0f, 0.2f);
    bvg::Color ZColor = bvg::Color(0.2f, 0.2f, 1.0f);
    bvg::Color centerColor = bvg::Color(1.0f, 0.6f, 0.2f);
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
                DrawingWrapper(new ArrowDrawing(viewproj, center, XArrowEnd, XColor, eye, target)),
                DrawingWrapper(new ArrowDrawing(viewproj, center, YArrowEnd, YColor, eye, target)),
                DrawingWrapper(new ArrowDrawing(viewproj, center, ZArrowEnd, ZColor, eye, target)),
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
            
            glm::mat4 XOrbitMat =
                glm::translate(state.translation) *
                rotationMat *
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
                DrawingWrapper(new OrbitDrawing(viewproj, XOrbitMat, XColor, eye, target)),
                DrawingWrapper(new OrbitDrawing(viewproj, YOrbitMat, YColor, eye, target)),
                DrawingWrapper(new OrbitDrawing(viewproj, ZOrbitMat, ZColor, eye, target))
            };
        }
            break;
        case GizmoTool::Scale:
        {
            glm::vec3 XLineEnd = glm::vec3(1.0f, 0.0f, 0.0f) * arrowLength;
            glm::vec3 YLineEnd = glm::vec3(0.0f, 1.0f, 0.0f) * arrowLength;
            glm::vec3 ZLineEnd = glm::vec3(0.0f, 0.0f, 1.0f) * arrowLength;
            XLineEnd = state.translation + XLineEnd;
            YLineEnd = state.translation + YLineEnd;
            ZLineEnd = state.translation + ZLineEnd;
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
    
    state.lastMouseX = mouseX;
    state.lastMouseY = mouseY;
}
