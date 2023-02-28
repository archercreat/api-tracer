#include "logger.hpp"
#include "config.hpp"
#include "printf.h"

#include <ntdll.h>
#include <asmjit/asmjit.h>

static asmjit::JitRuntime rt;
static asmjit::CodeHolder code;

typedef decltype(NtWriteFile) write;

logger::logger()
{
    file = CreateFile(file_trace, FILE_GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        throw -1;
    }

    // Get NtWriteFile syscall number.
    // mov r10, rcx
    // mov eax, ...
    //
    if (*reinterpret_cast<uint32_t*>(NtWriteFile) != 0xB8D18B4C)
    {
        throw -1;
    }
    uint32_t number = reinterpret_cast<uint32_t*>(NtWriteFile)[1];

    code.init(rt.environment(), rt.cpuFeatures());

    auto jit = asmjit::x86::Assembler(&code);
    // Craft NtWriteFile system call stub.
    //
    jit.mov(asmjit::x86::r10, asmjit::x86::rcx);
    jit.mov(asmjit::x86::rax, number);
    jit.syscall();
    jit.ret();

    rt.add(&syscall, &code);
}

logger::~logger()
{
    CloseHandle(file);
}

void logger::log(const char* format, ...) noexcept
{
    char buff[1024];
    va_list args;
    va_start(args, format);

    auto len = vsnprintf_(buff, sizeof(buff), format, args);

    va_end(args);

    IO_STATUS_BLOCK isb;
    static_cast<write*>(syscall)(file, nullptr, nullptr, nullptr, &isb, buff, len, nullptr, nullptr);
}

extern "C" void _putchar(char) {}
