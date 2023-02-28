#include "tracer.hpp"
#include "logger.hpp"

#include <ntdll.h>

BOOL DllMain(HINSTANCE dll, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        logger::get().log("api-tracer loading...\n");
        return tracer::initialize();
    }
    case DLL_PROCESS_DETACH:
    {
        logger::get().log("api-tracer unloading...\n");
        tracer::terminate();
        break;
    }
    default:
        break;
    }
    return TRUE;
}
