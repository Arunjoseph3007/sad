#include "KeyBindings.h"

KeyBinding::KeyBinding(ImGuiKey k, int cond, Shortcut s) {
    this->key = k;
    this->condition = cond;
    this->func = s;
}

bool KeyBinding::test() const {
    // TODO: maybe we should also have a repeat in keybinding just to make it configurable
    if (!ImGui::IsKeyPressed(this->key)) return false;

    bool ctrlDown = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
    bool shiftDown = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
    bool altDown = ImGui::IsKeyDown(ImGuiKey_LeftAlt) || ImGui::IsKeyDown(ImGuiKey_RightAlt);

    int state = (Ctrl * ctrlDown) | (Shift * shiftDown) | (Alt * altDown);

    return state == this->condition;
}
