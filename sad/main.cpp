#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include <GLFW/glfw3.h>

#include "FileTree.h"
#include "Editor.h"
#include "KeyBindings.h"
#include "Grammar.h"
#include "Timer.h"
#include "CommandCenter.h"

#include <stdio.h>
#include <regex>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <chrono>
#include <unordered_map>

static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

typedef std::unordered_map<std::string, ImColor> SyntaxHighlightTheme;

SyntaxHighlightTheme SyntaxTheme = {
	{ "control",        ImColor(94, 129, 172) },   // Nord Blue (#5E81AC)
	{ "declaration",    ImColor(191, 97, 106) },   // Nord Red (#BF616A)
	{ "context",        ImColor(136, 192, 208) },  // Nord Light Blue (#88C0D0)
	{ "literal",        ImColor(208, 135, 112) },  // Nord Orange (#D08770)
	{ "string",         ImColor(163, 190, 140) },  // Nord Green (#A3BE8C)
	{ "numeric",        ImColor(180, 142, 173) },  // Nord Purple (#B48EAD)
	{ "operator",       ImColor(236, 239, 244) },  // Nord Snow 2 (#ECEFF4)
	{ "comment",        ImColor(106, 115, 117) },  // Nord Blue Gray (#81A1C1)
	{ "variable",       ImColor(143, 188, 187) },  // Nord Cyan (#8FBCBB)
	{ "punctuation",    ImColor(236, 239, 244) },  // Nord Snow 2 (#ECEFF4)
};

static ImColor getTokenColor(const std::string& matchedClass, SyntaxHighlightTheme& theme) {
	auto it = theme.find(matchedClass);
	if (it != theme.end()) return it->second;

	return ImColor(255, 255, 255);
}

// Keybindings
static bool pasteTextFromClipBoard(GLFWwindow* window, Editor& e) {
	e.startTransaction();
	// something funny with pasting at end of line
	for (size_t i = 0; i < e.cursors.size(); i++) {
		if (e.cursors[i].isSelection()) {
			e.emptySelection(i);
		}
	}
	const char* cData = glfwGetClipboardString(window);
	e.insertBefore(std::string(cData));
	e.endTransaction();
	return true;
}
static bool copyTextToClipBoard(GLFWwindow* window, Editor& e) {
	hack("considering only 0th cursor in copyTextToClipBoard");

	if (e.cursors[0].isSelection()) {
		glfwSetClipboardString(window, e.getSelectionString(0).c_str());
	}
	else {
		// TODO try to better copy vscode style copying
		glfwSetClipboardString(window, e.buffer[e.getCursorStart(0).y].c_str());
	}
	return true;
}
static bool cutTextToClipBoard(GLFWwindow* window, Editor& e) {
	hack("considering only 0th cursor in cutTextToClipBoard");

	e.startTransaction();

	if (e.cursors[0].isSelection()) {
		glfwSetClipboardString(window, e.getSelectionString(0).c_str());
		e.emptySelection(0);
	}
	else {
		// TODO try to better copy vscode style cutting
		size_t lineNo = e.getCursorStart(0).y;
		glfwSetClipboardString(window, e.buffer[lineNo].c_str());
		e.buffer.erase(e.buffer.begin() + lineNo);
	}
	e.endTransaction();
	return true;
}
static bool selectAll(GLFWwindow* window, Editor& e) {
	Cursor scurs;
	scurs.start.x = 0;
	scurs.start.y = 0;
	scurs.end.y = e.buffer.size() - 1;
	scurs.end.x = e.buffer[e.buffer.size() - 1].size();

	e.cursors = { scurs };
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
	hack("considering only 0th cursor in moveLineUp");

	e.startTransaction();
	size_t y = e.cursors[0].selectionStart(e.buffer).y;
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
	hack("considering only 0th cursor in moveLineDown");

	e.startTransaction();
	size_t y = e.cursors[0].selectionStart(e.buffer).y;
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
	hack("considering only 0th cursor in copyLineDown");

	e.startTransaction();

	size_t y = e.cursors[0].selectionStart(e.buffer).y;
	e.buffer.insert(e.buffer.begin() + e.cursors[0].end.y, e.buffer[y]);
	e.down();

	e.endTransaction();
	return true;
}
static bool copyLineUp(GLFWwindow* window, Editor& e) {
	hack("considering only 0th cursor in copyLineUp");

	e.startTransaction();

	size_t y = e.cursors[0].selectionStart(e.buffer).y;
	e.buffer.insert(e.buffer.begin() + e.cursors[0].end.y, e.buffer[y]);

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
static bool insertCursorAbove(GLFWwindow* window, Editor& e) {
	IVec2 pos = e.cursors[0].selectionEnd(e.buffer);

	if (pos.y > 0) {
		Cursor ncurs(pos.x, pos.y - 1);
		e.cursors.insert(e.cursors.begin(), ncurs);
	}

	return true;
}
static bool insertCursorBelow(GLFWwindow* window, Editor& e) {
	IVec2 pos = e.cursors[e.cursors.size() - 1].selectionEnd(e.buffer);

	if (pos.y < e.buffer.size() - 1) {
		Cursor ncurs(pos.x, pos.y + 1);
		e.cursors.push_back(ncurs);
	}

	return true;
}
static bool resetCursors(GLFWwindow* window, Editor& e) {
	IVec2 pos = e.cursors[0].selectionEnd(e.buffer);

	e.cursors = { Cursor(pos.x,pos.y) };

	return true;
}
static bool findWord(Editor& e, CommandArgs args) {
	std::string search_query = args[0];

	IVec2 gPos = e.getGhostEnd(0);
	size_t searchStartPos = gPos.y;
	size_t line = searchStartPos;
	size_t offset = gPos.x;
	do {
		size_t found = e.buffer[line].find(search_query, offset);
		if (found != std::string::npos) {
			Cursor fcurs;
			fcurs.start.y = line;
			fcurs.end.y = line;
			fcurs.start.x = found;
			fcurs.end.x = found + search_query.size();
			e.cursors = { fcurs };
			break;
		}

		offset = 0;
		line = (line + 1) % e.buffer.size();
	} while (line != searchStartPos);

	// this always returns false 
	// so that input stays on focus 
	// and users can search for next just by enter
	return false;
}
static bool replaceWord(Editor& e, CommandArgs args) {
	std::string find = args[0];
	std::string replace = args[1];

	IVec2 gPos = e.getGhostEnd(0);
	size_t searchStartPos = gPos.y;
	size_t line = searchStartPos;
	size_t offset = gPos.x;
	do {
		size_t found = e.buffer[line].find(find, offset);
		if (found != std::string::npos) {
			Cursor fcurs;
			fcurs.start.y = line;
			fcurs.end.y = line;
			fcurs.start.x = found;
			fcurs.end.x = found + find.size();
			e.cursors = { fcurs };

			e.startTransaction();

			e.emptySelection(0);
			e.insertBefore(replace);
			e.cursors[0].end.x -= replace.size();

			e.endTransaction();
			break;
		}

		offset = 0;
		line = (line + 1) % e.buffer.size();
	} while (line != searchStartPos);

	// this always returns false 
	// so that input stays on focus 
	// and users can search for next just by enter
	return false;
}

static void SetupTheme() {}

static const int lineNumberMode = 0;
static const int lineHeight = 24;
static const int charWidth = 10;

static void renderEditor(Editor& editor, ImDrawList* drawList, ImGuiStyle& style) {
	const int lineNumberBarSize = 40;
	const ImColor lineNoCol(100, 100, 100);
	ImVec2 p = ImGui::GetCursorScreenPos();


	// render cursors
	for (size_t i = 0; i < editor.cursors.size(); i++) {
		const int cursorWidth = 2;
		IVec2 pos = editor.getGhostEnd(i);
		float yp = p.y + pos.y * lineHeight;
		float xp = p.x + lineNumberBarSize + pos.x * charWidth;
		ImVec2 start(xp, yp);
		ImVec2 end(start.x + cursorWidth, start.y + lineHeight);
		drawList->AddRectFilled(start, end, ImColor(255, 255, 255));
	}

	// render selection
	auto markSelectionLine = [&](size_t y, size_t sx, size_t ex) {
		ImVec2 lstart(lineNumberBarSize + p.x + sx * charWidth, p.y + y * lineHeight);
		ImVec2 lend(lineNumberBarSize + p.x + ex * charWidth, lstart.y + lineHeight);
		ImColor selCol = ImColor(1.0f, 1.0f, 1.0f, 0.3f);

		drawList->AddRectFilled(lstart, lend, selCol);
		};

	for (const Cursor& cursor : editor.cursors) {
		if (cursor.isSelection()) {
			IVec2 st = cursor.selectionStart(editor.buffer);
			IVec2 ed = cursor.selectionEnd(editor.buffer);

			size_t xmin = st.x, xmax = ed.x;
			size_t ymin = st.y, ymax = ed.y;

			if (ymin == ymax) {
				markSelectionLine(ymax, xmin, xmax);
			}
			else {
				markSelectionLine(ymin, xmin, editor.buffer[ymin].size() + 1);
				for (size_t i = ymin + 1;i < ymax;i++) markSelectionLine(i, 0, editor.buffer[i].size() + 1);
				markSelectionLine(ymax, 0, xmax);
			}
		}
	}

	// Render line numbers
	size_t maxLineLength = 0;
	char lineNoBuffer[10];
	for (int lineNo = 0;lineNo < editor.buffer.size();lineNo++) {
		sprintf_s(lineNoBuffer, "%d", lineNo + 1);

		float yp = p.y + lineNo * lineHeight;
		float xp = p.x;
		drawList->AddText(ImVec2(xp, yp), lineNoCol, lineNoBuffer);
		xp += lineNumberBarSize;

		if (editor.buffer[lineNo].size() > maxLineLength) maxLineLength = editor.buffer[lineNo].size();
	}

	// render tokens
	for (const GrammarMatch& token : editor.tokens) {
		if (token.matchedClass == "whitespace") continue;

		float xp = p.x + lineNumberBarSize + token.start * charWidth;
		float yp = p.y + token.line * lineHeight;
		drawList->AddText(
			ImVec2(xp, yp),
			getTokenColor(token.matchedClass, SyntaxTheme),
			editor.buffer[token.line].data() + token.start,
			editor.buffer[token.line].data() + token.end
		);
	}

	ImVec2 scrollSpace(
		(float)(maxLineLength * charWidth + lineNumberBarSize + 100),
		(float)(editor.buffer.size() * lineHeight));
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
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

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
	CommandCenter commandCenter = {};
	commandCenter.addCommand("up", CMD_DECL{ return e.up(); });
	commandCenter.addCommand("down", CMD_DECL{ return e.down(); });
	commandCenter.addCommand("left", CMD_DECL{ return e.left(); });
	commandCenter.addCommand("right", CMD_DECL{ return e.right(); });
	commandCenter.addCommand("insert", CMD_DECL{ e.insertBefore(args[0]); return true; }, 1);
	commandCenter.addCommand("find", findWord, 1);
	commandCenter.addCommand("replace", replaceWord, 2);

	Grammar grammar = simpleJsGrammar();
	editor.loadGrammar(grammar);
	editor.insertBefore(R"(const hello = "heheh";
const hey = 5.88181;

// hello
/* comment */
/* 
  multi line comment
*/ const x = 0;
console.log({ hello: hello, hey: hey });

const brother = (x) => {
  if (x > 100) {
    return 200;
  } else {
    throw new Error(something);
  }
};

export default class NewClass {
  constructor() {
    brother();
  }

  async root() {}
}
)");

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
		KeyBinding(ImGuiKey_UpArrow, Ctrl + Shift, insertCursorAbove),
		KeyBinding(ImGuiKey_DownArrow, Ctrl + Shift, insertCursorBelow),
		KeyBinding(ImGuiKey_A, Ctrl, selectAll),
		KeyBinding(ImGuiKey_Z, Ctrl, undo),
		KeyBinding(ImGuiKey_Y, Ctrl, redo),
		KeyBinding(ImGuiKey_Backspace, Ctrl, backspaceWord),
		KeyBinding(ImGuiKey_Delete, Ctrl, delWord),
		KeyBinding(ImGuiKey_Escape, 0, resetCursors),// maybe this doesnt belong here, move it down
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

			ImGui::Text("FPS = %.1f", io.Framerate);
			ImGui::Text("Ref count %d", editor.transactionRefCount);
			ImGui::Text("Undo stack %d", editor.undoHistory.size());
			ImGui::Text("Redo stack %d", editor.redoHistory.size());
			ImGui::Text("Num Lines %d", editor.buffer.size());

			for (const Cursor& cursor : editor.cursors) {
				if (cursor.isSelection()) {
					auto gps = cursor.selectionStart(editor.buffer);
					ImGui::Text("Selection Start (%d, %d)", gps.y, gps.x);
					auto gpe = cursor.selectionEnd(editor.buffer);
					ImGui::Text("Selection End (%d, %d)", gpe.y, gpe.x);
				}
				else {
					auto gp = cursor.end.getGhotsPos(editor.buffer);
					ImGui::Text("Cursor (%d, %d)", gp.y, gp.x);
				}
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

			static char commandInputBuf[256] = "";
			if (ImGui::InputText("##Command", commandInputBuf, IM_ARRAYSIZE(commandInputBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
				shoudlFocusEditor = commandCenter.dispatch(editor, commandInputBuf);
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
					editor.enterAndIndent();
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

				// if handled scroll cursor into focus
				if (handled) {
					//TODO currently we are only scrolling vertically
					size_t curY = editor.getCursorEnd(0).y;

					float scrollY = ImGui::GetScrollY();
					float curPosY = (float)curY * lineHeight;
					float screenY = ImGui::GetWindowContentRegionMin().y;

					if (curPosY < scrollY || curPosY > scrollY + screenY) {
						ImGui::SetScrollY(curPosY);
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
