/*
 * ? Author: LWT
 */

#ifndef NETWORKUTILS_HPP
#define NETWORKUTILS_HPP

#include "buffer_manager.h"
#include "char.h"
#include "desc.h"
#include "packet.h"

namespace networkutils
{
template <typename T>
concept HeaderPacketConcept = requires(T t) {
    {
        t.header
    } -> std::convertible_to<uint8_t>;
    {
        t.sub_header
    } -> std::convertible_to<uint8_t>;
};

template <typename HeaderPacket = DynamicPacketInfo>
    requires HeaderPacketConcept<HeaderPacket>
class DynamicPacketBuilder
{
public:
    template <typename T>
    DynamicPacketBuilder& add_header(const uint8_t header, const T sub_header)
    {
        m_header_packet.header = header;
        m_header_packet.sub_header = static_cast<uint8_t>(sub_header);

        m_buffer.write(m_header_packet);

        return *this;
    }

    template <typename T>
    DynamicPacketBuilder& add_payload(T&& payload)
    {
        m_buffer.write(std::forward<T>(payload));

        return *this;
    }

    void send_to_client(const LPCHARACTER p_character)
    {
        const auto p_desc{p_character->GetDesc()};

        p_desc->Packet(m_buffer.read_peek(), m_buffer.size());

        // breach single responsibility
        clear_buffer();
    }

    void clear_buffer()
    {
        m_buffer.reset();
    }

private:
    HeaderPacket m_header_packet{};

    TEMP_BUFFER m_buffer;
};

} // namespace networkutils

#endif // NETWORKUTILS_HPP
