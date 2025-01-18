# Communicating continuously with a subprocess (stdin/stdout/stderr) without deadlocking

Two classes for Windows and Linux that facilitate launching and controlling shells like powershell.exe, adb.exe and cmd.exe, sh, bash, etc. The shells are running constantly in the background, always ready to receive and execute commands. Communicating with them (writing to stdin, reading from stdout and stderr separately) happens without deadlocking.

 ### Advantages
 * no deadlocks ever
 * all output from stderr / stdout is captured separately, not a single byte escapes
 * read chunks are enumerated and returned in order 
 * Live capture - get the output even when the process is still executing
 * uses only the standard library
 * for Linux and Windows
 * installation: copy and paste the code into your project and import it (examples below)
 * no CMake required
 * very stable
 * launch the shell only once, and keep using it, no need to use one popen call for each command
 * no limitation on the number of commands you can execute through 
   stdin (Most subprocess/popen code (e.g., Python) permit only one input command, and calls then communicate(), and the party is over)
 * cmd.exe or shell exclusive commands are supported (dir, cd, etc.)
 * very simple API (starting, writing to stdin, reading from stdout,stderr - that's it)
 * Optional CreateProcess parameters for Windows
 * Automatic cleanup
 * Launch it with any shell you want (bash, sh, adb, cmd.exe, powershell, etc.)
 * Tested with g++, zig, msvc and clang
 * No "double escaping" needed, due to the direct interaction with the shell


```cpp
#include "nonblockingsubprocess.hpp"

int main(int argc, char *argv[])
{
    while (true)
    {
#ifdef _WIN32
        std::string shellcmd = "C:\\Windows\\System32\\cmd.exe";
#else
        std::string shellcmd = "/bin/bash";
#endif
        ShellProcessManager proc{shellcmd, 4096, 4096, 4096, "exit"};
        bool resultproc = proc.start_shell(); //optional arguments for Windows: DWORD creationFlag = 0, DWORD creationFlags = CREATE_NO_WINDOW, WORD wShowWindow = SW_NORMAL, LPSTR lpReserved = nullptr, LPSTR lpDesktop = nullptr, LPSTR lpTitle = nullptr, DWORD dwX = 0, DWORD dwY = 0, DWORD dwXSize = 0, DWORD dwYSize = 0, DWORD dwXCountChars = 0, DWORD dwYCountChars = 0, DWORD dwFillAttribute = 0, DWORD dwFlags = 0, WORD cbReserved2 = 0, LPBYTE lpReserved2 = nullptr
        std::cout << "resultproc: " << resultproc << std::endl;
        proc.stdin_write("ls -l");
        sleepcp(100);
        auto val = proc.get_stdout();
        std::cout << "stdout: " << val << std::endl;
        sleepcp(100);
        proc.stdin_write("ls -l");
        sleepcp(100);
        auto val2 = proc.get_stdout();
        std::cout << "stderr: " << val << std::endl;
        proc.stop_shell(); // optional: automatically called by the destructor
        sleepcp(1000);
    }
}
```