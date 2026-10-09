#ifndef FALLOUT_ANIMATION_DIAGNOSTIC_H_
#define FALLOUT_ANIMATION_DIAGNOSTIC_H_
#include <cstdio>
#include <cstdlib>
#include "combat_defs.h"
#include "input.h"
#include "item.h"
#include "object.h"
namespace fallout {
// Opt-in, one distinct path per process. No simulation or animation changes.
inline bool animDiagnosticEnabled()
{
    const char* path = std::getenv("F2_ANIM_DIAG");
    return path != nullptr && path[0] != '\0';
}
inline void animDiagnostic(const char* event, Object* obj, int a = 0, int b = 0, int c = 0)
{
    if (!animDiagnosticEnabled()) return;
    static FILE* log = nullptr;
    static bool attempted = false;
    static unsigned long sequence = 0;
    if (!attempted) {
        attempted = true;
        log = std::fopen(std::getenv("F2_ANIM_DIAG"), "w");
        if (log == nullptr) std::fprintf(stderr, "Cannot open F2_ANIM_DIAG log: %s\n", std::getenv("F2_ANIM_DIAG"));
        else std::fprintf(log, "animation diagnostic v1; a/b/c depend on event; ticks are process-local\n");
    }
    if (log == nullptr) return;
    Object* weapon = obj != nullptr && FID_TYPE(obj->fid) == OBJ_TYPE_CRITTER
        ? critterGetWeaponForHitMode(obj, HIT_MODE_RIGHT_WEAPON_PRIMARY) : nullptr;
    std::fprintf(log, "seq=%lu tick=%u event=%s net=%d tile=%d elev=%d x=%d y=%d fid=0x%X frame=%d rot=%d ap=%d weaponNet=%d weaponPid=%d a=%d b=%d c=%d\n",
        ++sequence, getTicks(), event, obj ? obj->netId : -1, obj ? obj->tile : -1,
        obj ? obj->elevation : -1, obj ? obj->x : 0, obj ? obj->y : 0,
        obj ? obj->fid : 0, obj ? obj->frame : -1, obj ? obj->rotation : -1,
        obj && FID_TYPE(obj->fid) == OBJ_TYPE_CRITTER ? obj->data.critter.combat.ap : -1,
        weapon ? weapon->netId : -1, weapon ? weapon->pid : -1, a, b, c);
    std::fflush(log);
}
}
#endif
