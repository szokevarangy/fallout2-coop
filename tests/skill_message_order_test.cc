// Production decoder gates with a stub clock and actor animation state.
// g++ -std=c++17 -O2 -flto -DSDL_MAIN_HANDLED -ffunction-sections -fdata-sections -Isrc -I<SDL2-include>
//   tests/skill_message_order_test.cc -Wl,--gc-sections -lws2_32 -o skill-message-test
#ifdef NDEBUG
#error This test requires assertions enabled.
#endif
#include <algorithm>
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#define private public
#include "../src/client_net.cc"
#undef private
#include "../src/presenter.cc"
#include <cassert>

namespace fallout {
static unsigned int now = 100;
static Object actor;
Object* gDude = nullptr;
static bool gesture = false, moving = false, attack = false, live = true;
unsigned int getTicksSince(unsigned int since) { return now - since; }
bool objectIsLive(const Object*) { return live; }
void objectSetFreedHook(void (*)(Object*)) {}
int animationIsBusyIgnoringFidgets(Object* obj) { return obj == &actor && gesture; }
bool clientAnimPlayableActiveFor(Object* obj) { return obj == &actor && moving; }
bool clientCombatAnimActive() { return attack; }
int debugPrint(const char*, ...) { return 0; }
}

int main()
{
    using namespace fallout;
    Decoder decoder;
    decoder._net[7] = &actor;
    Decoder::PresEvent message{};
    message.kind = Decoder::PresKind::kConsole;
    message.consoleSkillActor = 7;
    message.consoleQueuedAt = now;
    decoder._presQueue.push_back(message);

    gesture = true;
    assert(decoder.skillMessageWaiting(message));
    // A teammate's queued result must not hold our own out-of-combat HP.
    assert(!decoder.dudeHpWaitsForPresentation());
    Object localActor{};
    gDude = &localActor;
    decoder._net[8] = gDude;
    decoder.applyDudeHp(8, 42);
    assert(decoder._dudeHpAuth == 42);
    assert(decoder.skillMessageWaiting(message));
    gesture = false;
    moving = true;
    assert(decoder.skillMessageWaiting(message));
    moving = false;
    assert(!decoder.skillMessageWaiting(message));

    gesture = true;
    now += 11999;
    assert(decoder.skillMessageWaiting(message));
    ++now;
    assert(!decoder.skillMessageWaiting(message));
    message.consoleQueuedAt = 0xFFFFFF00u;
    now = message.consoleQueuedAt + 11999u;
    assert(decoder.skillMessageWaiting(message));
    ++now;
    assert(!decoder.skillMessageWaiting(message));

    message.consoleQueuedAt = now;
    message.consoleSkillActor = 99;
    assert(!decoder.skillMessageWaiting(message));
    message.consoleSkillActor = 7;
    live = false;
    assert(!decoder.skillMessageWaiting(message));
    message.consoleSkillActor = 0; // legacy console payload
    assert(!decoder.skillMessageWaiting(message));

    decoder.setInCombat(true);
    assert(decoder.dudeHpWaitsForPresentation());
    decoder.setInCombat(false);
    Decoder::PresEvent exit{};
    exit.kind = Decoder::PresKind::kExit;
    decoder._presQueue.push_back(exit);
    assert(decoder.dudeHpWaitsForPresentation()); // preserve combat-tail ordering

    assert(presenterSkillMessageActor() == 0);
    {
        PresenterSkillMessageScope outer(7);
        assert(presenterSkillMessageActor() == 7);
        {
            PresenterSkillMessageScope inner(8);
            assert(presenterSkillMessageActor() == 8);
        }
        assert(presenterSkillMessageActor() == 7);
    }
    assert(presenterSkillMessageActor() == 0);
    puts("Skill message ordering tests passed.");
}
