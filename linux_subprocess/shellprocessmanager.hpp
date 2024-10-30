#ifndef SHELL_PROCESS_MANAGER_H
#define SHELL_PROCESS_MANAGER_H

#include <iostream>
#include <sstream>
#include <thread>
#include "parallel_hashmap/phmap.h"
#include <map>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string>

class ShellProcessManager {
public:
    ShellProcessManager(const char* command) : Command(command), continue_reading_stdout(true), continue_reading_stderr(true), buffLen(4096) {
        pipe(pip0);
        pipe(pip1);
        pipe(pip2);
        PID = fork();
        if (PID < 0) {
            throw std::runtime_error("Failed to fork process");
        }
        if (PID == 0) {
            childProcess();
        }
        else {
            parentProcess();
        }
    }

    ~ShellProcessManager() {
        stopThreads();
    }

    void stdinWrite(const std::string& command) {
        std::string mycommand = command + "\n";
        fputs(mycommand.c_str(), pXFile);
        fflush(pXFile);
    }

    std::map<int, std::string> readStdOut() {
        return readFromMap(strmap_out);
    }

    std::map<int, std::string> readStdErr() {
        return readFromMap(strmap_err);
    }

private:
    const char* Command;
    bool continue_reading_stdout;
    bool continue_reading_stderr;
    int buffLen;
    int pip0[2], pip1[2], pip2[2];
    int FDChildStdin, FDChildStdout, FDChildStderr;
    pid_t PID;
    FILE* pXFile;
    phmap::node_hash_map<int, std::string> strmap_out;
    phmap::node_hash_map<int, std::string> strmap_err;
    std::thread t1;
    std::thread t2;

    void childProcess() {
        close(pip0[1]);
        close(pip1[0]);
        close(pip2[0]);
        dup2(pip2[1], 2);
        dup2(pip1[1], 1);
        dup2(pip0[0], 0);
        close(pip0[0]);
        close(pip1[1]);
        close(pip2[1]);
        char* argv[1] = {};
        char* envp[1] = {};
        execve(Command, argv, envp);
        exit(-1);
    }

    void parentProcess() {
        FDChildStdin = pip0[1];
        FDChildStdout = pip1[0];
        FDChildStderr = pip2[0];
        pXFile = fdopen(FDChildStdin, "w");

        t1 = std::thread(&ShellProcessManager::readFromStdOut, this);
        t2 = std::thread(&ShellProcessManager::readFromStdErr, this);
    }

    void readFromStdOut() {
        char buff[4096];
        int myco = 0;
        while (continue_reading_stdout) {
            int iret = read(FDChildStdout, buff, buffLen);
            if (!continue_reading_stdout) break;
            strmap_out.emplace(myco++, buff);
            memset(buff, 0, buffLen);
        }
    }

    void readFromStdErr() {
        char bufferr[4096];
        int myco = 0;
        while (continue_reading_stderr) {
            int iret = read(FDChildStderr, bufferr, buffLen);
            if (!continue_reading_stderr) break;
            strmap_err.emplace(myco++, bufferr);
            memset(bufferr, 0, buffLen);
        }
    }

    std::map<int, std::string> readFromMap(phmap::node_hash_map<int, std::string>& strmap) {
        std::map<int, std::string> v;
        if (strmap.empty()) {
            return v;
        }
        while (strmap.begin()!=strmap.end()) {
            int it = strmap.begin()->first;
            std::string it2 = strmap.begin()->second;
            v[it] = it2;
            strmap.erase(strmap.begin());
        }
        return v;
    }

    void stopThreads() {
        continue_reading_stdout = false;
        continue_reading_stderr = false;
        stdinWrite(">&2 echo done stderr\n");
        stdinWrite("echo done stdout\n");
        fclose(pXFile);
        close(FDChildStdin);
        close(FDChildStdout);
        close(FDChildStderr);
        pthread_cancel(t1.native_handle());
        pthread_cancel(t2.native_handle());
        if (t1.joinable()) t1.join();
        if (t2.joinable()) t2.join();
    }
};

#endif // SHELL_PROCESS_MANAGER_H
