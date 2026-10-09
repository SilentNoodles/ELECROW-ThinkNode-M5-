#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

// Dependency-free line formatter for the FCU serial mirror; see docs/nctnl-fcu-mirror-spec.md.
namespace NctnlFcuLine
{

constexpr size_t base64Len(size_t n)
{
    return ((n + 2) / 3) * 4;
}

// "@NCTNL1 X " + seq + " " + payload + " *" + crc + "\r\n" + NUL
constexpr size_t lineLen(size_t payloadLen)
{
    return 10 + 10 + 1 + payloadLen + 2 + 8 + 2 + 1;
}

// CRC-32/IEEE, identical to zlib.crc32
inline uint32_t crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc >> 1) ^ (0xEDB88320 & (0 - (crc & 1)));
    }
    return ~crc;
}

inline size_t base64Encode(const uint8_t *in, size_t len, char *out)
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t o = 0;
    for (size_t i = 0; i < len; i += 3) {
        uint32_t v = (uint32_t)in[i] << 16;
        if (i + 1 < len)
            v |= (uint32_t)in[i + 1] << 8;
        if (i + 2 < len)
            v |= in[i + 2];
        out[o++] = alphabet[(v >> 18) & 0x3F];
        out[o++] = alphabet[(v >> 12) & 0x3F];
        out[o++] = i + 1 < len ? alphabet[(v >> 6) & 0x3F] : '=';
        out[o++] = i + 2 < len ? alphabet[v & 0x3F] : '=';
    }
    return o;
}

// Builds a complete line into out; returns its length (excluding NUL) or 0 if out is too small.
inline size_t formatLine(char type, uint32_t seq, const uint8_t *payload, size_t payloadLen, bool asBase64, char *out,
                         size_t outSize)
{
    const size_t encodedLen = asBase64 ? base64Len(payloadLen) : payloadLen;
    if (outSize < lineLen(encodedLen))
        return 0;

    size_t n = (size_t)snprintf(out, outSize, "@NCTNL1 %c %lu ", type, (unsigned long)seq);
    if (asBase64) {
        n += base64Encode(payload, payloadLen, out + n);
    } else {
        for (size_t i = 0; i < payloadLen; i++)
            out[n++] = (char)payload[i];
    }
    out[n++] = ' ';
    const uint32_t crc = crc32((const uint8_t *)out, n);
    n += (size_t)snprintf(out + n, outSize - n, "*%08lx\r\n", (unsigned long)crc);
    return n;
}

} // namespace NctnlFcuLine
