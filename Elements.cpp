#include <Elements.h>

Spring2D::Spring2D(ui::SpringPhysicalProperties& props)
	: x(0.0f, 0.0f, props), y(0.0f, 0.0f, props)
{
}

Point2D Element::GetScreenPosition() {
	glm::vec2 pos = glm::vec2(this->position.x.currentValue, this->position.y.currentValue);
	pos = glm::vec2(glm::vec4(pos, 0.0f, 1.0f) * this->matrix);
	std::shared_ptr<Element> curParent = parent.lock();
	while (curParent != nullptr) {
		Spring2D& parentPos = curParent->position;
		glm::vec2 parentCurPos = glm::vec2(parentPos.x.currentValue, parentPos.y.currentValue);
		parentCurPos = glm::vec2(glm::vec4(parentCurPos, 0.0f, 1.0f) * curParent->matrix);
		pos += parentCurPos;
		curParent = curParent->parent.lock();
	}
	return { pos.x, pos.y };
}
