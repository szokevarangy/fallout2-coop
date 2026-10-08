// Standalone parsing/code-table test; no running game required.
// g++ -std=c++17 -O0 -ffunction-sections -fdata-sections -Isrc -I<SDL2-include> \
//   tests/admin_status_commands_test.cc -Wl,--gc-sections -o /tmp/admin-status-test
#ifdef NDEBUG
#error This test requires assertions enabled.
#endif
#include "../src/server_admin.cc"
#include <cassert>
int main()
{
    using namespace fallout;
    int slot = -1;
    int value = -1;
    assert(parseStateArgs("0 20", slot, value) && slot == 0 && value == 20);
    assert(parseStateArgs(" 2\t-100  ", slot, value) && slot == 2 && value == -100);
    assert(!parseStateArgs(nullptr, slot, value));
    assert(!parseStateArgs("", slot, value));
    assert(!parseStateArgs("0", slot, value));
    assert(!parseStateArgs("no 20", slot, value));
    assert(!parseStateArgs("-1 20", slot, value));
    assert(!parseStateArgs("0 2 extra", slot, value));
    assert(!parseStateArgs("0 2x", slot, value));
    assert(!parseStateArgs("0 9999999999999999999999999", slot, value));
    assert(!parseStateArgs("9999999999999999999999999 1", slot, value));
    assert(kAdminAddictionPids[3] == PROTO_ID_JET);
    assert(kAdminInjuryFlags[0] == DAM_CRIP_ARM_LEFT);
    assert(kAdminInjuryFlags[4] == DAM_BLIND);
    int pid = 0;
    int count = 0;
    assert(parseGiveArgs("1 40", slot, pid, count) && slot == 1 && pid == 40 && count == 1);
    assert(parseGiveArgs("2 0x28 10", slot, pid, count) && slot == 2 && pid == 40 && count == 10);
    assert(parseGiveArgs("0 008 2", slot, pid, count) && pid == 8 && count == 2);
    assert(!parseGiveArgs(nullptr, slot, pid, count));
    assert(!parseGiveArgs("1", slot, pid, count));
    assert(!parseGiveArgs("1 40 0", slot, pid, count));
    assert(!parseGiveArgs("1 40 -2", slot, pid, count));
    assert(!parseGiveArgs("1 40 10001", slot, pid, count));
    assert(!parseGiveArgs("1 40 2 garbage", slot, pid, count));
    assert(!parseGiveArgs("1 40x", slot, pid, count));
    assert(!parseGiveArgs("1 99999999999999999999999", slot, pid, count));
    puts("PASS: status command argument validation and code mapping");
}
