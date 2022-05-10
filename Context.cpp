#include <Context.h>
#include <iostream>

Context::Context(bvg::Context& vg):
    vg(vg)
{
}

void ElementState::onCreate() {
    
}

void ElementState::onDelete() {
    
}

void ElementState::applyChanges(ElementState& other) {
    
}

ElementComparison ElementState::compare(ElementState& other) {
    return ElementComparison::Different;
}

void changesScript(std::list<ElementState>& before, std::list<ElementState>& after) {
    /*
     Input:
         dst: Abc Bca Egf Bgg Ppt
         src: Abc Mvb Hjj Bca Bgf Ppt
     
     LCS:
        Abc Bca Bgg Ppt
     
     Output (SES):
         Common (Abc)
         Added (Mvb)
         Added (Hjj)
         Removed (Egf)
         Common (Bca)
         Edited (Bgg -> Bgf)
         Common (Ppt)
     
     Task:
        Longest common subsequence
        Shortest edit script
     */
    
    ElementComparison comp;
    auto beforeIt = before.begin();
    auto afterIt = after.begin();
    while(beforeIt != before.end()) {
        comp = beforeIt->compare(*afterIt);
        switch(comp) {
            case ElementComparison::Different:
            {
                for(auto afterIt2 = std::next(afterIt); afterIt2 != after.end(); afterIt2++) {
                    if(beforeIt->compare(*afterIt2) != ElementComparison::Different) {
                        afterIt = afterIt2;
                        // Add to list
                        break;
                    }
                }
            }
                break;
            case ElementComparison::Changed:
            case ElementComparison::Equal:
                beforeIt++;
                afterIt++;
                // Add to list
                break;
        }
    }
}

void beginPass(Context& ctx, ContextPass pass) {
    ctx.currentPass = pass;
    ctx.affineList.clear();
    ctx.boundaryList.clear();
    glm::mat3 matrix = glm::mat3(1.0f);
    Affine affine;
    affine.local = matrix;
    affine.world = matrix;
    ctx.affineList.push_back(affine);
    Boundary boundary;
    boundary.width = ctx.vg.width;
    boundary.height = ctx.vg.height;
    ctx.boundaryList.push_back(boundary);
}

void endPass(Context& ctx) {
    ctx.currentPass = ContextPass::UNSPECIFIED;
}

void beginAffine(Context& ctx, glm::mat3 matrix) {
    Affine affine;
    affine.local = matrix;
    affine.world = ctx.affineList.back().world;
    affine.world *= affine.local;
    ctx.affineList.push_back(affine);
}

void endAffine(Context& ctx) {
    ctx.affineList.pop_back();
}
