#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include <GLFW/glfw3.h>

#include "FileTree.h"
#include "Editor.h"
#include "KeyBindings.h"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <chrono>
#include <unordered_map>

static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Keybindings
static bool pasteTextFromClipBoard(GLFWwindow* window, Editor& e) {
	if (e.cursor.isSelection()) {
		e.emptySelection();
	}
	const char* cData = glfwGetClipboardString(window);
	e.insertBefore(std::string(cData));
	return true;
}
static bool copyTextToClipBoard(GLFWwindow* window, Editor& e) {
	if (e.cursor.isSelection()) {
		glfwSetClipboardString(window, e.getSelectionString().c_str());
	}
	else {
		// TODO try to better copy vscode style copying
		glfwSetClipboardString(window, e.buffer[e.getCursorStart().y].c_str());
	}
	return true;
}
static bool cutTextToClipBoard(GLFWwindow* window, Editor& e) {
	if (e.cursor.isSelection()) {
		glfwSetClipboardString(window, e.getSelectionString().c_str());
		e.emptySelection();
	}
	else {
		// TODO try to better copy vscode style cutting
		int lineNo = e.getCursorStart().y;
		glfwSetClipboardString(window, e.buffer[lineNo].c_str());
		e.buffer.erase(e.buffer.begin() + lineNo);
	}
	return true;
}
static bool selectAll(GLFWwindow* window, Editor& e) {
	e.cursor.start.x = 0;
	e.cursor.start.y = 0;

	e.cursor.end.y = e.buffer.size() - 1;
	e.cursor.end.x = e.buffer[e.buffer.size() - 1].size();
	return true;
}

static bool moveLeftWord(GLFWwindow* window, Editor& e) {
	e.leftWord();
	return true;
}
static bool moveRightWord(GLFWwindow* window, Editor& e) {
	e.rightWord();
	return true;
}

static bool moveLineUp(GLFWwindow* window, Editor& e) {
	int y = e.cursor.end.y;
	if (y > 0) {
		std::string t = e.buffer[y - 1];
		e.buffer[y - 1] = e.buffer[y];
		e.buffer[y] = t;
		e.up();
	}
	return true;
}
static bool moveLineDown(GLFWwindow* window, Editor& e) {
	int y = e.cursor.end.y;
	if (y < e.buffer.size() - 1) {
		std::string t = e.buffer[y + 1];
		e.buffer[y + 1] = e.buffer[y];
		e.buffer[y] = t;
		e.down();
	}
	return true;
}
static bool copyLineDown(GLFWwindow* window, Editor& e) {
	e.buffer.insert(e.buffer.begin() + e.cursor.end.y, e.buffer[e.cursor.end.y]);
	e.down();
	return true;
}
static bool copyLineUp(GLFWwindow* window, Editor& e) {
	e.buffer.insert(e.buffer.begin() + e.cursor.end.y, e.buffer[e.cursor.end.y]);
	return true;
}
static bool undo(GLFWwindow* window, Editor& e) {
	e.undo();
	return true;
}
static bool redo(GLFWwindow* window, Editor& e) {
	e.redo();
	return true;
}

// Main code
int main(int, char**) {
	std::string path = "C:\\Users\\arun.mulakkal\\xlabs\\ramdon";
	std::filesystem::path filepath = std::filesystem::path(path);
	std::cout << "Filepath is: " << filepath.string() << std::endl;

	FileTree ft = FileTree(filepath);

	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit()) return 1;

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Sad", nullptr, nullptr);
	if (window == nullptr) return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL2_Init();

	const char* fontLocation = "C:\\Users\\arun.mulakkal\\AppData\\Local\\Microsoft\\Windows\\Fonts\\FiraCode-Regular.ttf";
	ImFont* appFont = io.Fonts->AddFontFromFileTTF(fontLocation);
	IM_ASSERT(appFont != nullptr);
	ImGui::PushFont(appFont);

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	Editor editor = Editor();
	// TODO load this from config file
	KeyBindings bindings = {
		KeyBinding(ImGuiKey_V, Ctrl, pasteTextFromClipBoard),
		KeyBinding(ImGuiKey_C, Ctrl, copyTextToClipBoard),
		KeyBinding(ImGuiKey_X, Ctrl, cutTextToClipBoard),
		KeyBinding(ImGuiKey_LeftArrow, Ctrl, moveLeftWord),
		KeyBinding(ImGuiKey_RightArrow, Ctrl, moveRightWord),
		KeyBinding(ImGuiKey_UpArrow, Alt, moveLineUp),
		KeyBinding(ImGuiKey_DownArrow, Alt, moveLineDown),
		KeyBinding(ImGuiKey_UpArrow, Alt + Shift, copyLineUp),
		KeyBinding(ImGuiKey_DownArrow, Alt + Shift, copyLineDown),
		KeyBinding(ImGuiKey_A, Ctrl, selectAll),
		KeyBinding(ImGuiKey_Z, Ctrl, undo),
		KeyBinding(ImGuiKey_Y, Ctrl, redo),
	};

	// Main loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
			ImGui_ImplGlfw_Sleep(10);
			continue;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::DockSpaceOverViewport(1);
		// File tree
		{
			ImGui::Begin("FileTree");                          // Create a window called "Hello, world!" and append into it.

			auto [clicked, selectedPath] = ft.renderImGui();
			if (clicked) {
				std::string filename = selectedPath.string();
				std::cout << "Opened: " << filename << std::endl;

				std::fstream inputFile(filename.c_str());

				editor.buffer.clear();
				std::string line;
				while (std::getline(inputFile, line)) { // Read line by line
					editor.buffer.push_back(line);
				}

				std::cout << editor.buffer.size() << std::endl;
			}

			ImGui::End();
		}

		// Editor
		{
			ImGui::Begin("Editor");

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 p = ImGui::GetCursorScreenPos();
			ImVec2 fSize(2, 20);

			// Ghost Mouse End
			{
				IVec2 pos = editor.getGhostEnd();
				ImVec2 start(p.x + pos.x * 10, p.y + pos.y * 24);
				ImVec2 end(start.x + fSize.x, start.y + fSize.y);
				drawList->AddRectFilled(start, end, ImColor(255, 255, 255));
			}

			auto markSelectionLine = [&drawList, &p](int y, int sx, int ex) {
				const int lineHeight = 24;
				const int charWidth = 10;
				ImVec2 lstart(p.x + sx * charWidth, p.y + y * lineHeight);
				ImVec2 lend(p.x + ex * charWidth, lstart.y + lineHeight);
				ImColor selCol = ImColor(1.0f, 1.0f, 1.0f, 0.3f);

				drawList->AddRectFilled(lstart, lend, selCol);
				};

			if (editor.cursor.isSelection()) {
				IVec2 st = editor.cursor.selectionStart(editor.buffer);
				IVec2 ed = editor.cursor.selectionEnd(editor.buffer);

				int xmin = st.x, xmax = ed.x;
				int ymin = st.y, ymax = ed.y;

				if (ymin == ymax) {
					markSelectionLine(ymax, xmin, xmax);
				}
				else {
					markSelectionLine(ymin, xmin, editor.buffer[ymin].size());
					for (int i = ymin + 1;i < ymax;i++) markSelectionLine(i, 0, editor.buffer[i].size());
					markSelectionLine(ymax, 0, xmax);
				}
			}



			for (int i = 0;i < editor.buffer.size();i++) {
				char lineNumber[10];
				sprintf_s(lineNumber, "%d", i + 1);

				ImGui::TextUnformatted(editor.buffer[i].c_str());
			}

			// Event handling
			if (ImGui::IsWindowFocused()) {
				/*
				* Handle key bindings
				* bindings must bind to a function that takes editor as and return true or false
				* if bound function return true, further keypressed events wont be processed
				* if returned false, further events will be
				*/
				bool handled = false;
				for (const KeyBinding& kb : bindings) {
					if (kb.test()) {
						handled = kb.func(window, editor);
						if (handled) break;
					}
				}
				bool shiftDown = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);

				if (handled) {}
				else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
					if (shiftDown) {
						std::cout << "SelectLeft\n";
						editor.selectLeft();
					}
					else {
						std::cout << "Left\n";
						editor.left();
					}
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
					if (shiftDown) {
						std::cout << "SelectRight\n";
						editor.selectRight();
					}
					else {
						std::cout << "Right\n";
						editor.right();
					}
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
					if (shiftDown) {
						std::cout << "SelectUp\n";
						editor.selectUp();
					}
					else {
						std::cout << "Up\n";
						editor.up();
					}
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
					if (shiftDown) {
						std::cout << "SelectDown\n";
						editor.selectDown();
					}
					else {
						std::cout << "Down\n";
						editor.down();
					}
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
					std::cout << "Enter\n";
					editor.enter();
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
					std::cout << "Home\n";
					editor.home();
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_End)) {
					std::cout << "End\n";
					editor.end();
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
					std::cout << "Space\n";
					editor.insertBefore(' ');
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
					std::cout << "Backspace\n";
					editor.backspace();
				}
				else if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
					std::cout << "Delete\n";editor.eDelete();
				}
				// text input
				else {
					// number
					if (!handled) {
						for (int i = ImGuiKey::ImGuiKey_0;i <= ImGuiKey::ImGuiKey_9 && !handled;i++) {
							if (ImGui::IsKeyPressed((ImGuiKey)i)) {
								handled = true;
								bool shift = ImGui::IsKeyDown(ImGuiKey::ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey::ImGuiKey_RightShift);
								if (shift) {
									editor.charInsertBefore(i, true);
								}
								else {
									char c = i - ImGuiKey_0 + 48;
									editor.insertBefore(c);
								}
							}
						}
					}

					// alphabets
					if (!handled) {
						for (int i = ImGuiKey::ImGuiKey_A;i <= ImGuiKey::ImGuiKey_Z && !handled;i++) {
							if (ImGui::IsKeyPressed((ImGuiKey)i)) {
								handled = true;
								bool shift = ImGui::IsKeyDown(ImGuiKey::ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey::ImGuiKey_RightShift);
								char c = i - ImGuiKey_A + (shift ? 65 : 97);
								editor.insertBefore(c);
							}
						}
					}
					// characters
					if (!handled) {
						for (int i = ImGuiKey::ImGuiKey_Apostrophe;i <= ImGuiKey::ImGuiKey_GraveAccent && !handled;i++) {
							if (ImGui::IsKeyPressed((ImGuiKey)i)) {
								handled = true;
								bool shift = ImGui::IsKeyDown(ImGuiKey::ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey::ImGuiKey_RightShift);
								editor.charInsertBefore(i, shift);
							}
						}
					}
				}
			}

			ImGui::End();
		}

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		glfwMakeContextCurrent(window);
		glfwSwapBuffers(window);
	}

	// Cleanup
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
