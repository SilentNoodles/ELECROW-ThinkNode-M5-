#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

// Dependency-free line formatter for the FCU serial mirror; see docs/nctnl-fcu-mirror-spec.md.
namespace NctnlFcuLine
{

constexpr size_t MAX_EXTRA_LEN = 10;

constexpr size_t base64Len(size_t n)
{
    return ((n + 2) / 3) * 4;
}

// "\r\n@NCTNL1 X " + seq + " " + extra + " " + base64 + " *" + crc + "\r\n" + NUL
constexpr size_t lineLen(size_t payloadLen, size_t extraLen = 0)
{
    return 2 + 10 + 10 + 1 + (extraLen ? extraLen + 1 : 0) + base64Len(payloadLen) + 2 + 8 + 2 + 1;
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

// Builds "\r\n" + a complete line into out; returns its length (excluding NUL) or 0 if it doesn't fit.
// extra may be null or empty; it must be printable ASCII without spaces.
inline size_t formatLine(char type, uint32_t seq, const char *extra, const uint8_t *payload, size_t payloadLen, char *out,
                         size_t outSize)
{
    size_t extraLen = 0;
    while (extra && extra[extraLen])
        extraLen++;
    if (extraLen > MAX_EXTRA_LEN || outSize < lineLen(payloadLen, extraLen))
        return 0;

    size_t n = (size_t)snprintf(out, outSize, "\r\n@NCTNL1 %c %lu ", type, (unsigned long)seq);
    for (size_t i = 0; i < extraLen; i++)
        out[n++] = extra[i];
    if (extraLen)
        out[n++] = ' ';
    n += base64Encode(payload, payloadLen, out + n);
    out[n++] = ' ';
    const uint32_t crc = crc32((const uint8_t *)out + 2, n - 2);
    n += (size_t)snprintf(out + n, outSize - n, "*%08lx\r\n", (unsigned long)crc);
    return n;
}

// Appends to a fixed buffer; ok goes false (and stays false) on overflow.
class JsonWriter
{
  public:
    JsonWriter(char *out, size_t size) : out(out), size(size) {}

    void raw(const char *s)
    {
        while (*s)
            put(*s++);
    }

    void number(long v)
    {
        char tmp[24];
        snprintf(tmp, sizeof(tmp), "%ld", v);
        raw(tmp);
    }

    void unsignedNumber(unsigned long v)
    {
        char tmp[24];
        snprintf(tmp, sizeof(tmp), "%lu", v);
        raw(tmp);
    }

    void string(const char *s)
    {
        put('"');
        for (; s && *s; s++) {
            const unsigned char c = (unsigned char)*s;
            if (c == '"' || c == '\\') {
                put('\\');
                put((char)c);
            } else if (c < 0x20) {
                char tmp[8];
                snprintf(tmp, sizeof(tmp), "\\u%04x", c);
                raw(tmp);
            } else {
                put((char)c);
            }
        }
        put('"');
    }

    // Length written (excluding NUL), or 0 on overflow.
    size_t finish()
    {
        if (!ok || n >= size)
            return 0;
        out[n] = '\0';
        return n;
    }

  private:
    void put(char c)
    {
        if (n + 1 < size)
            out[n++] = c;
        else
            ok = false;
    }

    char *out;
    size_t size;
    size_t n = 0;
    bool ok = true;
};

struct Hello {
    uint32_t node;
    const char *hw;
    const char *fw;
    const char *nb;
    uint32_t up;
    uint32_t boots;
    const char *const *channels;
    size_t channelCount;
    uint32_t drop;
    bool fcu;
};

inline size_t formatHelloJson(const Hello &h, char *out, size_t size)
{
    JsonWriter w(out, size);
    w.raw("{\"v\":1,\"node\":");
    w.unsignedNumber(h.node);
    w.raw(",\"hw\":");
    w.string(h.hw);
    w.raw(",\"fw\":");
    w.string(h.fw);
    w.raw(",\"nb\":");
    w.string(h.nb);
    w.raw(",\"up\":");
    w.unsignedNumber(h.up);
    w.raw(",\"boots\":");
    w.unsignedNumber(h.boots);
    w.raw(",\"ch\":[");
    for (size_t i = 0; i < h.channelCount; i++) {
        if (i)
            w.raw(",");
        w.string(h.channels[i]);
    }
    w.raw("],\"drop\":");
    w.unsignedNumber(h.drop);
    w.raw(",\"fcu\":");
    w.raw(h.fcu ? "1" : "0");
    w.raw("}");
    return w.finish();
}

inline size_t formatBatteryJson(int mv, int pct, bool usb, bool chg, char *out, size_t size)
{
    JsonWriter w(out, size);
    w.raw("{\"mv\":");
    w.number(mv);
    w.raw(",\"pct\":");
    w.number(pct);
    w.raw(",\"usb\":");
    w.raw(usb ? "1" : "0");
    w.raw(",\"chg\":");
    w.raw(chg ? "1" : "0");
    w.raw("}");
    return w.finish();
}

} // namespace NctnlFcuLine
