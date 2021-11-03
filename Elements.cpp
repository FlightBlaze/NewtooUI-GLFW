#include <Elements.h>

Element::Element():
    physicalProperties(10, 5, 2),
    position(physicalProperties),
    matrix(1.0f)
{
    
}

Spring2D::Spring2D(ui::SpringPhysicalProperties& props)
	: x(0.0f, 0.0f, props), y(0.0f, 0.0f, props)
{
}

glm::vec4 toVec4(glm::vec2 v) {
    return glm::vec4(v, 0.0f, 1.0f);
}

Point2D Element::getScreenPosition() {
	glm::vec2 pos = glm::vec2(this->position.x.currentValue, this->position.y.currentValue);
	pos = glm::vec2(toVec4(pos) * this->matrix);
	std::shared_ptr<Element> curParent = parent.lock();
	while (curParent != nullptr) {
		Spring2D& parentPos = curParent->position;
		glm::vec2 parentCurPos = glm::vec2(parentPos.x.currentValue, parentPos.y.currentValue);
		parentCurPos = glm::vec2(toVec4(parentCurPos) * curParent->matrix);
		pos += parentCurPos;
		curParent = curParent->parent.lock();
	}
	return { pos.x, pos.y };
}
