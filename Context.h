#pragma once

#include <blazevg.hh>
#include <vector>
#include <memory>
#include <list>

class Affine {
public:
    glm::mat3 local;
    glm::mat3 world;
};

class Boundary {
public:
    float width, height = 0;
    bool clip = false;
};

enum class ContextPass {
    UNSPECIFIED,
    LAYOUT,
    DRAW
};

enum class ElementComparison {
    Equal,
    Changed,
    Different
};

class Rect {
    float x, y, width, height;
};

class Layout {
    glm::vec2 cursor;
    std::vector<Rect> lines;
};

class ElementState {
public:
    std::string type;
    Boundary parentBounds;
    std::shared_ptr<Layout> layout;
    Rect rect;
    
    virtual void onCreate();
    virtual void onDelete();
    virtual void applyChanges(ElementState& other);
    virtual ElementComparison compare(ElementState& other);
};

void changesScript(std::list<ElementState>& before, std::list<ElementState>& after);

// 1. Measure dimension constraints from parent to children
// 2. Measure dimensions from child to parent
// 3. Move elements to their position in the flow
// 4. Align elements

class Context {
public:
    Context(bvg::Context& vg);
    
    bvg::Context& vg;
    
    std::list<Affine> affineList;
    std::list<Boundary> boundaryList;
    
    std::list<ElementState> savedElements;
    std::list<ElementState> currentElements;
    
    ContextPass currentPass;
};

void beginPass(Context& ctx, ContextPass pass);
void endPass(Context& ctx);

void beginAffine(Context& ctx, glm::mat3 matrix);
void endAffine(Context& ctx);

void beginBoundary(Context& ctx, float left, float top, float right, float bottom);
void endBoundary(Context& ctx);
