#pragma once
#include "web_compat.hpp"
class utils {
  IBML* bml_;
public:
  explicit utils(IBML* bml): bml_(bml) {}
  int get_bgui_font_size(float size) {
    // BGui uses the engine viewport. Browser CSS reference DPI is 96; canvas
    // backing DPR is already handled by the renderer and must not be divided twice.
    return std::max(1, int(std::round(bml_->GetRenderContext()->GetHeight() / (768.0f / 119) * size / 96.0f)));
  }
};
