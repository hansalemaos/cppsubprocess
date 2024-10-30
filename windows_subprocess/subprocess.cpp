#include "shellprocessmanager.hpp"
#include <algorithm>
//#include "folly"
//compile: zig c++ -std=c++2a -O3 -g0 subprocess.cpp
bool is_whitespace(const std::string& s) {
	return std::all_of(s.begin(), s.end(), isspace);
}

auto replace_rn(std::string str ) {
	const std::string str2= "\r\n";
	std::size_t found = str.find(str2);
	if (found != std::string::npos)
		str[found] = ' ';
	return str;
}
int main() {
	std::string myshell = "C:\\Windows\\System32\\cmd.exe";
	ShellProcessManager ph(myshell);
	if (!ph.Initialize()) {
		std::cerr << "Failed to initialize the process." << std::endl;
		return -1;
	}
	int counter = 0;
	ph.StartReadingThreads();

	while (counter++ < 4000) {
		ph.stdinWrite("dir");
		//Sleep(1000); //1 s
		Sleep(100);

		auto stdout_output = ph.readStdOut();
		for (const auto& out : stdout_output) {
			std::string tmpstring = replace_rn(out.second); 
			// do some automation stuff here
			//if (is_whitespace(tmpstring)) continue; 
			//if (tmpstring.size() < 3) continue;
			//tmpstring.replace(tmpstring.size()-2,tmpstring.size()-1,1,'\n');
			std::cout << tmpstring;
		}
		std::cout << std::endl;
		auto stderr_output = ph.readStdErr();
		for (const auto& out : stderr_output) {
			if (is_whitespace(out.second))  continue; // do some automation stuff here
			std::cout << out.first << " : " << out.second << std::endl;
		}
	}
	return 0;
}
