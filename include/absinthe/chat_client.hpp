#pragma once

#include <deque>
#include <mutex>
#include <string>

#include "botcraft/AI/TemplatedBehaviourClient.hpp"
#include "protocolCraft/BinaryReadWrite.hpp"

namespace absinthe
{
    struct ChatMessage
    {
        ProtocolCraft::UUID sender{};
        std::string sender_name;
        std::string content;
        bool has_signature = false;
        bool secure_chat_enforced = false;
    };

    class ChatBehaviourClient : public Botcraft::TemplatedBehaviourClient<ChatBehaviourClient>
    {
    public:
        explicit ChatBehaviourClient(bool use_renderer);

        bool PopChatMessage(ChatMessage& message);
        bool IsSecureChatEnforced() const;

    protected:
        using Botcraft::TemplatedBehaviourClient<ChatBehaviourClient>::Handle;
        void Handle(ProtocolCraft::ClientboundLoginPacket& packet) override;
#if PROTOCOL_VERSION > 758 /* > 1.18.2 */
        void Handle(ProtocolCraft::ClientboundPlayerChatPacket& packet) override;
#endif

    private:
        mutable std::mutex chat_mutex;
        std::deque<ChatMessage> chat_messages;
        bool secure_chat_enforced = false;
    };
}
