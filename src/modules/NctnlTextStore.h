#pragma once

#include "configuration.h"

#if NCTNL_FCU_MIRROR && defined(ELECROW_ThinkNode_M5) && !MESHTASTIC_EXCLUDE_NCTNL && HAS_SCREEN
#define NCTNL_TEXT_STORE 1
#include "mesh/generated/meshtastic/mesh.pb.h"
#include <cstddef>
#include <cstdint>

// Hidden ring of the last NCTNL_DATA / NCTNL_CTRL text events, replayed to the FCU as D lines. Never displayed.
// Persisted only when MessageStore saves (its autosave timer and the shutdown/reboot paths).
namespace NctnlTextStore
{
constexpr size_t CAPACITY = 50;
constexpr size_t MAX_TEXT = meshtastic_Constants_DATA_PAYLOAD_LEN;

struct Entry {
    uint32_t from;
    uint32_t to;
    uint32_t id;
    uint32_t time; // rx_time for received, RTC time for sent; 0 = unknown
    uint8_t channel;
    uint8_t len;
    char text[MAX_TEXT + 1];
};

void record(const meshtastic_MeshPacket &mp, uint32_t from, uint32_t time);
// Records our own text if it is on NCTNL_DATA or NCTNL_CTRL.
void onSent(const meshtastic_MeshPacket &mp);
size_t count();
// Oldest first.
const Entry *get(size_t index);
bool isDirty();
// Writes the file only when entries changed since the last save or load.
void save();
void load();
} // namespace NctnlTextStore
#else
#define NCTNL_TEXT_STORE 0
#endif
