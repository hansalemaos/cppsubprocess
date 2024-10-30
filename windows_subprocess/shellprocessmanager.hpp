#ifndef SHELL_PROCESS_MANAGER_H
#define SHELL_PROCESS_MANAGER_H

#include <windows.h>
#include <iostream>
#include <sstream>
#include <thread>
#include "parallel_hashmap/phmap.h"
#include <map>
#include <string>
#include <stdio.h>
#include <stdlib.h>

class ShellProcessManager {
public:
	ShellProcessManager(std::string shellCommand = "cmd.exe", DWORD creationFlags = CREATE_NO_WINDOW, WORD wShowWindow = SW_NORMAL, LPSTR lpReserved = nullptr, LPSTR lpDesktop = nullptr, LPSTR lpTitle = nullptr, DWORD dwX = 0, DWORD dwY = 0, DWORD dwXSize = 0, DWORD dwYSize = 0, DWORD dwXCountChars = 0, DWORD dwYCountChars = 0, DWORD dwFillAttribute = 0, DWORD dwFlags = 0, WORD cbReserved2 = 0, LPBYTE lpReserved2 = nullptr) :
		continue_reading_stdout(true),
		continue_reading_stderr(true),
		siStartInfo{}
	{
		ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
		siStartInfo.lpReserved = lpReserved;
		siStartInfo.lpDesktop = lpDesktop;
		siStartInfo.lpTitle = lpTitle;
		siStartInfo.dwX = dwX;
		siStartInfo.dwY = dwY;
		siStartInfo.dwXSize = dwXSize;
		siStartInfo.dwYSize = dwYSize;
		siStartInfo.dwXCountChars = dwXCountChars;
		siStartInfo.dwYCountChars = dwYCountChars;
		siStartInfo.dwFillAttribute = dwFillAttribute;
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
		siStartInfo.wShowWindow = wShowWindow;
		siStartInfo.cbReserved2 = cbReserved2;
		siStartInfo.lpReserved2 = lpReserved2;
		siStartInfo.cb = sizeof(STARTUPINFO);
		creationFlag = creationFlags;
		shellExecutable = shellCommand;
	}

	~ShellProcessManager() {
		StopReadingThreads();
	}

	bool Initialize();
	void stdinWrite(std::string str);
	std::map<int, std::string> readStdOut();
	std::map<int, std::string> readStdErr();
	void StartReadingThreads();
	void StopReadingThreads();

private:
	STARTUPINFO siStartInfo;
	std::string shellExecutable;
	DWORD creationFlag;
	HANDLE g_hChildStd_IN_Rd = NULL;
	HANDLE g_hChildStd_IN_Wr = NULL;
	HANDLE g_hChildStd_OUT_Rd = NULL;
	HANDLE g_hChildStd_OUT_Wr = NULL;
	HANDLE g_hChildStd_ERR_Rd = NULL;
	HANDLE g_hChildStd_ERR_Wr = NULL;

	bool continue_reading_stdout;
	bool continue_reading_stderr;
	phmap::node_hash_map<int, std::string> strmap_out;
	phmap::node_hash_map<int, std::string> strmap_err;
	std::thread t1;
	std::thread t2;

	static std::string ws2s(const std::wstring& str);
	static std::wstring s2ws(const std::string& str);
	std::wstring ReadFromPipe(HANDLE pipeHandle);
	void ReadFromStdOut();
	void ReadFromStdErr();
	void CloseHandles();
};

std::string ShellProcessManager::ws2s(const std::wstring& str) {
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &str[0], (int)str.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

std::wstring ShellProcessManager::s2ws(const std::string& str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

bool ShellProcessManager::Initialize() {
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
		return false;
	if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
		return false;
	if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
		return false;
	if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
		return false;
	if (!CreatePipe(&g_hChildStd_ERR_Rd, &g_hChildStd_ERR_Wr, &saAttr, 0))
		return false;
	if (!SetHandleInformation(g_hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0))
		return false;

	siStartInfo.hStdError = g_hChildStd_ERR_Wr;
	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartInfo.hStdInput = g_hChildStd_IN_Rd;

	std::wstring mycmd = s2ws(shellExecutable);
	TCHAR mychararray[1024 * 32] = { 0 };
	for (int i = 0; i < mycmd.size(); i++) {
		mychararray[i] = mycmd.c_str()[i];
	}

	PROCESS_INFORMATION piProcInfo;
	ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	auto myproc = CreateProcess(NULL, mychararray, NULL, NULL, TRUE, creationFlag, NULL, NULL, &siStartInfo, &piProcInfo);
	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);
	CloseHandle(g_hChildStd_OUT_Wr);
	CloseHandle(g_hChildStd_IN_Rd);
	return myproc;
}

void ShellProcessManager::stdinWrite(std::string str) {
	DWORD dwWritten;
	//std::string s = ws2s(str);
	str.append("\n");
	WriteFile(g_hChildStd_IN_Wr, str.c_str(), str.size(), &dwWritten, NULL);

}

std::wstring ShellProcessManager::ReadFromPipe(HANDLE pipeHandle) {
	DWORD dwRead;
	CHAR chBuf[4096] = { 0 };
	ReadFile(pipeHandle, chBuf, 4096, &dwRead, NULL);
	return s2ws(chBuf);
}

void ShellProcessManager::ReadFromStdOut() {
	int myco = 0;
	std::wstring m;
	while (continue_reading_stdout) {
		m.append(ReadFromPipe(g_hChildStd_OUT_Rd));
		if (continue_reading_stdout) {
			strmap_out.emplace(myco++, ws2s(m));
			m.clear();
		}
		else { break; }
	}
}

void ShellProcessManager::ReadFromStdErr() {
	int myco = 0;
	std::wstring m;
	while (continue_reading_stderr) {
		m.append( ReadFromPipe(g_hChildStd_ERR_Rd));
		if (continue_reading_stderr) {
			strmap_err.emplace(myco++, ws2s(m));
			m.clear();
		}
		else { break; }
	}
}

void ShellProcessManager::StartReadingThreads() {
	t1 = std::thread(&ShellProcessManager::ReadFromStdOut, this);
	t2 = std::thread(&ShellProcessManager::ReadFromStdErr, this);
}

void ShellProcessManager::StopReadingThreads() {
	continue_reading_stdout = false;
	continue_reading_stderr = false;
	stdinWrite(">&2 echo done stderr\n");
	stdinWrite("echo done stdout\n");
	Sleep(10);
	CloseHandles();
	try {
		TerminateThread(t1.native_handle(), 1);
		t1.detach();
		t1.join();
	}
	catch (...) {
		std::cout << "" << std::endl;
	}
	try {
		TerminateThread(t2.native_handle(), 1);
		t2.detach();
		t2.join();
	}
	catch (...) {
		std::cout << "" << std::endl;
	}
}

std::map<int, std::string> ShellProcessManager::readStdOut() {
	std::map<int, std::string> v;
	while (strmap_out.begin() != strmap_out.end()) {
		int tempint = strmap_out.begin()->first;
		std::string tmpstring = strmap_out.begin()->second;
		v[tempint] = tmpstring;
		strmap_out.erase(strmap_out.begin());

	}
	return v;
}
std::map<int, std::string> ShellProcessManager::readStdErr() {
	std::map<int, std::string> v;
	while (strmap_err.begin() != strmap_err.end()) {
		int tempint = strmap_err.begin()->first;
		std::string tmpstring = strmap_err.begin()->second;
		v[tempint] = tmpstring;
		strmap_err.erase(strmap_err.begin());

	}
	return v;
}


void ShellProcessManager::CloseHandles() {
	CloseHandle(g_hChildStd_OUT_Rd);
	CloseHandle(g_hChildStd_ERR_Rd);
	CloseHandle(g_hChildStd_IN_Wr);
}

#endif // SHELL_PROCESS_MANAGER_H
