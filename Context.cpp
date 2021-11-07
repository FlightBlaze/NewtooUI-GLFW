#include <Context.h>
#include <iostream>

void beginBoundary(Context& ctx, std::shared_ptr<Shape> shape) {
    Boundary b;
    b.shape = shape;
    b.currentPosition = glm::vec3(0.0f);
    b.currentLine = nullptr;
    ctx.boundariesList.push_back(b);
}

void endBoundary(Context& ctx) {
    ctx.boundariesList.pop_back();
}

void beginAffine(Context& ctx, glm::mat4 matrix) {
    Affine a;
    a.local = matrix;
    if(ctx.affineList.size() != 0)
        a.world = ctx.affineList.back().world * a.local;
    else
        a.world = a.local;
    ctx.affineList.push_back(a);
}

void endAffine(Context& ctx) {
    ctx.affineList.pop_back();
}

Context::Context() {
    
}

void doLayout(LayoutSpec& ls, Boundary& b) {
    if(b.currentLine == nullptr) {
        b.currentLine = std::make_shared<LayoutLine>(LayoutLine());
    }
    b.currentLine->width += ls.width;
    b.currentLine->height = fmaxf(b.currentLine->height, ls.height);
    ls.position = b.currentPosition;
    b.currentPosition.x += ls.width;
}

void Context::pushLayoutSpec(LayoutSpec ls)
{
    doLayout(ls, this->boundariesList.back());
    ls.boundaryShape = this->boundariesList.back().shape;
    ls.line = this->boundariesList.back().currentLine;
    this->layoutSpecList.push_back(ls);
}

LayoutSpec Context::popLayoutSpec()
{
    LayoutSpec ls = this->layoutSpecList.front();
    this->layoutSpecList.pop_front();
    return ls;
}
