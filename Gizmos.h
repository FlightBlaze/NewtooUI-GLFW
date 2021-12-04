//
//  Gizmos.hpp
//  MyProject
//
//  Created by Dmitry on 03.12.2021.
//

#pragma once

#include <blazevg.hh>

enum class Control {
    None,
    Center
};

struct GizmoState {
    Control selectedControl = Control::None;
    Control controlOverMouse = Control::None;
    bool isMouseOverControl = false;
    float lastMouseX = 0.0f;
    float lastMouseY = 0.0f;
    glm::vec3 viewRight = glm::vec3(0.0f);
    glm::vec3 viewUp = glm::vec3(0.0f);
};

void drawGizmos(bvg::Context& ctx, GizmoState& state,
                glm::mat4 viewproj, glm::mat4& model,
                glm::vec3 eye, glm::vec3 target, glm::vec3 up,
                bool isMouseDown, float mouseX, float mouseY);
