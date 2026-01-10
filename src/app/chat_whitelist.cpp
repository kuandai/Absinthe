#include "absinthe/chat_whitelist.hpp"

#include <algorithm>
#include <cctype>

namespace absinthe
{
    namespace
    {
        int HexValue(const char c)
        {
            if (c >= '0' && c <= '9')
            {
                return c - '0';
            }
            if (c >= 'a' && c <= 'f')
            {
                return 10 + (c - 'a');
            }
            if (c >= 'A' && c <= 'F')
            {
                return 10 + (c - 'A');
            }
            return -1;
        }
    }

    bool ChatWhitelist::AddEntry(const std::string& entry)
    {
        if (entry.empty())
        {
            return false;
        }

        const std::optional<ProtocolCraft::UUID> uuid = ParseUuid(entry);
        if (uuid.has_value())
        {
            for (const auto& existing : allowed_uuids)
            {
                if (existing == uuid.value())
                {
                    return false;
                }
            }
            allowed_uuids.push_back(uuid.value());
            return true;
        }

        const std::string normalized = NormalizeName(entry);
        if (!normalized.empty())
        {
            for (const auto& existing : allowed_names)
            {
                if (existing == normalized)
                {
                    return false;
                }
            }
            allowed_names.push_back(normalized);
            return true;
        }
        return false;
    }

    bool ChatWhitelist::RemoveEntry(const std::string& entry)
    {
        if (entry.empty())
        {
            return false;
        }

        const std::optional<ProtocolCraft::UUID> uuid = ParseUuid(entry);
        if (uuid.has_value())
        {
            const auto it = std::find(allowed_uuids.begin(), allowed_uuids.end(), uuid.value());
            if (it != allowed_uuids.end())
            {
                allowed_uuids.erase(it);
                return true;
            }
            return false;
        }

        const std::string normalized = NormalizeName(entry);
        if (!normalized.empty())
        {
            const auto it = std::find(allowed_names.begin(), allowed_names.end(), normalized);
            if (it != allowed_names.end())
            {
                allowed_names.erase(it);
                return true;
            }
        }

        return false;
    }

    bool ChatWhitelist::IsEmpty() const
    {
        return allowed_uuids.empty() && allowed_names.empty();
    }

    bool ChatWhitelist::IsAllowed(const ChatMessage& message) const
    {
        if (IsEmpty())
        {
            return false;
        }

        for (const auto& allowed_uuid : allowed_uuids)
        {
            if (allowed_uuid == message.sender)
            {
                return true;
            }
        }

        const std::string normalized = NormalizeName(message.sender_name);
        for (const auto& allowed_name : allowed_names)
        {
            if (allowed_name == normalized)
            {
                return true;
            }
        }

        return false;
    }

    std::string ChatWhitelist::FormatEntries() const
    {
        if (IsEmpty())
        {
            return "Allowlist is empty.";
        }

        std::string output = "Allowlist: ";
        bool first = true;
        for (const auto& name : allowed_names)
        {
            if (!first)
            {
                output += ", ";
            }
            output += name;
            first = false;
        }
        for (const auto& uuid : allowed_uuids)
        {
            if (!first)
            {
                output += ", ";
            }
            output += FormatUuid(uuid);
            first = false;
        }
        return output;
    }

    std::optional<ProtocolCraft::UUID> ChatWhitelist::ParseUuid(const std::string& value)
    {
        std::string hex;
        hex.reserve(value.size());
        for (const char c : value)
        {
            if (c == '-')
            {
                continue;
            }
            if (HexValue(c) < 0)
            {
                return std::nullopt;
            }
            hex.push_back(c);
        }

        if (hex.size() != 32)
        {
            return std::nullopt;
        }

        ProtocolCraft::UUID uuid{};
        for (size_t i = 0; i < uuid.size(); ++i)
        {
            const int high = HexValue(hex[i * 2]);
            const int low = HexValue(hex[i * 2 + 1]);
            if (high < 0 || low < 0)
            {
                return std::nullopt;
            }
            uuid[i] = static_cast<unsigned char>((high << 4) | low);
        }
        return uuid;
    }

    std::string ChatWhitelist::NormalizeName(const std::string& value)
    {
        std::string output;
        output.reserve(value.size());
        for (const char c : value)
        {
            output.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
        return output;
    }

    std::string ChatWhitelist::FormatUuid(const ProtocolCraft::UUID& uuid)
    {
        static constexpr char kHex[] = "0123456789abcdef";
        std::string output;
        output.reserve(36);
        for (size_t i = 0; i < uuid.size(); ++i)
        {
            if (i == 4 || i == 6 || i == 8 || i == 10)
            {
                output.push_back('-');
            }
            output.push_back(kHex[(uuid[i] >> 4) & 0xF]);
            output.push_back(kHex[uuid[i] & 0xF]);
        }
        return output;
    }
}
