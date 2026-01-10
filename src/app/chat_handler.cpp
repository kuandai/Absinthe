#include "absinthe/chat_handler.hpp"

#include <cctype>
#include <sstream>

namespace absinthe
{
    namespace
    {
        bool StartsWith(const std::string& value, const std::string& prefix)
        {
            return value.rfind(prefix, 0) == 0;
        }

        void TrimLeft(std::string& value)
        {
            size_t start = 0;
            while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0)
            {
                ++start;
            }
            value.erase(0, start);
        }

        std::vector<std::string> SplitWords(const std::string& value)
        {
            std::vector<std::string> words;
            std::istringstream stream(value);
            std::string token;
            while (stream >> token)
            {
                words.push_back(token);
            }
            return words;
        }

        std::string Join(const std::vector<std::string>& parts, const size_t start_index)
        {
            std::ostringstream stream;
            for (size_t i = start_index; i < parts.size(); ++i)
            {
                if (i > start_index)
                {
                    stream << ' ';
                }
                stream << parts[i];
            }
            return stream.str();
        }
    }

    ChatHandler::ChatHandler(std::string prefix)
        : prefix_(std::move(prefix))
    {
    }

    const std::string& ChatHandler::GetPrefix() const
    {
        return prefix_;
    }

    ChatParseResult ChatHandler::Parse(const std::string& message) const
    {
        ChatParseResult result;
        if (!StartsWith(message, prefix_))
        {
            return result;
        }

        result.is_command = true;
        std::string rest = message.substr(prefix_.size());
        TrimLeft(rest);
        if (rest.empty())
        {
            result.error = "Malformed command. Usage: " + prefix_ + " <command> [args]. Try \"" + prefix_ + " help\".";
            return result;
        }

        const std::vector<std::string> words = SplitWords(rest);
        if (words.empty())
        {
            result.error = "Malformed command. Usage: " + prefix_ + " <command> [args]. Try \"" + prefix_ + " help\".";
            return result;
        }

        result.ok = true;
        result.command.name = words.front();
        if (words.size() > 1)
        {
            result.command.args.assign(words.begin() + 1, words.end());
        }
        return result;
    }

    std::optional<std::string> ChatHandler::HandleCommand(const ChatCommand& command) const
    {
        if (command.name == "help")
        {
            return FormatHelp();
        }

        if (command.name == "ping")
        {
            return std::string("pong");
        }

        if (command.name == "echo")
        {
            if (command.args.empty())
            {
                return std::string("Malformed command. Usage: ") + prefix_ + " echo <text>.";
            }
            return Join(command.args, 0);
        }

        return std::string("Unknown command \"") + command.name + "\". Try \"" + prefix_ + " help\".";
    }

    std::string ChatHandler::FormatHelp() const
    {
        return "Commands: " + prefix_ + "help, " + prefix_ + "ping, " + prefix_ + "echo <text>, "
            + prefix_ + "allow <name|uuid>, " + prefix_ + "deny <name|uuid>, " + prefix_ + "list";
    }
}
