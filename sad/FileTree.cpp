#include "FileTree.h"
#include "imgui.h"
#include <filesystem>
#include <iostream>

FileTree::FileTree(std::filesystem::path root) {
    this->filepath = root;
    this->isFile = std::filesystem::is_regular_file(this->filepath);
    this->isOpen = true;

    if (!this->isFile) {
        for (const auto& file : std::filesystem::directory_iterator(root)) {
            this->children.push_back(FileTree(file.path()));
        }
    }
}

void FileTree::printToStdOut(int depth) {
    char icon = this->isFile ? 'F' : 'D';

    for (int i = 0;i < depth;i++) {
        std::cout << "    ";
    }
    std::string iden = this->filepath.filename().string();
    std::cout << icon << " " << iden << std::endl;

    if (this->isFile) return;

    if (this->isOpen) {
        for (auto& ch : this->children) {
            ch.printToStdOut(depth + 1);
        }
    }
}

std::tuple<bool, std::filesystem::path> FileTree::renderImGui() {
    std::string filename = this->filepath.filename().string();
    const char* label = filename.c_str();
    if (this->isFile) {
        if (ImGui::Selectable(label)) {
            return { true, this->filepath };
        }
        return { false, this->filepath };
    }

    bool clicked = false;
    std::filesystem::path selectedPath;

    if (ImGui::TreeNode(label)) {
        for (auto& ch : this->children) {
            if (!ch.isFile) {
                auto [chClicked, chPath] = ch.renderImGui();
                if (chClicked) {
                    clicked = true;
                    selectedPath = chPath;
                }
            }
        }
        for (auto& ch : this->children) {
            if (ch.isFile) {
                auto [chClicked, chPath] = ch.renderImGui();
                if (chClicked) {
                    clicked = true;
                    selectedPath = chPath;
                }
            }
        }

        ImGui::TreePop();

    }

    return { clicked, selectedPath };
}
