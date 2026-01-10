#include "absinthe/chat_client.hpp"

#include "botcraft/Utilities/Logger.hpp"
#include "protocolCraft/Packets/Game/Clientbound/ClientboundLoginPacket.hpp"
#include "protocolCraft/Packets/Game/Clientbound/ClientboundPlayerChatPacket.hpp"

namespace absinthe
{
    ChatBehaviourClient::ChatBehaviourClient(bool use_renderer)
        : Botcraft::TemplatedBehaviourClient<ChatBehaviourClient>(use_renderer)
    {
    }

    bool ChatBehaviourClient::PopChatMessage(ChatMessage& message)
    {
        std::lock_guard<std::mutex> lock(chat_mutex);
        if (chat_messages.empty())
        {
            return false;
        }
        message = std::move(chat_messages.front());
        chat_messages.pop_front();
        return true;
    }

    bool ChatBehaviourClient::IsSecureChatEnforced() const
    {
        return secure_chat_enforced;
    }

    void ChatBehaviourClient::Handle(ProtocolCraft::ClientboundLoginPacket& packet)
    {
        Botcraft::ManagersClient::Handle(packet);
#if PROTOCOL_VERSION > 765 /* > 1.20.4 */
        secure_chat_enforced = packet.GetEnforceSecureChat();
        LOG_INFO("Server enforce secure chat: " << (secure_chat_enforced ? "true" : "false"));
#else
        secure_chat_enforced = false;
#endif
    }

#if PROTOCOL_VERSION > 758 /* > 1.18.2 */
    void ChatBehaviourClient::Handle(ProtocolCraft::ClientboundPlayerChatPacket& packet)
    {
        ChatMessage message;
#if PROTOCOL_VERSION > 760 /* > 1.19.2 */
        message.sender = packet.GetSender();
        message.has_signature = packet.GetSignature().has_value();
        if (packet.GetUnsignedContent().has_value())
        {
            message.content = packet.GetUnsignedContent()->GetText();
        }
        else
        {
            message.content = packet.GetBody().GetContent();
        }
#else
        (void)packet;
        return;
#endif

        message.secure_chat_enforced = secure_chat_enforced;
        message.sender_name = GetPlayerName(message.sender);
        if (message.sender_name.empty())
        {
            message.sender_name = "unknown";
        }

        if (message.content.empty())
        {
            return;
        }

        std::lock_guard<std::mutex> lock(chat_mutex);
        chat_messages.push_back(std::move(message));
    }
#endif
}
