#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>
#include <vector>

class Memory
{
public:
    Memory();
    ~Memory();

    bool Initialize(const wchar_t* processName, const wchar_t* moduleName);
    bool IsValid() const;
    void Cleanup();

    DWORD GetProcessId() const;
    HANDLE GetProcessHandle() const;
    uintptr_t GetModuleBase() const;

    template<typename T>
    T Read(uintptr_t address)
    {
        T value{};
        if (!ReadProcessMemory(m_hProcess, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), nullptr))
        {
            //std::cerr << "Failed to read memory at 0x" << std::hex << address << std::dec << std::endl;
        }
        return value;
    }

    template<typename T>
    bool Write(uintptr_t address, const T& value)
    {
        return WriteProcessMemory(m_hProcess, reinterpret_cast<LPVOID>(address), &value, sizeof(T), nullptr);
    }

    template<typename T>
    std::vector<T> ReadArray(uintptr_t address, size_t count)
    {
        std::vector<T> array(count);
        if (!ReadProcessMemory(m_hProcess, reinterpret_cast<LPCVOID>(address), array.data(), sizeof(T) * count, nullptr))
        {
            //std::cerr << "Failed to read array at 0x" << std::hex << address << std::dec << std::endl;
            array.clear();
        }
        return array;
    }

    bool ReadString(uintptr_t address, std::string& outString, size_t maxLength = 256);
    bool ReadWideString(uintptr_t address, std::wstring& outString, size_t maxLength = 256);

private:
    DWORD m_ProcessId = 0;
    HANDLE m_hProcess = NULL;
    uintptr_t m_ModuleBase = 0;
};

extern Memory g_Memory;