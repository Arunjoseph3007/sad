#pragma once
#include "imgui.h"
#include "Editor.h"
#include <vector>
#include <GLFW/glfw3.h>

#define POW2(x) (1 << x)

enum AccentKey {
    Ctrl = POW2(1),
    Shift = POW2(2),
    Alt = POW2(3)
};

typedef bool (*Shortcut)(GLFWwindow*, Editor&);

class KeyBinding {
public:
    ImGuiKey key;
    int condition;
    Shortcut func;

    KeyBinding(ImGuiKey k, int cond, Shortcut s);

    bool test() const;
};

typedef  std::vector<KeyBinding> KeyBindings;
