#include "configuration.h"

#if NCTNL_FCU_MIRROR
#include "NctnlFcuLine.h"
#include "NctnlFcuMirror.h"
#include "NodeDB.h"
#include "PowerStatus.h"
#include "SerialConsole.h"
#include "Throttle.h"
#include "concurrency/LockGuard.h"
#include "concurrency/OSThread.h"
#include "mesh-pb-constants.h"

namespace NctnlFcuMirror
{
namespace
{
constexpr uint32_t HELLO_INTERVAL_MS = 60 * 1000;

concurrency::Lock *mirrorLock;
uint32_t seq;
uint32_t lastHelloMs;
uint8_t encodeBuf[meshtastic_MeshPacket_size];
char lineBuf[NctnlFcuLine::lineLen(NctnlFcuLine::base64Len(meshtastic_MeshPacket_size))];
meshtastic_MeshPacket fromPhoneCopy;

// Caller holds mirrorLock.
void writeLine(char type, const uint8_t *payload, size_t len, bool asBase64)
{
    const size_t n = NctnlFcuLine::formatLine(type, seq + 1, payload, len, asBase64, lineBuf, sizeof(lineBuf));
    if (n == 0 || !console)
        return;
    seq++;
    console->writeMirrorLine(reinterpret_cast<const uint8_t *>(lineBuf), n);
}

// Caller holds mirrorLock.
void writePacket(char type, const meshtastic_MeshPacket &p)
{
    const size_t len = pb_encode_to_bytes(encodeBuf, sizeof(encodeBuf), &meshtastic_MeshPacket_msg, &p);
    if (len > 0)
        writeLine(type, encodeBuf, len, true);
}

void mirrorPacket(char type, const meshtastic_MeshPacket &p)
{
    concurrency::LockGuard guard(mirrorLock);
    writePacket(type, p);
}

bool isAdmin(const meshtastic_MeshPacket &p)
{
    return p.which_payload_variant == meshtastic_MeshPacket_decoded_tag && p.decoded.portnum == meshtastic_PortNum_ADMIN_APP;
}

void emitHello()
{
    if (!mirrorLock)
        return;
    char text[160];
    const int n = snprintf(text, sizeof(text), "node=!%08lx fw=%s mirror=1 up=%lu mv=%d pct=%u usb=%d chg=%d",
                           (unsigned long)nodeDB->getNodeNum(), optstr(APP_VERSION), (unsigned long)(millis() / 1000),
                           powerStatus->getBatteryVoltageMv(), (unsigned)powerStatus->getBatteryChargePercent(),
                           powerStatus->getHasUSB() ? 1 : 0, powerStatus->getIsCharging() ? 1 : 0);
    if (n <= 0 || (size_t)n >= sizeof(text))
        return;
    concurrency::LockGuard guard(mirrorLock);
    writeLine('H', reinterpret_cast<const uint8_t *>(text), (size_t)n, false);
}

class HelloThread : public concurrency::OSThread
{
  public:
    HelloThread() : OSThread("NctnlFcuMirror") {}

  protected:
    int32_t runOnce() override
    {
        if (isActive())
            Throttle::execute(&lastHelloMs, HELLO_INTERVAL_MS, emitHello);
        return 1000;
    }
};
} // namespace

bool isActive()
{
    return !moduleConfig.nctnl.fcu_integration_disabled;
}

void begin()
{
    if (mirrorLock)
        return;
    mirrorLock = new concurrency::Lock();
    if (isActive())
        Throttle::execute(&lastHelloMs, HELLO_INTERVAL_MS, emitHello);
    new HelloThread();
}

void onLoraRx(const meshtastic_MeshPacket *p)
{
    if (p && mirrorLock && isActive())
        mirrorPacket('R', *p);
}

void onTxComplete(const meshtastic_MeshPacket *p)
{
    if (p && mirrorLock && isActive())
        mirrorPacket('T', *p);
}

void onToPhone(const meshtastic_MeshPacket *p)
{
    if (p && mirrorLock && isActive() && !isAdmin(*p))
        mirrorPacket('P', *p);
}

void onFromPhone(const meshtastic_MeshPacket &p)
{
    if (!mirrorLock || !isActive() || isAdmin(p))
        return;
    concurrency::LockGuard guard(mirrorLock);
    fromPhoneCopy = p;
    fromPhoneCopy.from = nodeDB->getNodeNum();
    writePacket('U', fromPhoneCopy);
}
} // namespace NctnlFcuMirror
#endif
