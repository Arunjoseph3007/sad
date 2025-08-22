#include "Edit.h"
#include <iostream>

void Edit::undo(TextBuffer& buffer) const {
	buffer.erase(buffer.begin() + this->start, buffer.begin() + this->start + this->plus.size());

	for (int i = 0;i < this->minus.size();i++) {
		buffer.insert(buffer.begin() + this->start + i, this->minus[i]);
	}
}

void Edit::redo(TextBuffer& buffer) const {
	buffer.erase(buffer.begin() + this->start, buffer.begin() + this->start + this->minus.size());

	for (int i = 0;i < this->plus.size();i++) {
		buffer.insert(buffer.begin() + this->start + i, this->plus[i]);
	}
}

std::ostream& operator<<(std::ostream& os, const Edit& ed) {
	os << "Edit(start: " << ed.start << ", plus: {";

	for (int i = 0;i < ed.plus.size();i++) { 
		os << "\"" << ed.plus[i] << "\""; 

		if (i < ed.plus.size() - 1) os << ", ";
	}
	os << "}, minus: {";
	
	for (int i = 0;i < ed.minus.size();i++) {
		os << "\"" << ed.minus[i] << "\"";

		if (i < ed.minus.size() - 1) os << ", ";
	}
	os << "})";

	return os;
}


/*
0:hello
1:ab|cd|ef
2:world
-
0:hello
1:abXYZ|ef
2:world
=
Edit(y1=1,y2=2,plus={"abXYZef"},minus={"abcdef"})

ab|cd|ef
  ^
  s
ab|XYZ|ef

*/

