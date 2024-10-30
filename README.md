# Communicating continuously with a subprocess (stdin/stdout/stderr) without deadlocking

Two classes for Windows and Linux that facilitate launching and controlling shells like powershell.exe, adb.exe and cmd.exe, sh, bash, etc. The shells are running constantly in the background, always ready to receive and execute commands. Communicating with them (writing to stdin, reading from stdout and stderr separately) happens without deadlocking.

### Example output - running *dir / ls -l* every second and capturing the output

[![YT](https://i.ytimg.com/vi/-lAtMUOHMyA/maxresdefault.jpg)](https://www.youtube.com/watch?v=-lAtMUOHMyA)
[https://www.youtube.com/watch?v=-lAtMUOHMyA]()

## The Windows version

```cpp
@class ShellProcessManager 

 @brief This class facilitates launching a command shell (`cmd.exe` by default) and reading/writing to/from its standard input, output, and error streams. 
  It provides multithreaded mechanisms for continuously reading from stdout and stderr, while allowing for writing commands to stdin.

class ShellProcessManager {
public:
   
    Initializes the STARTUPINFO structure, sets up default parameters for process creation, and creates necessary pipes for I/O handling.

    @param shellCommand The command to execute, defaults to `cmd.exe`.
    @param creationFlags Flags for process creation.
    @param wShowWindow Controls how the window is to be shown.
    @param lpReserved Reserved for future use.
    @param lpDesktop Specifies the desktop associated with the process.
    @param lpTitle The title of the console window.
    @param dwX, dwY, dwXSize, dwYSize, dwXCountChars, dwYCountChars Various window attributes for process startup.
    @param dwFillAttribute Attribute for console screen buffer.
    @param dwFlags STARTUPINFO flags.
    @param cbReserved2 Reserved byte count.
    @param lpReserved2 Reserved pointer.

```
More information about the parameters here:

https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw

https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessa

### Usage example 

```cpp
#include "shellprocessmanager.hpp"
#include <algorithm>

//compile: zig c++ -std=c++2a -O3 -g0 subprocess.cpp
bool is_whitespace(const std::string& s) {
	return std::all_of(s.begin(), s.end(), isspace);
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

	while (counter < 4000) {
		counter++;
		ph.stdinWrite("dir");
		Sleep(1000); //1 s

		auto stdout_output = ph.readStdOut();
		for (const auto& out : stdout_output) {
			std::string tmpstring = out.second;
			// do some automation stuff here, like:
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

```

## The Linux version 

```cpp
  @class ShellProcessManager
  
 
  @brief This class facilitates launching a command shell (`/bin/sh` by default) and reading/writing to/from its standard input, output, and error streams. 
  It provides multithreaded mechanisms for continuously reading from stdout and stderr, while allowing for writing commands to stdin.
 
class ShellProcessManager {
public:
    
      @brief Constructor for ShellProcessManager.
      
      Initializes the necessary parameters for process creation and sets up pipes for I/O handling.
      
      @param command The command to execute, defaults to `/bin/sh`.
```

### Usage example 

```cpp 
#include "shellprocessmanager.hpp"
//compile and run: g++ -std=c++2a -O3 -g0 subprocess.cpp -o sourceBinary; ./sourceBinary
int main() {
    try {
        ShellProcessManager shellManager("/bin/sh");
        int counter = 0;
        while (counter < 200000) {
            counter++;
            shellManager.stdinWrite("ls -l");
            usleep(1000 * 1000); // 1 second

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

```
