#include <argparse/argparse.hpp>

#include <ntdll.h>

void inject(const std::string& dll, const std::string& exe, const std::vector<std::string>& args)
{
    STARTUPINFO si{ .cb = sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi{};

    std::string cmdline;

    for (const auto& arg : args)
    {
        cmdline += arg + " ";
    }

    if (!CreateProcess(exe.c_str(), cmdline.empty() ? nullptr : cmdline.data(), nullptr, nullptr, false, CREATE_SUSPENDED | DETACHED_PROCESS, nullptr, nullptr, &si, &pi))
    {
        std::printf("[-] Failed to start process. error: %d", GetLastError());
        std::exit(1);
    }

    std::printf("[+] Process created. PID: %u\n", pi.dwProcessId);

    if (auto memory = VirtualAllocEx(pi.hProcess, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))
    {
        std::printf("[+] remote buffer: 0x%p\n", memory);
        if (WriteProcessMemory(pi.hProcess, memory, dll.c_str(), dll.size(), nullptr))
        {
            if (auto thread = CreateRemoteThread(pi.hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryA), memory, 0, nullptr))
            {
                WaitForSingleObject(thread, INFINITE);
                CloseHandle(thread);
                VirtualFreeEx(pi.hProcess, memory, 0x1000, MEM_RELEASE);
            }
            else
            {
                std::printf("[-] Failed to create remote thread. error:", GetLastError());
            }
        }
        else
        {
            std::printf("[-] Failed to write dll path into remote process. error:", GetLastError());
        }
    }
    else
    {
        std::printf("[-] Failed to allocate memory in remote process. error:", GetLastError());
    }
    ResumeThread(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

int main(int argc, char** argv)
{
    argparse::ArgumentParser program("api-tracer: trace.dll Loader");

    program.add_argument("-e", "--exe")
        .help("Target executable to trace")
        .default_value<std::string>("")
        .required();

    program.add_argument("-t", "--tracer")
        .help("Path to tracer.dll")
        .default_value<std::string>("");

    program.add_argument("args")
        .remaining()
        .help("Arguments to pass to target executable")
        .default_value<std::vector<std::string>>({});

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        std::exit(1);
    }

    const auto tracer = program.get<std::string>("--tracer");
    const auto exe    = program.get<std::string>("--exe");
    const auto args   = program.get<std::vector<std::string>>("args");

    std::cout << tracer << std::endl;
    std::cout << exe << std::endl;
    for (const auto& arg : args)
    {
        std::cout << arg << std::endl;
    }

    inject(tracer.empty() ? "tracer.dll" : tracer, exe, args);

    return 0;
}
