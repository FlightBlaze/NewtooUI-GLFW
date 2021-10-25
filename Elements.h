#pragma once

#include <Spring.hh>
#include <Font.hh>
#include <Shape.h>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

struct Spring2D {
	Spring2D(ui::SpringPhysicalProperties& props);
	ui::Spring x, y;
};

struct Point2D {
	float x, y;
};

class Element {
public:
	std::weak_ptr<Element> parent;
	std::vector<std::shared_ptr<Element>> children;

	ui::SpringPhysicalProperties physicalProperties;
	Spring2D position;
	Spring2D size;

	glm::mat4 matrix;

	Point2D GetScreenPosition();
};
