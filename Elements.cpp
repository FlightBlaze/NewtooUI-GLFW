#include <Elements.h>
#include <glm/gtx/transform.hpp>

Color::Color(float r, float g, float b, float a):
    r(r), g(g), b(b), a(a)
{
}

// https://stackoverflow.com/a/5888676/5468048
std::vector<std::wstring> splitWString(const std::wstring &text, wchar_t delimiter)
{
    std::vector<std::wstring> words;
    size_t pos = text.find(delimiter);
    size_t initialPos = 0;

    while(pos != std::string::npos) {
        words.push_back(text.substr(initialPos, pos - initialPos));
        initialPos = pos + 1;
        pos = text.find(delimiter, initialPos);
    }
    
    words.push_back(text.substr(initialPos, std::min(pos, text.size()) - initialPos + 1));
    return words;
}

void elements::Text(Context& ctx, std::wstring text, std::shared_ptr<TextRenderer> textRenderer, float sizePx, Color color) {
    std::vector<std::wstring> words = splitWString(text, L' ');
    for(int i = 0; i < words.size(); i++) {
        elements::Word(ctx, words[i], textRenderer, sizePx, color);
        if(i != words.size() - 1)
            elements::Whitespace(ctx, textRenderer, sizePx);
    }
}

void elements::Word(Context& ctx, std::wstring word, std::shared_ptr<TextRenderer> textRenderer, float sizePx, Color color) {
    switch(ctx.currentPass) {
        case ContextPass::LAYOUT:
        {
            LayoutSpec spec;
            spec.width = textRenderer->measureWidth(word, sizePx);
            spec.height = textRenderer->measureHeight(sizePx);
            ctx.pushLayoutSpec(spec);
        }
            break;
        case ContextPass::DRAW:
        {
            LayoutSpec spec = ctx.popLayoutSpec();
            textRenderer->draw(
               ctx.context,
               ctx.affineList.back().world *
                   glm::translate(glm::mat4(1.0f), glm::vec3(spec.position, 0.0f)),
               word,
               sizePx,
               glm::vec3(color.r, color.g, color.b),
               color.a);
//             std::wcout << word << L" pos: (" << spec.position.x << L", " << spec.position.y << L"), sizePx: " << sizePx << std::endl;
        }
            break;
        default:
            break;
    }
}

void elements::Whitespace(Context& ctx, std::shared_ptr<TextRenderer> textRenderer, float sizePx) {
    switch(ctx.currentPass) {
        case ContextPass::LAYOUT:
        {
            LayoutSpec spec;
            spec.width = textRenderer->font->characters[L' '].xAdvance *
                textRenderer->getScale(sizePx);
            spec.height = textRenderer->measureHeight(sizePx);
            ctx.pushLayoutSpec(spec);
        }
            break;
        case ContextPass::DRAW:
        {
            LayoutSpec spec = ctx.popLayoutSpec();
        }
            break;
        default:
            break;
    }
}

