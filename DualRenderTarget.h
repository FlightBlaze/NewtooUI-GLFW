#pragma once

#include <RenderTarget.h>
#include <memory>

class DualRenderTarget {
public:
	std::shared_ptr<RenderTarget> current = nullptr;
	std::shared_ptr<RenderTarget> previous = nullptr;

	void swap();
};
