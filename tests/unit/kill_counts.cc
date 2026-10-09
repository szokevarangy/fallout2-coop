// Standalone test of the production kill-counter storage and serialization.
// Build with critter.cc and section GC; unused engine code is discarded.
#include "critter.h"
#include "object.h"
#include "player_sheet.h"
#include "server_players.h"
#include "proto_types.h"
#include <cassert>
#include <vector>

namespace fallout {
struct TestFile : File { std::vector<int> values; size_t offset = 0; };
Object host{}, guest{}, npc{}, companion{}, bomb{};
Object* gDude = &host;
static int dirty[2];
int playerActorCount() { return 2; }
int partyMemberOwnerSlot(Object* member) { return member == &companion ? 1 : -1; }
int playerActorSlotOf(Object* actor) { return actor == &host ? 0 : actor == &guest ? 1 : -1; }
void playerSheetMarkDirty(Object* actor) { dirty[playerActorSlotOf(actor)]++; }
int fileWriteInt32List(File* stream, int* values, int count)
{
    auto* test = static_cast<TestFile*>(stream);
    test->values.insert(test->values.end(), values, values + count);
    return 0;
}
int fileReadInt32List(File* stream, int* values, int count)
{
    auto* test = static_cast<TestFile*>(stream);
    if (test->offset + count > test->values.size()) return -1;
    for (int i = 0; i < count; i++) values[i] = test->values[test->offset++];
    return 0;
}
}

int main()
{
    using namespace fallout;
    host.fid = guest.fid = npc.fid = companion.fid = 0x01000000;
    bomb.fid = 0x05000000;
    assert(killsAttackOwnerSlot(&host, &npc) == 0);
    assert(killsAttackOwnerSlot(&guest, &npc) == 1);
    assert(killsAttackOwnerSlot(&companion, &npc) == 1);
    assert(killsAttackOwnerSlot(&npc, &host) == -1);
    assert(killsAttackOwnerSlot(&bomb, &npc) == -1);
    assert(killsAttackOwnerSlot(nullptr, &npc) == -1);
    assert(killsAttackOwnerSlot(&guest, &guest) == -1);
    // A prior hit by the host must not affect the guest's final-hit credit.
    npc.data.critter.combat.whoHitMe = &host;
    assert(killsAttackOwnerSlot(&guest, &npc) == 1);
    killsPlayerActorResetSlot(0);
    killsPlayerActorResetSlot(1);
    assert(killsIncByType(0, &host) == 0);
    assert(killsIncByType(1, &guest) == 0);
    assert(killsIncByType(1, &guest) == 0);
    assert(killsGetByType(0, &host) == 1);
    assert(killsGetByType(1, &host) == 0);
    assert(killsGetByType(0, &guest) == 0);
    assert(killsGetByType(1, &guest) == 2);
    assert(dirty[0] == 1 && dirty[1] == 2);
    // Viewer role rebinding must select the guest, not the host's legacy array.
    gDude = &guest;
    assert(killsGetByType(1) == 2);
    assert(killsIncByType(1) == 0);
    assert(killsGetByType(1, &guest) == 3);
    assert(killsIncByType(-2, &guest) == -1);
    assert(killsIncByType(KILL_TYPE_COUNT, &guest) == -1);
    assert(killsIncByType(0, &npc) == -1);
    assert(killsGetByType(-2, &guest) == 0);
    TestFile saved;
    assert(killsPlayerActorRowWrite(&saved, 1) == 0);
    killsPlayerActorResetSlot(1);
    assert(killsGetByType(1, &guest) == 0);
    assert(killsPlayerActorRowRead(&saved, 1) == 0);
    assert(killsGetByType(1, &guest) == 3);
    assert(killsGetByType(0, &host) == 1);
    // Truncated and invalid rows must not partially overwrite the live row.
    TestFile truncated;
    truncated.values = {99};
    assert(killsPlayerActorRowRead(&truncated, 1) == -1);
    TestFile invalid;
    invalid.values.resize(KILL_TYPE_COUNT, 0);
    invalid.values[0] = -1;
    assert(killsPlayerActorRowRead(&invalid, 1) == -1);
    assert(killsGetByType(1, &guest) == 3);
    assert(killsPlayerActorRowRead(&saved, -1) == -1);
}
