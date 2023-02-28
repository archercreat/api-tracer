#include "tracer.hpp"
#include "logger.hpp"
#include "config.hpp"

#include <MinHook.h>
#include <asmjit/asmjit.h>

typedef void (*Func)(void);

namespace tracer
{
struct filter_t
{
    std::string name;
    uintptr_t   base;
    uintptr_t   size;
};
// Array that holds trampoline pointers.
//
static void* hooks[100000];
// Array of addresses to filter.
//
static std::vector<filter_t> filters;
// Array of hooked dlls.
//
static std::vector<std::string> dlls;
// Jit runtime that holds code.
//
static asmjit::JitRuntime rt;

/// NOTE: Make sure not to call any dlls in this function because that would cause the recursion.
/// The logger instance uses syscalls and custom vsprintf function to avoid that.
///
void logger_routine(const char* const api, const char* const dll, uintptr_t ret)
{
    // If no filters loaded, log everything.
    //
    if (filters.empty())
    {
        logger::get().log("0x%012llx %s:%s\n", ret, dll, api);
    }
    else
    {
        for (const auto& filter : filters)
        {
            if (ret >= filter.base && ret < filter.base + filter.size)
            {
                logger::get().log("%s+0x%06llx %s:%s\n", filter.name.c_str(), ret - filter.base, dll, api);
                break;
            }
        }
    }
}

bool setup_hooks(uint8_t* base, const char* dll_name)
{
    static uint64_t count;
    if (count >= 100000)
    {
        return false;
    }

    const auto nt_headers = PIMAGE_NT_HEADERS(base + PIMAGE_DOS_HEADER(base)->e_lfanew);
    const auto data_dir   = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!data_dir.VirtualAddress || !data_dir.Size)
        return false;

    const auto export_dir = PIMAGE_EXPORT_DIRECTORY(base + data_dir.VirtualAddress);
    const auto names      = reinterpret_cast<uint32_t*>(base + export_dir->AddressOfNames);
    const auto ordinals   = reinterpret_cast<uint16_t*>(base + export_dir->AddressOfNameOrdinals);
    const auto functions  = reinterpret_cast<uint32_t*>(base + export_dir->AddressOfFunctions);

    for (size_t i = 0; i < export_dir->NumberOfNames; i++)
    {
        asmjit::CodeHolder code;
        code.init(rt.environment(), rt.cpuFeatures());

        const auto api_name = reinterpret_cast<const char*>(base + names[i]);
        // Check for redirected exports.
        //
        const auto func_rva = functions[ordinals[i]];
        if (func_rva >= data_dir.VirtualAddress && func_rva < data_dir.VirtualAddress + data_dir.Size)
            continue;
        // Build function.
        //
        auto jit = asmjit::x86::Assembler(&code);
        // Save context.
        //
        jit.push(asmjit::x86::rax);
        jit.push(asmjit::x86::rbx);
        jit.push(asmjit::x86::rcx);
        jit.push(asmjit::x86::rdx);
        jit.push(asmjit::x86::rbp);
        jit.push(asmjit::x86::rdi);
        jit.push(asmjit::x86::rsi);
        jit.push(asmjit::x86::r8);
        jit.push(asmjit::x86::r9);
        jit.push(asmjit::x86::r10);
        jit.push(asmjit::x86::r11);
        jit.push(asmjit::x86::r12);
        jit.push(asmjit::x86::r13);
        jit.push(asmjit::x86::r14);
        jit.push(asmjit::x86::r15);
        // Get name of the called function and return address.
        //
        jit.mov(asmjit::x86::rcx, api_name);
        jit.mov(asmjit::x86::rdx, dll_name);
        jit.mov(asmjit::x86::r8, asmjit::x86::qword_ptr(asmjit::x86::rsp, 8 * 15));
        // Call logger.
        //
        jit.sub(asmjit::x86::rsp, 0x20);
        jit.call(logger_routine);
        jit.add(asmjit::x86::rsp, 0x20);
        // Restore context and exit.
        //
        jit.pop(asmjit::x86::r15);
        jit.pop(asmjit::x86::r14);
        jit.pop(asmjit::x86::r13);
        jit.pop(asmjit::x86::r12);
        jit.pop(asmjit::x86::r11);
        jit.pop(asmjit::x86::r10);
        jit.pop(asmjit::x86::r9);
        jit.pop(asmjit::x86::r8);
        jit.pop(asmjit::x86::rsi);
        jit.pop(asmjit::x86::rdi);
        jit.pop(asmjit::x86::rbp);
        jit.pop(asmjit::x86::rdx);
        jit.pop(asmjit::x86::rcx);
        jit.pop(asmjit::x86::rbx);
        jit.pop(asmjit::x86::rax);

        jit.mov(asmjit::x86::rax, asmjit::x86::qword_ptr(reinterpret_cast<uintptr_t>(hooks + count)));
        jit.jmp(asmjit::x86::rax);

        Func fn;
        if (auto err = rt.add(&fn, &code))
        {
            return false;
        }

        void* trampoline{};
        auto err = MH_CreateHook(static_cast<void*>(func_rva + base), fn, &trampoline);

        if (err == MH_OK)
        {
            hooks[count++] = trampoline;
        }
    }
    return true;
}

bool initialize()
{
    if (MH_Initialize() != MH_OK)
        return false;
    // Initialize hooks.
    //
    for (const auto& dll : load_hooks())
    {
        // Load dll.
        //
        if (auto base = LoadLibraryA(dll.c_str()))
        {
            // Hook every export.
            //
            if (!setup_hooks(reinterpret_cast<uint8_t*>(base), dll.c_str()))
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    // Initialize filters.
    //
    for (const auto& dll : load_filters())
    {
        const auto base = GetModuleHandle(dll.c_str());
        if (base != nullptr)
        {
            auto nt = PIMAGE_NT_HEADERS(reinterpret_cast<uint8_t*>(base) + PIMAGE_DOS_HEADER(base)->e_lfanew);
            filters.push_back({ dll, reinterpret_cast<uintptr_t>(base), nt->OptionalHeader.SizeOfImage });
        }
    }
    return MH_EnableHook(MH_ALL_HOOKS) == MH_OK;
}

void terminate()
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}
} // namespace hooks
