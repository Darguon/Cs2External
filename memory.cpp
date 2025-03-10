#include "Memory.h"

Memory g_Memory;

Memory::Memory() : m_ProcessId(0), m_hProcess(NULL), m_ModuleBase(0)
{
}

Memory::~Memory()
{
    Cleanup();
}

bool Memory::Initialize(const wchar_t* processName, const wchar_t* moduleName)
{
    Cleanup(); // Cleanup any existing handles

    // Find process ID
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(snapshot, &entry))
    {
        do
        {
            if (_wcsicmp(entry.szExeFile, processName) == 0)
            {
                m_ProcessId = entry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);

    if (m_ProcessId == 0)
    {
        std::wcerr << L"Process " << processName << L" not found." << std::endl;
        return false;
    }

    // Open process handle
    m_hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, m_ProcessId);
    if (!m_hProcess)
    {
        std::cerr << "Failed to open process. Error code: " << GetLastError() << std::endl;
        std::cerr << "Make sure you're running as Administrator." << std::endl;
        m_ProcessId = 0;
        return false;
    }

    // Find module base address
    MODULEENTRY32 moduleEntry;
    moduleEntry.dwSize = sizeof(MODULEENTRY32);

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_ProcessId);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to create module snapshot. Error code: " << GetLastError() << std::endl;
        CloseHandle(m_hProcess);
        m_hProcess = NULL;
        m_ProcessId = 0;
        return false;
    }

    if (Module32First(snapshot, &moduleEntry))
    {
        do
        {
            if (_wcsicmp(moduleEntry.szModule, moduleName) == 0)
            {
                m_ModuleBase = reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr);
                break;
            }
        } while (Module32Next(snapshot, &moduleEntry));
    }
    CloseHandle(snapshot);

    if (m_ModuleBase == 0)
    {
        std::wcerr << L"Module " << moduleName << L" not found." << std::endl;
        CloseHandle(m_hProcess);
        m_hProcess = NULL;
        m_ProcessId = 0;
        return false;
    }

    std::wcout << L"Successfully attached to process " << processName << L" (ID: " << m_ProcessId << L")" << std::endl;
    std::wcout << L"Module " << moduleName << L" found at: 0x" << std::hex << m_ModuleBase << std::dec << std::endl;

    return true;
}

bool Memory::IsValid() const
{
    return (m_ProcessId != 0 && m_hProcess != NULL && m_ModuleBase != 0);
}

void Memory::Cleanup()
{
    if (m_hProcess)
    {
        CloseHandle(m_hProcess);
        m_hProcess = NULL;
    }
    m_ProcessId = 0;
    m_ModuleBase = 0;
}

DWORD Memory::GetProcessId() const
{
    return m_ProcessId;
}

HANDLE Memory::GetProcessHandle() const
{
    return m_hProcess;
}

uintptr_t Memory::GetModuleBase() const
{
    return m_ModuleBase;
}

bool Memory::ReadString(uintptr_t address, std::string& outString, size_t maxLength)
{
    outString.clear();
    std::vector<char> buffer(maxLength + 1, 0);

    if (!ReadProcessMemory(m_hProcess, reinterpret_cast<LPCVOID>(address), buffer.data(), maxLength, nullptr))
    {
        return false;
    }

    buffer[maxLength] = '\0';
    outString = buffer.data();
    return true;
}

bool Memory::ReadWideString(uintptr_t address, std::wstring& outString, size_t maxLength)
{
    outString.clear();
    std::vector<wchar_t> buffer(maxLength + 1, 0);

    if (!ReadProcessMemory(m_hProcess, reinterpret_cast<LPCVOID>(address), buffer.data(), maxLength * sizeof(wchar_t), nullptr))
    {
        return false;
    }

    buffer[maxLength] = L'\0';
    outString = buffer.data();
    return true;
}