#ifndef GUI_HPP
#define GUI_HPP

#include "imgui.h"
#include "color.hpp"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IMGUI HELPERS                                                                                                    ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace ImGui { //helpers
    int InputTextDyn(const char * label, char ** str, ImGuiInputTextFlags flags = 0, const char * filter = "");
    int ButtonDyn(const char * label, bool enabled);
    int CheckboxDyn(const char * label, bool * v, bool mixed);
    int CollapsingHeaderWithDefault(const char * label, bool * def, ImGuiTreeNodeFlags flags = 0);
    bool InputLevelName(const char * label, char ** str, char * old, const char * path, bool * exists);
    bool ColorEdit3(const char * label, Color * c, ImGuiColorEditFlags flags = 0);
}

#endif //GUI_HPP
