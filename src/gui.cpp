#include "gui.hpp"
#include "trace.hpp"
#include "common.hpp"
#include "imgui_internal.h"
#include "platform.hpp"

struct TextUserData { char ** str; const char * filter; };
static int imgui_text_callback(ImGuiInputTextCallbackData * data) {
    TextUserData * user = (TextUserData *) data->UserData;

    if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
        return data->EventChar < 32 || data->EventChar > 126 || strchr(user->filter, data->EventChar) != nullptr;
    }

    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        *user->str = data->Buf = (char *) realloc(data->Buf, data->BufSize);
    }

    return 0;
}

namespace ImGui { //helpers
    int InputTextDyn(const char * label, char ** str, ImGuiInputTextFlags flags, const char * filter) {
        if (!*str) *str = dup("");
        TextUserData user = { str, filter };
        return InputText(label, *str, strlen(*str) + 1,
            flags | ImGuiInputTextFlags_CallbackResize, imgui_text_callback, &user);
    }

    int ButtonDyn(const char * label, bool enabled) {
        if (!enabled) {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        bool ret = ImGui::Button(label);
        if (!enabled) {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
        return ret;
    }

    int CheckboxDyn(const char * label, bool * v, bool mixed) {
        if (mixed) {
            //could also change ImGuiCol_Text
            ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(191, 63, 63, 127));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(191, 95, 95, 191));
        }
        int ret = ImGui::Checkbox(label, v);
        if (mixed) {
            ImGui::PopStyleColor(2);
        }
        return ret;
    }

    int CollapsingHeaderWithDefault(const char * label, bool * def, ImGuiTreeNodeFlags flags) {
        if (*def) flags |= ImGuiTreeNodeFlags_DefaultOpen;
        return *def = ImGui::CollapsingHeader(label, flags);
    }

    bool InputLevelName(const char * label, char ** str, char * old, const char * path, bool * exists) {
        //TODO: figure out how to keep the text box focused after pressing enter
        bool enter = ImGui::InputTextDyn(label, str,
            ImGuiInputTextFlags_CallbackCharFilter | ImGuiInputTextFlags_EnterReturnsTrue, "<>\"\\|:?*/");
        char * dest = dsprintf(nullptr, "%s/%s.txt", path, *str);
        *exists = file_exists(dest);
        ImGui::PushStyleColor(ImGuiCol_Text, *exists? 0xFF0000FF : 0xFF00FF00);
        if (old) ImGui::Text("%s -> %s%s", old, dest, *exists? " ALREADY EXISTS" : "");
        else     ImGui::Text(      "%s%s",      dest, *exists? " ALREADY EXISTS" : "");
        ImGui::PopStyleColor();
        free(dest);
        return enter;
    }

    bool ColorEdit3(const char * label, Color * c, ImGuiColorEditFlags flags) {
        Vec4 v = vec4(*c);
        bool ret = ImGui::ColorEdit3(label, &v.x, flags);
        if (ret) *c = color(v);
        return ret;
    }
}
