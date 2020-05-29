#ifndef BASICNETWORKING_NOTIFIER
#define BASICNETWORKING_NOTIFIER

#include <iostream>
#include <string>

namespace notifier {
enum Color {
	black, red, green, yellow, blue, purple, magenta, cyan, white
};

struct Notifier {
	
	static void notify(std::string msg, Color color = Color::black) {
		std::cout << msg <<'\n';
		return; //DEBUG color codes don't work anymore
		std::string print = "\x1B[3" + std::to_string(color) + "m" + msg + "\033[0m\n";
		std::cout << print;
	}
	
};

}

#endif
