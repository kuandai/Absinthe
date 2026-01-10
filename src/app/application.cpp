#include "absinthe/application.hpp"
#include "absinthe/chat_client.hpp"
#include "absinthe/chat_handler.hpp"
#include "absinthe/chat_whitelist.hpp"

#include <cctype>
#include <deque>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "botcraft/AI/BehaviourTree.hpp"
#include "botcraft/Utilities/Logger.hpp"
#include "botcraft/Utilities/SleepUtilities.hpp"
#include "protocolCraft/enums.hpp"

namespace absinthe
{
    namespace
    {
        struct Args
        {
            std::string address = "127.0.0.1:25565";
            std::string login = "absinthe";
            std::vector<std::string> allow_list;
            int return_code = 0;
        };

        Args ParseCommandLine(int argc, char* argv[])
        {
            Args args;
            for (int i = 1; i < argc; ++i)
            {
                std::string arg = argv[i];
                if (arg == "--address")
                {
                    if (i + 1 < argc)
                    {
                        args.address = argv[++i];
                        continue;
                    }

                    LOG_FATAL("--address requires an argument");
                    args.return_code = 1;
                    return args;
                }
                if (arg == "--login")
                {
                    if (i + 1 < argc && argv[i + 1][0] != '-')
                    {
                        args.login = argv[++i];
                    }
                    else
                    {
                        args.login.clear();
                    }

                    continue;
                }
                if (arg == "--allow")
                {
                    if (i + 1 < argc && argv[i + 1][0] != '-')
                    {
                        args.allow_list.push_back(argv[++i]);
                        continue;
                    }

                    LOG_FATAL("--allow requires an argument");
                    args.return_code = 1;
                    return args;
                }

                LOG_FATAL("Unknown argument: " << arg);
                args.return_code = 1;
                return args;
            }
            return args;
        }

        std::string Trim(const std::string& value)
        {
            size_t start = 0;
            while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0)
            {
                ++start;
            }
            size_t end = value.size();
            while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
            {
                --end;
            }
            return value.substr(start, end - start);
        }
    }

    void ShowHelp(const char* argv0)
    {
        std::cout << "Usage: " << argv0 << " <options>\n"
            << "Options:\n"
            << "\t-h, --help\tShow this help message\n"
            << "\t--address\tAddress of the server you want to connect to, default: 127.0.0.1:25565\n"
            << "\t--login [name]\tPlayer name in offline mode, omit/empty for Microsoft account, default: absinthe\n"
            << "\t--allow <name|uuid>\tAllowlisted player name or UUID (repeatable)\n"
            << std::endl;
    }

    int Application::Run(int argc, char* argv[])
    {
        Args args = ParseCommandLine(argc, argv);
        if (args.return_code != 0)
        {
            return args.return_code;
        }

        ChatHandler chat_handler;
        ChatWhitelist whitelist;
        for (const auto& entry : args.allow_list)
        {
            whitelist.AddEntry(entry);
        }

        struct StdinQueue
        {
            std::mutex mutex;
            std::deque<std::string> lines;
        };

        auto stdin_queue = std::make_shared<StdinQueue>();
        std::thread stdin_thread([stdin_queue]() {
            std::string line;
            while (std::getline(std::cin, line))
            {
                std::lock_guard<std::mutex> lock(stdin_queue->mutex);
                stdin_queue->lines.push_back(line);
            }
        });
        stdin_thread.detach();
        auto behaviour_tree = Botcraft::Builder<ChatBehaviourClient>("startup")
            .sequence()
                .leaf("await play state", [](ChatBehaviourClient& client) {
                    const bool ready = Botcraft::Utilities::YieldForCondition([&]() {
                        const auto manager = client.GetNetworkManager();
                        return manager && manager->GetConnectionState() == ProtocolCraft::ConnectionState::Play;
                    }, client, 15000);

                    if (!ready)
                    {
                        LOG_ERROR("Timeout waiting for Play state");
                        client.SetShouldBeClosed(true);
                        return Botcraft::Status::Failure;
                    }

                    return Botcraft::Status::Success;
                })
                .repeater("chat loop", 0)
                    .leaf("chat handler", [&chat_handler, &whitelist, stdin_queue](ChatBehaviourClient& client) {
                        auto send_feedback = [&](const std::string& text, const bool from_console) {
                            if (from_console)
                            {
                                LOG_INFO(text);
                            }
                            else
                            {
                                client.SendChatMessage(text);
                            }
                        };

                        auto handle_command = [&](const ChatParseResult& parsed, const bool from_console, const ChatMessage* message) {
                            if (!parsed.is_command)
                            {
                                return;
                            }

                            if (!parsed.ok)
                            {
                                send_feedback(parsed.error, from_console);
                                return;
                            }

                            if (!from_console)
                            {
                                if (!message || !message->has_signature)
                                {
                                    send_feedback("Secure chat signature missing. Commands require signed chat.", false);
                                    return;
                                }

                                if (!whitelist.IsAllowed(*message))
                                {
                                    send_feedback("You are not authorized to issue commands.", false);
                                    return;
                                }
                            }

                            if (parsed.command.name == "allow")
                            {
                                if (parsed.command.args.empty())
                                {
                                    send_feedback("Malformed command. Usage: " + chat_handler.GetPrefix() + " allow <name|uuid>.", from_console);
                                    return;
                                }

                                int added = 0;
                                for (const auto& entry : parsed.command.args)
                                {
                                    if (whitelist.AddEntry(entry))
                                    {
                                        ++added;
                                    }
                                }
                                if (added == 0)
                                {
                                    send_feedback("No new entries added to allowlist.", from_console);
                                }
                                else
                                {
                                    send_feedback("Allowlist updated. Added " + std::to_string(added) + " entr"
                                        + (added == 1 ? "y." : "ies."), from_console);
                                }
                                return;
                            }

                            if (parsed.command.name == "deny")
                            {
                                if (parsed.command.args.empty())
                                {
                                    send_feedback("Malformed command. Usage: " + chat_handler.GetPrefix() + " deny <name|uuid>.", from_console);
                                    return;
                                }

                                int removed = 0;
                                for (const auto& entry : parsed.command.args)
                                {
                                    if (whitelist.RemoveEntry(entry))
                                    {
                                        ++removed;
                                    }
                                }
                                if (removed == 0)
                                {
                                    send_feedback("No matching entries found in allowlist.", from_console);
                                }
                                else
                                {
                                    send_feedback("Allowlist updated. Removed " + std::to_string(removed) + " entr"
                                        + (removed == 1 ? "y." : "ies."), from_console);
                                }
                                return;
                            }

                            if (parsed.command.name == "list")
                            {
                                send_feedback(whitelist.FormatEntries(), from_console);
                                return;
                            }

                            std::optional<std::string> response = chat_handler.HandleCommand(parsed.command);
                            if (response.has_value())
                            {
                                send_feedback(response.value(), from_console);
                            }
                        };

                        auto parse_console = [&](const std::string& line) {
                            std::string trimmed = Trim(line);
                            if (trimmed.empty())
                            {
                                return ChatParseResult{};
                            }

                            if (trimmed.rfind(chat_handler.GetPrefix(), 0) != 0)
                            {
                                trimmed = chat_handler.GetPrefix() + " " + trimmed;
                            }

                            return chat_handler.Parse(trimmed);
                        };

                        ChatMessage message;
                        while (client.PopChatMessage(message))
                        {
                            ChatParseResult parsed = chat_handler.Parse(message.content);
                            handle_command(parsed, false, &message);
                        }

                        std::deque<std::string> pending;
                        {
                            std::lock_guard<std::mutex> lock(stdin_queue->mutex);
                            pending.swap(stdin_queue->lines);
                        }

                        for (const auto& line : pending)
                        {
                            ChatParseResult parsed = parse_console(line);
                            handle_command(parsed, true, nullptr);
                        }

                        client.Yield();
                        return Botcraft::Status::Failure;
                    })
                .end();

        ChatBehaviourClient client(false);
        client.SetAutoRespawn(true);
        LOG_INFO("Starting connection process");
        client.Connect(args.address, args.login);
        client.SetBehaviourTree(behaviour_tree);

        client.RunBehaviourUntilClosed();
        client.Disconnect();
        return 0;
    }
}
