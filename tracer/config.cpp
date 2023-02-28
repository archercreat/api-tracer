#include "config.hpp"

#include <fstream>

std::vector<std::string> read_lines(const char* filepath)
{
    std::vector<std::string> lines;
    if (std::ifstream inf(filepath, std::ios::in); inf.is_open())
    {
        std::string line;
        while(std::getline(inf, line))
        {
            lines.push_back(line);
        }
    }
    return lines;
}

std::vector<std::string> load_filters()
{
    return read_lines(file_filter);
}

std::vector<std::string> load_hooks()
{
    return read_lines(file_hooks);
}
