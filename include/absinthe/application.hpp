#pragma once

namespace absinthe
{
    void ShowHelp(const char* argv0);

    class Application
    {
    public:
        int Run(int argc, char* argv[]);
    };
}
