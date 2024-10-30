#include "shellprocessmanager.hpp"
//compile and run: g++ -std=c++2a -O3 -g0 subprocess.cpp -o sourceBinary; ./sourceBinary
int main() {
    try {
        ShellProcessManager shellManager("/bin/sh");
        int counter = 0;
        while (counter < 400000) {
            counter++;
            shellManager.stdinWrite("ls -l");
           // usleep(1000 * 1000); // 1 second
            usleep(1000 * 100); 

            auto stdoutmap = shellManager.readStdOut();
            for (auto& it : stdoutmap) {
                // do something here
                std::cout << it.first << ": " << it.second;
            }
            auto stderrmap = shellManager.readStdErr();
            for (auto& it : stderrmap) {
                // do something here
                std::cout << it.first << ": " << it.second;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
