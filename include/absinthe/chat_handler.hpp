#pragma once

#include <optional>
#include <string>
#include <vector>

namespace absinthe
{
    struct ChatCommand
    {
        std::string name;
        std::vector<std::string> args;
    };

    struct ChatParseResult
    {
        bool is_command = false;
        bool ok = false;
        std::string error;
        ChatCommand command;
    };

    class ChatHandler
    {
    public:
        explicit ChatHandler(std::string prefix = "?");

        const std::string& GetPrefix() const;
        ChatParseResult Parse(const std::string& message) const;
        std::optional<std::string> HandleCommand(const ChatCommand& command) const;
        std::string FormatHelp() const;

    private:
        std::string prefix_;
    };
}
