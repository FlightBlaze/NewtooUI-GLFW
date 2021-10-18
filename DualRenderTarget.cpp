#include <DualRenderTarget.h>

void DualRenderTarget::swap() {
	std::shared_ptr<RenderTarget> newCurrent = previous;
	std::shared_ptr<RenderTarget> newPrevious = current;
	current = newCurrent;
	previous = newPrevious;
}
