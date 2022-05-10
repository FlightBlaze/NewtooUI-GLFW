#pragma once

#include <Spring.hh>
#include <Font.hh>
#include <Context.h>
#include <string>

//struct Spring2D {
//	Spring2D(ui::SpringPhysicalProperties& props);
//	ui::Spring x, y;
//};
//
//struct Point2D {
//	float x, y;
//};
//
//struct Color {
//    Color(float r, float g, float b, float a);
//    float r, g, b, a;
//};
//
//namespace elements {
//
//void Text(Context& ctx, std::wstring text, std::shared_ptr<TextRenderer> textRenderer, float sizePx, Color color);
//void Word(Context& ctx, std::wstring word, std::shared_ptr<TextRenderer> textRenderer, float sizePx, Color color);
//void Whitespace(Context& ctx, std::shared_ptr<TextRenderer> textRenderer, float sizePx);
//
//}

namespace elements {

void Label(Context& ctx, std::wstring text);

};
