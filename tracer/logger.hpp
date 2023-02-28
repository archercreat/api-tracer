#pragma once

struct logger
{
    static logger& get()
    {
        static logger log;
        return log;
    }

    logger(logger const&)         = delete;
    void operator=(logger const&) = delete;

    void log(const char* format, ...) noexcept;

private:
    logger();
    ~logger();

    void* file;
    void* syscall;
};
