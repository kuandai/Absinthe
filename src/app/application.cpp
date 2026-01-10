#include "absinthe/application.hpp"

#include <iostream>
#include <string>

#include "botcraft/AI/BehaviourTree.hpp"
#include "botcraft/AI/SimpleBehaviourClient.hpp"
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

                LOG_FATAL("Unknown argument: " << arg);
                args.return_code = 1;
                return args;
            }
            return args;
        }
    }

    void ShowHelp(const char* argv0)
    {
        std::cout << "Usage: " << argv0 << " <options>\n"
            << "Options:\n"
            << "\t-h, --help\tShow this help message\n"
            << "\t--address\tAddress of the server you want to connect to, default: 127.0.0.1:25565\n"
            << "\t--login [name]\tPlayer name in offline mode, omit/empty for Microsoft account, default: BCHelloWorld\n"
            << std::endl;
    }

    int Application::Run(int argc, char* argv[])
    {
        Args args = ParseCommandLine(argc, argv);
        if (args.return_code != 0)
        {
            return args.return_code;
        }

        auto behaviour_tree = Botcraft::Builder<Botcraft::SimpleBehaviourClient>("startup")
            .sequence()
                .leaf("await play state", [](Botcraft::SimpleBehaviourClient& client) {
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
                .leaf("announce", [](Botcraft::SimpleBehaviourClient& client) {
                    client.SendChatMessage(u8"Hello, World!");
                    return Botcraft::Status::Success;
                })
                .leaf("cooldown", [](Botcraft::SimpleBehaviourClient& client) {
                    Botcraft::Utilities::SleepFor(std::chrono::seconds(2));
                    return Botcraft::Status::Success;
                })
                .leaf("shutdown", [](Botcraft::SimpleBehaviourClient& client) {
                    client.SetBehaviourTree(nullptr);
                    client.SetShouldBeClosed(true);
                    return Botcraft::Status::Success;
                })
            .end();

        Botcraft::SimpleBehaviourClient client(false);
        LOG_INFO("Starting connection process");
        client.Connect(args.address, args.login);
        client.SetBehaviourTree(behaviour_tree);

        client.RunBehaviourUntilClosed();
        client.Disconnect();
        return 0;
    }
}
