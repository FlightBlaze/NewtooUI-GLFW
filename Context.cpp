#include <Context.h>
#include <iostream>
#include <Tube.h>

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

float multipleRayIntersectionLeft(
                             std::shared_ptr<Shape> shape,
                             glm::vec2 rayOrigin,
                             float height,
                             bool shortest,
                             bool cullInside = true,
                             int numRays = 3,
                             Context* debugCtx = nullptr) {
    float bestDistance = NoIntersection;
    float step = height / numRays;
    for(float f = 0; f < height; f += step) {
        static const glm::vec2 leftDir = glm::vec2(1.0f, 0.0f);
        glm::vec2 origin = rayOrigin + glm::vec2(0.0f, f);
        if(cullInside) {
            if(shape->containsPoint(origin))
                continue;
        }
        float distance = shape->intersectsRay(origin, leftDir);
        if(distance != NoIntersection) {
            if(bestDistance == NoIntersection)
                bestDistance = distance;
            else {
                if(shortest)
                    bestDistance = fminf(bestDistance, distance);
                else
                    bestDistance = fmaxf(bestDistance, distance);
            }
            if(debugCtx != nullptr) {
//                glm::vec2 position = origin + shape->offset;
//                tube::Path path;
//                path.points = {
//                    tube::Point(glm::vec3(position, 0.0f)),
//                    tube::Point(glm::vec3(position + glm::vec2(distance, 0.0f), 0.0f))
//                };
//                tube::Builder strokeBuilder = tube::Builder(path)
//                    .withShape(tube::Shapes::circle(1.0f, 4));
//                Shape stroke = CreateStroke(debugCtx->renderDevice, strokeBuilder);
//                debugCtx->shapeRenderer->draw(debugCtx->context, debugCtx->affineList.back().world, stroke, glm::vec3(1.0f, 0.0f, 0.0f), 1.0f);
                debugCtx->NumRaycasts += shape->vertices.size();
            }
        }
    }
    return bestDistance;
}

void Context::newLineIfNeeded(Boundary& b, glm::vec2 position, glm::vec2 padding) {
    bool need = false;
    
    if(b.currentLine == nullptr)
        need = true;
    
    glm::vec2 relativeRayOrigin = position - b.shape->offset;
    float distance = multipleRayIntersectionLeft(b.shape, relativeRayOrigin, padding.y, true, false, 3, this);
    
    if(distance == NoIntersection) {
        position = glm::vec2(0.0f, position.y + padding.y);
        need = true;
    }
    else if(distance < padding.x) {
        need = true;
    }
    
    if(need) {
        newLine(b, position, padding);
    }
}

void Context::newLine(Boundary& b, glm::vec2 position, glm::vec2 padding) {
    auto shape = b.shape;
    float lineHeight = 0;
    if(b.currentLine != nullptr)
        lineHeight = b.currentLine->height;
    float distanceToNextLine = 0.0f;
    for(int i = 0; i < 2; i++) {
        glm::vec2 relativeRayOrigin = position - shape->offset;
        distanceToNextLine = multipleRayIntersectionLeft(shape, relativeRayOrigin, padding.y, false, true, 3, this);//, this);
        if(distanceToNextLine == NoIntersection) {
            // We are out of left border
            float add = lineHeight;
            if(add == 0)
                add = padding.y;
            else
                lineHeight = 0;
            position = glm::vec2(0.0f, position.y + add);
            continue;
        }
        glm::vec2 nextRelativeRayOrigin = relativeRayOrigin + glm::vec2(distanceToNextLine, 0.0f);
        float distanceToLineEnd = multipleRayIntersectionLeft(shape, nextRelativeRayOrigin, padding.y, true, false, 3, this);
        if(distanceToLineEnd < padding.x) {
            position += distanceToLineEnd;
            continue;
        }
    }
    b.currentLine = std::make_shared<LayoutLine>(LayoutLine());
    b.currentLine->position = position + glm::vec2(distanceToNextLine, 0.0f);
    b.currentPosition = b.currentLine->position;
}

void Context::doLayout(LayoutSpec& ls, Boundary& b) {
    newLineIfNeeded(b, b.currentPosition, glm::vec2(ls.width, ls.height));
    b.currentLine->width += ls.width;
    b.currentLine->height = fmaxf(b.currentLine->height, ls.height);
    ls.position = b.currentPosition;
    b.currentPosition.x += ls.width;
}
