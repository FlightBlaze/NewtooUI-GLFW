//
//  Gizmos.cpp
//  MyProject
//
//  Created by Dmitry on 03.12.2021.
//

#include <Gizmos.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
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
    ctx.arc(centerS.x, centerS.y, 16.0f, 0.0f, M_PI * 2.0f + 0.2f);
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
    ctx.fill();
    ctx.stroke();
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

bool CenterDrawing::mouseEvent(bvg::Context& ctx, GizmoState& state, glm::mat4& model,
                               bool isMouseDown, float mouseX, float mouseY) {
    if(state.selectedControl == Control::Center) {
        float relX = mouseX - state.lastMouseX;
        float relY = mouseY - state.lastMouseY;
        glm::vec3 move = state.viewUp * -relY + state.viewRight * -relX;
        move *= 0.088f;
        model = glm::translate(move) * model;
    }
    
    if(isMouseOverGizmoCenter(ctx, viewproj, center, glm::vec2(mouseX, mouseY))) {
        if(!isMouseDown) {
            state.controlOverMouse = Control::Center;
            return true;
        }
        if(state.controlOverMouse == Control::Center) {
            state.selectedControl = Control::Center;
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

void drawGizmos(bvg::Context& ctx,  GizmoState& state,
                glm::mat4 viewproj, glm::mat4& model,
                glm::vec3 eye, glm::vec3 target, glm::vec3 up,
                bool isMouseDown, float mouseX, float mouseY)
{
    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(model, scale, rotation, translation, skew, perspective);
    
    float arrowLength = 5.0f;
    float planeSize = 1.0f;
    float planeDistance = 3.0f;
    
    float eyeDistance = glm::distance(translation, eye);
    float minDistance = 7.5f;
    float visibleDistance = 10.0f;
    float opacity = remapf(minDistance, visibleDistance, 0.0f, 1.0f, eyeDistance);
    opacity = clampf(0.0f, opacity, 1.0f);
    
    // If gizmo is beind the camera
    if(worldToScreenSpace(ctx, translation, viewproj).z > 1.0f ||
       eyeDistance < 7.5f) {
        return;
    }
    
    glm::vec3 center = glm::vec3(0.0f);
    glm::vec3 XArrowEnd = glm::vec3(1.0f, 0.0f, 0.0f) * arrowLength;
    glm::vec3 YArrowEnd = glm::vec3(0.0f, 1.0f, 0.0f) * arrowLength;
    glm::vec3 ZArrowEnd = glm::vec3(0.0f, 0.0f, 1.0f) * arrowLength;
    center = translation + center;
    XArrowEnd = translation + XArrowEnd;
    YArrowEnd = translation + YArrowEnd;
    ZArrowEnd = translation + ZArrowEnd;
    bvg::Color XColor = bvg::Color(1.0f, 0.2f, 0.2f);
    bvg::Color YColor = bvg::Color(0.2f, 1.0f, 0.2f);
    bvg::Color ZColor = bvg::Color(0.2f, 0.2f, 1.0f);
    bvg::Color centerColor = bvg::Color(1.0f, 0.6f, 0.2f);
    XColor.a *= opacity;
    YColor.a *= opacity;
    ZColor.a *= opacity;
    centerColor.a *= opacity;
    glm::mat4 XPlaneMat =
        glm::translate(translation) *
        glm::translate(glm::vec3(0.0f, 1.0f, 1.0f) * planeDistance) *
        glm::rotate((float)M_PI_2, glm::vec3(0.0f, 0.0f, 1.0f)) *
        glm::scale(glm::vec3(planeSize));
    glm::mat4 YPlaneMat =
        glm::translate(translation) *
        glm::translate(glm::vec3(1.0f, 0.0f, 1.0f) * planeDistance) *
        glm::scale(glm::vec3(planeSize));
    glm::mat4 ZPlaneMat =
        glm::translate(translation) *
        glm::translate(glm::vec3(1.0f, 1.0f, 0.0f) * planeDistance) *
        glm::rotate((float)M_PI_2, glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::scale(glm::vec3(planeSize));
    
    if(!isMouseDown) {
        state.controlOverMouse = Control::None;
        state.selectedControl = Control::None;
    }
    
    glm::vec3 viewForward = glm::normalize(target - eye);
    state.viewRight = glm::normalize(glm::cross(up, viewForward));
    state.viewUp = glm::cross(viewForward, state.viewRight);
    
    std::vector<DrawingWrapper> drawings {
        DrawingWrapper(new ArrowDrawing(viewproj, center, XArrowEnd, XColor, eye, target)),
        DrawingWrapper(new ArrowDrawing(viewproj, center, YArrowEnd, YColor, eye, target)),
        DrawingWrapper(new ArrowDrawing(viewproj, center, ZArrowEnd, ZColor, eye, target)),
        DrawingWrapper(new PlaneDrawing(viewproj, XPlaneMat, XColor, eye, target)),
        DrawingWrapper(new PlaneDrawing(viewproj, YPlaneMat, YColor, eye, target)),
        DrawingWrapper(new PlaneDrawing(viewproj, ZPlaneMat, ZColor, eye, target)),
        DrawingWrapper(new CenterDrawing(viewproj, center, centerColor, eye))
    };
    
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
