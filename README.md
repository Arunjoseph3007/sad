# sad

A simple text editor written in C++ using ImGui

It looks like this
![Sad editor](./assets/image.png "Sad editor")

## Features
We are at a very basic stage so lower you expectations
- Simple Cursor navigation
- Simple selection
- Basic Keybindings system
- Cut/Copy/Paste
- Cursor position aware Undo/Redo
- syntax highlighting
- Basic command center

## TODO
- Multi cursor
- Undo/Redo system
  - We are using line diff, maybe more specific diff?
  - A better way of start/end transactions, dont want to write it everywhere
- Selecting by clicking/dragging
- Actually save the file
- Edit multiple files simultaneously (optional)
- Better UI styling
- fix alignment issue due to `\r \n \t`
- Text wrapping
- searching
  - maybe also use search by regex
- emacs style query replace
- Handle gigantic files
- Handle line number better
- Command system
  - we could use [sol2](https://github.com/ThePhD/sol2)
- Syntax highlighting
  - Preferably based on active mode
- auto completions
- LSP integration
- modding using lua

## Reference
- Syntax highlighting - we ultimately cooked our own simple (less powerful) system
  - [How Emacs modes work?](https://www.emacswiki.org/emacs/ModeTutorial)
  - [TextMate Grammar](https://macromates.com/manual/en/language_grammars)
  - [Sublime Text Syntax Definitions](https://www.sublimetext.com/docs/syntax.html)
- Embedding & scripting
  - [sol2](https://github.com/ThePhD/sol2)