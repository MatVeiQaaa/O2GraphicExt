#include <windows.h>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

static void Log(const std::string format, ...) 
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    WORD originalAttributes = consoleInfo.wAttributes;
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
    printf("[+] ");
    SetConsoleTextAttribute(hConsole, originalAttributes);
    va_list args;
    va_start(args, format);
    vprintf(format.c_str(), args);
    va_end(args);
    printf("\n");
    fflush(stdout);
}

static void Logger(std::string buffer)
{
    std::ofstream logFile;
    logFile.open("O2GraphicExt.log", std::ios_base::app);
    std::time_t timeStamp = std::time(NULL);
    logFile << std::put_time(std::localtime(&timeStamp), "%D %T ") << buffer;
    logFile << std::endl << std::endl;
    logFile.close();
}

static BOOL CanAccess(uintptr_t ptr)
{
    LPVOID addressToCheck = reinterpret_cast<LPVOID>(ptr);
    MEMORY_BASIC_INFORMATION memInfo;
    SIZE_T memInfoSize = sizeof(memInfo);
    SIZE_T queryResult = VirtualQuery(addressToCheck, &memInfo, memInfoSize);

    if (queryResult == 0) {
        return 0;
    }

    if (memInfo.State == MEM_COMMIT && (memInfo.Protect == PAGE_READWRITE || memInfo.Protect == PAGE_EXECUTE_READWRITE || 
                                        memInfo.Protect == PAGE_WRITECOPY || memInfo.Protect == PAGE_EXECUTE_WRITECOPY)) {
        return 1;
    }
    return 0;
}

static uintptr_t FollowPointers(uintptr_t base, std::vector<int> offsets)
{
    uintptr_t addr = base;
    for (int i = 0; i < offsets.size() - 1; i++) {
        addr += offsets[i];
        if (!CanAccess(addr)) {
            return NULL;
        }
        addr = *(uintptr_t*)addr;
    }
    addr += offsets[offsets.size() - 1];
    return addr;
}
