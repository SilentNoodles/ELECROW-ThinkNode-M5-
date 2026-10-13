#include "NctnlTextStore.h"

#if NCTNL_TEXT_STORE
#include "FSCommon.h"
#include "NodeDB.h"
#include "RTC.h"
#include "SPILock.h"
#include "SafeFile.h"
#include "modules/NctnlModule.h"
#include <cstring>

namespace NctnlTextStore
{
namespace
{
constexpr const char *FILENAME = "/nctnl_text.dat";
constexpr uint8_t FILE_VERSION = 1;

struct __attribute__((packed)) RecordHeader {
    uint32_t from;
    uint32_t to;
    uint32_t id;
    uint32_t time;
    uint8_t channel;
    uint8_t len;
};

Entry entries[CAPACITY];
size_t head;
size_t used;
bool dirty;
} // namespace

void record(const meshtastic_MeshPacket &mp, uint32_t from, uint32_t time)
{
    Entry &e = entries[(head + used) % CAPACITY];
    if (used < CAPACITY)
        used++;
    else
        head = (head + 1) % CAPACITY;
    e.from = from;
    e.to = mp.to;
    e.id = mp.id;
    e.time = time;
    e.channel = mp.channel;
    const size_t len = mp.decoded.payload.size < MAX_TEXT ? mp.decoded.payload.size : MAX_TEXT;
    memcpy(e.text, mp.decoded.payload.bytes, len);
    e.text[len] = '\0';
    e.len = (uint8_t)strnlen(e.text, len);
    dirty = true;
}

void onSent(const meshtastic_MeshPacket &mp)
{
    if (nctnlModule && nctnlModule->isHiddenChannelText(mp))
        record(mp, nodeDB->getNodeNum(), getValidTime(RTCQualityDevice));
}

size_t count()
{
    return used;
}

const Entry *get(size_t index)
{
    return index < used ? &entries[(head + index) % CAPACITY] : nullptr;
}

bool isDirty()
{
    return dirty;
}

void save()
{
#ifdef FSCom
    if (!dirty)
        return;
    SafeFile f(FILENAME, false);
    spiLock->lock();
    const uint8_t header[2] = {FILE_VERSION, (uint8_t)used};
    f.write(header, sizeof(header));
    for (size_t i = 0; i < used; i++) {
        const Entry &e = *get(i);
        const RecordHeader rec = {e.from, e.to, e.id, e.time, e.channel, e.len};
        f.write(reinterpret_cast<const uint8_t *>(&rec), sizeof(rec));
        f.write(reinterpret_cast<const uint8_t *>(e.text), e.len);
    }
    spiLock->unlock();
    if (f.close())
        dirty = false;
#endif
}

void load()
{
    head = 0;
    used = 0;
    dirty = false;
#ifdef FSCom
    concurrency::LockGuard guard(spiLock);
    if (!FSCom.exists(FILENAME))
        return;
    auto f = FSCom.open(FILENAME, FILE_O_READ);
    if (!f)
        return;
    uint8_t header[2] = {};
    if (f.readBytes(reinterpret_cast<char *>(header), sizeof(header)) == sizeof(header) && header[0] == FILE_VERSION) {
        const size_t n = header[1] < CAPACITY ? header[1] : CAPACITY;
        for (size_t i = 0; i < n; i++) {
            RecordHeader rec;
            if (f.readBytes(reinterpret_cast<char *>(&rec), sizeof(rec)) != sizeof(rec) || rec.len > MAX_TEXT)
                break;
            Entry &e = entries[i];
            if (f.readBytes(e.text, rec.len) != rec.len)
                break;
            e.text[rec.len] = '\0';
            e.from = rec.from;
            e.to = rec.to;
            e.id = rec.id;
            e.time = rec.time;
            e.channel = rec.channel;
            e.len = rec.len;
            used++;
        }
    }
    f.close();
#endif
}
} // namespace NctnlTextStore
#endif
