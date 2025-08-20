#pragma once
#include <vector>
#include <string>
#include <filesystem>

class FileTree {
public:
    std::filesystem::path filepath;
    bool isOpen;
    bool isFile;
    std::vector<FileTree> children;

    FileTree(std::filesystem::path root);
    void printToStdOut(int depth = 0);
    std::tuple<bool, std::filesystem::path> renderImGui();
};
