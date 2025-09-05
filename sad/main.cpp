#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include <GLFW/glfw3.h>

#include "FileTree.h"
#include "Editor.h"
#include "KeyBindings.h"
#include "Grammar.h"
#include "Timer.h"

#include <stdio.h>
#include <regex>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <chrono>
#include <unordered_map>

// uncomment this to remove title bar
//#define NO_TITLE_BAR

static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

typedef std::unordered_map<std::string, ImColor> SyntaxHighlightTheme;

SyntaxHighlightTheme SyntaxTheme = {
	{ "control",		ImColor(72, 163, 219) },
	{ "declaration",	ImColor(159, 81, 176) },
	{ "context",		ImColor(54, 89, 217) },
	{ "literal",		ImColor(45, 142, 207) },
	{ "string",			ImColor(84, 150, 75) },
	{ "numeric",		ImColor(160, 171, 60) },
	{ "operator",		ImColor(255, 255, 255) },
	{ "commnets",		ImColor(160, 161, 157) },
	{ "variable",		ImColor(116, 182, 232) },
	{ "punctuation",	ImColor(255, 255, 255) },
};

ImColor getTokenColor(int curTokIdx, const std::vector<GrammarMatch>& tokens, SyntaxHighlightTheme& theme) {
	if (tokens.size() == 0) return ImColor(255, 0, 0);

	if (curTokIdx >= tokens.size()) curTokIdx = tokens.size() - 1;

	auto it = theme.find(tokens[curTokIdx].matchedClass);
	if (it != theme.end()) {
		return it->second;
	}

	return ImColor(255, 0, 0);
}

// Keybindings
static bool pasteTextFromClipBoard(GLFWwindow* window, Editor& e) {
	e.startTransaction();
	if (e.cursor.isSelection()) {
		e.emptySelection();
	}
	const char* cData = glfwGetClipboardString(window);
	e.insertBefore(std::string(cData));
	e.endTransaction();
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
	e.startTransaction();
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
	e.endTransaction();
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
	e.startTransaction();
	int y = e.cursor.selectionStart(e.buffer).y;
	if (y > 0) {
		std::string t = e.buffer[y - 1];
		e.buffer[y - 1] = e.buffer[y];
		e.buffer[y] = t;
		e.up();
	}

	e.endTransaction();
	return true;
}
static bool moveLineDown(GLFWwindow* window, Editor& e) {
	e.startTransaction();
	int y = e.cursor.selectionStart(e.buffer).y;
	if (y < e.buffer.size() - 1) {
		std::string t = e.buffer[y + 1];
		e.buffer[y + 1] = e.buffer[y];
		e.buffer[y] = t;
		e.down();
	}
	e.endTransaction();
	return true;
}
static bool copyLineDown(GLFWwindow* window, Editor& e) {
	e.startTransaction();

	int y = e.cursor.selectionStart(e.buffer).y;
	e.buffer.insert(e.buffer.begin() + e.cursor.end.y, e.buffer[y]);
	e.down();

	e.endTransaction();
	return true;
}
static bool copyLineUp(GLFWwindow* window, Editor& e) {
	e.startTransaction();

	int y = e.cursor.selectionStart(e.buffer).y;
	e.buffer.insert(e.buffer.begin() + e.cursor.end.y, e.buffer[y]);

	e.endTransaction();
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
static bool backspaceWord(GLFWwindow* window, Editor& e) {
	e.backspaceWord();
	return true;
}
static bool delWord(GLFWwindow* window, Editor& e) {
	e.delWord();
	return true;
}
static bool findWord(Editor& e, std::string search_query) {
	IVec2 gEnd = e.getGhostEnd();
	for (int i = gEnd.y;i < e.buffer.size();i++) {
		size_t found = e.buffer[i].find(search_query);
		if (found != std::string::npos) {
			e.cursor.start.y = i;
			e.cursor.start.x = found;
			e.cursor.end.y = i;
			e.cursor.end.x = found + search_query.size();
			return true;
		}
	}
	return false;
}

static void SetupTheme() {}

static TextBuffer tokenize(const std::string& text) {
	TextBuffer result;
	std::string segment;
	for (size_t i = 0; i < text.size(); i++) {
		if (text[i] == ' ') {
			result.push_back(segment);
			segment.clear();
			while (text[i + 1] == ' ')i++;
			continue;
		}
		segment += text[i];
	}
	result.push_back(segment);

	return result;
}

static void handleTitleBar(GLFWwindow* window) {
	static double s_xpos = 0, s_ypos = 0;
	static int w_xsiz = 0, w_ysiz = 0;
	static int dragState = 0;

	static ImGuiWindowFlags titlebar_flags =
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse;
	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(ImVec2{ viewport->WorkSize.x, 25.0f });
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2{ 0.0f, 0.0f });
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 40.0f, 2.0f });
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{ 0.01f, 0.01f, 0.01f, 1.0f });

	ImGui::Begin("window-frame-titlebar", nullptr, titlebar_flags);
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(1);
	ImGui::SetWindowFontScale(0.75f);
	ImGui::TextColored(ImVec4{ 1.0f,1.0f,1.0f,1.0f }, "Sad");
	ImGui::End();

	if (glfwGetMouseButton(window, 0) == GLFW_PRESS && dragState == 0) {
		glfwGetCursorPos(window, &s_xpos, &s_ypos);
		glfwGetWindowSize(window, &w_xsiz, &w_ysiz);
		dragState = 1;
	}
	if (glfwGetMouseButton(window, 0) == GLFW_PRESS && dragState == 1) {
		double c_xpos, c_ypos;
		int w_xpos, w_ypos;
		glfwGetCursorPos(window, &c_xpos, &c_ypos);
		glfwGetWindowPos(window, &w_xpos, &w_ypos);
		if (
			s_xpos >= 0 && s_xpos <= ((double)w_xsiz - 170) &&
			s_ypos >= 0 && s_ypos <= 25) {
			glfwSetWindowPos(window, w_xpos + (c_xpos - s_xpos), w_ypos + (c_ypos - s_ypos));
		}
		if (
			s_xpos >= ((double)w_xsiz - 15) && s_xpos <= ((double)w_xsiz) &&
			s_ypos >= ((double)w_ysiz - 15) && s_ypos <= ((double)w_ysiz)) {
			glfwSetWindowSize(window, w_xsiz + (c_xpos - s_xpos), w_ysiz + (c_ypos - s_ypos));
		}
	}
	if (glfwGetMouseButton(window, 0) == GLFW_RELEASE && dragState == 1) {
		dragState = 0;
	}
}

int gmain() {
	auto grammar = simpleJsGrammar();
	std::string input(R"(
		const height="strinjdhdjsdhdkhfgheght";
		const width = 500;
		// comment
		/* single line comment */
		/* multi line comment
		   multi line comment
		   multi line comment */
		if (width > height) {
			console.log({hey: "hello"});
		}
	)");
	auto matches = grammar.parseString(input);

	for (auto m : matches) {
		auto s = input.substr(m.start, m.end - m.start);
		std::cout << m << ": " << s << std::endl;
	}
	return 0;
}

static int lineNumberMode = 0;

static void renderEditor(Editor& editor, ImDrawList* drawList, ImGuiStyle& style) {
	const int lineNumberBarSize = 40;
	const int lineHeight = 24;
	const int charWidth = 10;
	const ImColor lineNoCol(100, 100, 100);
	ImVec2 p = ImGui::GetCursorScreenPos();


	// render cursor
	{
		const int cursorWidth = 2;
		IVec2 pos = editor.getGhostEnd();
		int yp = p.y + pos.y * lineHeight;
		int xp = p.x + lineNumberBarSize + pos.x * charWidth;
		ImVec2 start(xp, yp);
		ImVec2 end(start.x + cursorWidth, start.y + lineHeight);
		drawList->AddRectFilled(start, end, ImColor(255, 255, 255));
	}

	// render selection
	auto markSelectionLine = [&](int y, int sx, int ex) {
		ImVec2 lstart(lineNumberBarSize + p.x + sx * charWidth, p.y + y * lineHeight);
		ImVec2 lend(lineNumberBarSize + p.x + ex * charWidth, lstart.y + lineHeight);
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
			markSelectionLine(ymin, xmin, editor.buffer[ymin].size() + 1);
			for (int i = ymin + 1;i < ymax;i++) markSelectionLine(i, 0, editor.buffer[i].size() + 1);
			markSelectionLine(ymax, 0, xmax);
		}
	}

	// Render text
	char lineNoBuffer[10];

	int charCount = 0;
	int curTokIdx = 0;
	int maxLineLength = 0;
	ImColor textCol = getTokenColor(curTokIdx, editor.tokens, SyntaxTheme);
	for (int lineNo = 0;lineNo < editor.buffer.size();lineNo++) {
		sprintf_s(lineNoBuffer, "%d", lineNo + 1);

		int yp = p.y + lineNo * lineHeight;
		int xp = p.x;
		drawList->AddText(ImVec2(xp, yp), lineNoCol, lineNoBuffer);
		xp += lineNumberBarSize;

		if (editor.buffer[lineNo].size() > maxLineLength) maxLineLength = editor.buffer[lineNo].size();

		for (int j = 0;j < editor.buffer[lineNo].size();j++) {
			const char* lbegin = editor.buffer[lineNo].data();
			drawList->AddText(ImVec2(xp, yp), textCol, lbegin + j, lbegin + j + 1);

			xp += charWidth;
			charCount++;
			if (curTokIdx < editor.tokens.size() && charCount >= editor.tokens[curTokIdx].end) {
				curTokIdx++;
				textCol = getTokenColor(curTokIdx, editor.tokens, SyntaxTheme);
			}

			if (j != editor.buffer[lineNo].size() - 1) ImGui::SameLine(0, 0);
		}
		charCount++;
		if (curTokIdx < editor.tokens.size() && charCount >= editor.tokens[curTokIdx].end) {
			curTokIdx++;
			textCol = getTokenColor(curTokIdx, editor.tokens, SyntaxTheme);
		}
	}

	ImVec2 scrollSpace(maxLineLength * charWidth + lineNumberBarSize + 100, editor.buffer.size() * lineHeight);
	ImGui::Dummy(scrollSpace);
}

// Main code
int main(int, char**) {
	std::string path = "C:\\Users\\arun.mulakkal\\xlabs\\ramdon";
	std::filesystem::path filepath = std::filesystem::path(path);
	std::cout << "Filepath is: " << filepath.string() << std::endl;

	FileTree ft = FileTree(filepath);

	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit()) return 1;

#ifdef NO_TITLE_BAR
	glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
#endif
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Sad", nullptr, nullptr);
	if (window == nullptr) return 1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	SetupTheme();
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
	Grammar grammar = simpleJsGrammar();
	editor.loadGrammar(grammar);
	float scroll_y = 0.0f;
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
		KeyBinding(ImGuiKey_Backspace, Ctrl, backspaceWord),
		KeyBinding(ImGuiKey_Delete, Ctrl, delWord),
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


#ifdef NO_TITLE_BAR
		handleTitleBar(window);
#endif // NO_TITLE_BAR


		// File tree
		{
			ImGui::Begin("FileTree");

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

		// Debug
		{
			ImGui::Begin("Debug");

			ImGui::Text("Ref count %d", editor.transactionRefCount);
			ImGui::Text("Undo stack %d", editor.undoHistory.size());
			ImGui::Text("Redo stack %d", editor.redoHistory.size());
			ImGui::Text("Num Lines %d", editor.buffer.size());
			if (editor.cursor.isSelection()) {
				auto gps = editor.cursor.selectionStart(editor.buffer);
				ImGui::Text("Selection Start (%d, %d)", gps.y, gps.x);
				auto gpe = editor.cursor.selectionEnd(editor.buffer);
				ImGui::Text("Selection End (%d, %d)", gpe.y, gpe.x);
			}
			else {
				auto gp = editor.getGhostEnd();
				ImGui::Text("Cursor (%d, %d)", gp.y, gp.x);
			}


			ImGui::End();
		}

		// Command Pallete
		{
			ImGui::Begin("Command");
			// on enter return focus back to editor, but for next frame
			// TODO this seems like a hack, fix it
			static int shoudlFocusEditor = false;
			if (ImGui::IsWindowFocused() && shoudlFocusEditor) {
				std::cout << "focusing editor\n";
				ImGui::SetWindowFocus("Editor");
				shoudlFocusEditor = false;
			}

			char commandInputBuf[256] = "";
			if (ImGui::InputText("##Command", commandInputBuf, IM_ARRAYSIZE(commandInputBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
				// command submitted
				std::vector<std::string> tokens = tokenize(commandInputBuf);
				if (tokens.size() > 0) {
					int repeat = 1;
					std::string command = tokens[0];
					if (tokens.size() == 2) {
						try {
							repeat = std::stoi(tokens[0]);
						}
						catch (const std::invalid_argument& e) {
							std::cerr << "Error converting string: " << e.what() << std::endl;
						}

						command = tokens[1];
					}
					for (int i = 0;i < repeat;i++) {
						if (command == "toggleLineNoMode") lineNumberMode = (lineNumberMode + 1) % 2;
						else if (command == "up") editor.up();
						else if (command == "down") editor.down();
						else if (command == "left") editor.left();
						else if (command == "right") editor.right();
						else if (command == "find") findWord(editor, "query");
					}

					shoudlFocusEditor = true;
				}
			}

			// part of above hack
			if (ImGui::IsWindowFocused() and !shoudlFocusEditor) {
				ImGui::SetKeyboardFocusHere(-1);
			}

			ImGui::End();
		}

		// Editor
		{
			// prevent scroll on arrow key
			bool isKeyScrolling = ImGui::IsKeyDown(ImGuiKey_DownArrow) || ImGui::IsKeyDown(ImGuiKey_UpArrow);
			if (isKeyScrolling) {
				ImGui::SetNextWindowScroll({ ImGui::GetScrollX(), scroll_y });
			}

			ImGui::Begin("Editor");
			if (!isKeyScrolling) {
				scroll_y = ImGui::GetScrollY();
			}

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			renderEditor(editor, drawList, style);

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
				bool ctrlDown = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);

				if (handled) {}
				else if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
					// TODO rework these using keybindings, only single keys should be here
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
					std::cout << "Delete\n";
					editor.del();
				}
				else if (ctrlDown && ImGui::IsKeyPressed(ImGuiKey_P)) {
					std::cout << "Focuscommand pallete\n";
					ImGui::SetWindowFocus("Command");
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
