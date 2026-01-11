#pragma once

#include <optional>
#include <string>
#include <vector>

#include "absinthe/chat_client.hpp"

namespace absinthe
{
    class ChatWhitelist
    {
    public:
        bool AddEntry(const std::string& entry);
        bool RemoveEntry(const std::string& entry);
        bool IsEmpty() const;
        bool LoadFromFile(const std::string& path, std::string* error = nullptr);
        bool SaveToFile(const std::string& path, std::string* error = nullptr) const;
        bool IsAllowed(const ChatMessage& message) const;
        std::string FormatEntries() const;

    private:
        static std::optional<ProtocolCraft::UUID> ParseUuid(const std::string& value);
        static std::string NormalizeName(const std::string& value);
        static std::string FormatUuid(const ProtocolCraft::UUID& uuid);

        std::vector<ProtocolCraft::UUID> allowed_uuids;
        std::vector<std::string> allowed_names;
    };
}
