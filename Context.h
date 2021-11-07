#pragma once

#include <TextRenderer.h>
#include <Shape.h>
#include <memory>
#include <list>

class Context;

void beginBoundary(Context& ctx, std::shared_ptr<Shape> shape);
void endBoundary(Context& ctx);

void beginAffine(Context& ctx, glm::mat4 matrix);
void endAffine(Context& ctx);

enum class ContextPass {
    UNSPECIFIED,
    LAYOUT,
    DRAW
};

struct ClippingMask {
    std::shared_ptr<Shape> shape;
    glm::mat4 affine;
};

struct LayoutLine {
    float width, height;
};

struct Boundary {
    std::shared_ptr<Shape> shape;
    std::shared_ptr<LayoutLine> currentLine;
    glm::vec2 currentPosition;
};

struct Affine {
    glm::mat4 local;
    glm::mat4 world;
};

struct LayoutSpec {
    float width, height;
    glm::vec2 position = glm::vec2(0.0f);
    std::shared_ptr<Shape> boundaryShape;
    std::shared_ptr<LayoutLine> line;
};

class Context {
public:
    Context();
    
    ContextPass currentPass = ContextPass::UNSPECIFIED;
    
    std::shared_ptr<TextRenderer> textRenderer;
    std::shared_ptr<ShapeRenderer> shapeRenderer;
    
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> renderDevice;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> context;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> swapChain;
    
    std::list<ClippingMask> clippingMaskList;
    std::list<Boundary> boundariesList;
    std::list<Affine> affineList;
    std::list<LayoutSpec> layoutSpecList;
    
    void pushLayoutSpec(LayoutSpec ls);
    LayoutSpec popLayoutSpec();
};
