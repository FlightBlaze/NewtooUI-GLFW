//
//  Gizmos.hpp
//  MyProject
//
//  Created by Dmitry on 03.12.2021.
//

#pragma once

#include <blazevg.hh>
#include <glm/gtc/quaternion.hpp>

enum class Control {
    None,
    Center,
    SSOrbit
};

enum class GizmoTool {
    Translate,
    Rotate,
    Scale
};

struct GizmoState {
    Control selectedControl = Control::None;
    Control controlOverMouse = Control::None;
    bool isMouseOverControl = false;
    float lastMouseX = 0.0f;
    float lastMouseY = 0.0f;
    glm::vec3 viewRight;
    glm::vec3 viewUp;
    glm::vec3 offset;
    float z = 0.0f;
    float angle = 0.0f;
    glm::vec3 translation;
    glm::vec3 scale;
    glm::quat rotation;
};

void drawGizmos(bvg::Context& ctx, GizmoState& state, GizmoTool tool,
                glm::mat4 viewproj, glm::mat4& model,
                glm::vec3 eye, glm::vec3 target, glm::vec3 up,
                bool isMouseDown, float mouseX, float mouseY);
