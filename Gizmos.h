//
//  Gizmos.hpp
//  MyProject
//
//  Created by Dmitry on 03.12.2021.
//

#pragma once

#include <blazevg.hh>
#include <glm/gtc/quaternion.hpp>

namespace gizmo {

enum class Control {
    None,
    Center,
    SSOrbit,
    Orbit,
    UniformScale,
    Arrow,
    Arcball
};

enum class GizmoTool {
    Translate,
    Rotate,
    Scale
};

enum class Axis {
    X,
    Y,
    Z
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

struct GizmoState {
    Control selectedControl = Control::None;
    Control controlOverMouse = Control::None;
    bool isMouseOverControl = false;
    float lastMouseX = 0.0f;
    float lastMouseY = 0.0f;
    glm::vec3 offset;
    float z = 0.0f;
    float rotatedAngle = 0.0f;
    float startAngle = 0.0f;
    float angle = 0.0f;
    glm::vec3 startPiePoint;
    glm::vec3 piePoint;
    glm::vec3 translation;
    glm::vec3 scale;
    glm::quat rotation;
    glm::vec2 mouse;
    Axis axis;
    bool isRotationSnapping = false;
    glm::vec3 viewRight;
    glm::vec3 viewUp;
};

struct GizmoProperties {
    bool enabledRotationSnap = false;
};

void drawGizmos(bvg::Context& ctx, GizmoState& state, GizmoTool tool,
                GizmoProperties& props,
                glm::mat4 viewproj, glm::mat4& model,
                glm::vec3 eye, glm::vec3 target, glm::vec3 up,
                bool isMouseDown, float mouseX, float mouseY);

} // namespace gizmo
