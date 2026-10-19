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
#ifdef SKILL_RESULT_INTEGRATION_TEST
// Link with the built client/core objects (except client_net/presenter/main),
// SDL, zlib and fpattern. On MinGW use --allow-multiple-definition with this
// object FIRST, without LTO: the clock, display and animation probes are replaced.
static std::vector<std::string> displayed;
static int expectedVisibleHp = -1;
int falloutMain(int, char**) { std::abort(); }
bool clientViewerActive() { return true; }
unsigned int getTicks() { return now; }
void interfaceRenderHitPoints(bool) {}
void interfaceBarRefresh() {}
void paletteFadeTo(unsigned char* palette)
{
    if (palette != gPaletteBlack && expectedVisibleHp >= 0)
        assert(gDude->data.critter.hp == expectedVisibleHp);
    displayed.push_back(palette == gPaletteBlack ? "black" : "visible");
}
void displayMonitorAddMessageStyled(char* text, int) { displayed.push_back(text); }

static void checkSkillResults()
{
    live = true;
    Decoder d;
    Object patient{};
    patient.netId = 8;
    patient.pid = 0x01000001;
    patient.data.critter.hp = 10;
    actor.netId = 7;
    actor.pid = 0x01000001;
    gDude = &patient;
    d._net[7] = &actor;
    d._net[8] = &patient;
    d._loaded = true;
    d.applyDudeHp(8, 10);
    live = true;
    gesture = true;
    now = 100;
    unsigned int entry = 0;
    auto send = [&](unsigned char type, std::vector<unsigned char> bytes) {
        Reader r(bytes.data(), bytes.size());
        d.event(type, r, ++entry);
    };
    auto hp = [&](int netId, int total) {
        send(EVENT_OBJECT_DELTA, { (unsigned char)netId, 0, 0, 0,
            OBJECT_DELTA_HP, 0, (unsigned char)total, 0, 0, 0 });
    };
    auto result = [&]() {
        send(EVENT_SKILL_RESULT_BEGIN, { 7, 0, 0, 0 });
        send(EVENT_FADE_OUT, { 7, 0, 0, 0 });
        send(EVENT_CONSOLE, { 4, 0, 'h', 'e', 'a', 'l' });
        hp(8, 20);
        send(EVENT_FADE_IN, { 7, 0, 0, 0 });
        send(EVENT_WORLD_DELTA, { WORLD_DELTA_GAMETIME, 0, 40, 0, 0, 0 });
        send(EVENT_SKILL_RESULT_END, {});
    };
    result();
    assert(displayed.empty() && d._dudeHpAuth == 10);
    assert(d._presQueue.size() == 1 && d.skillMessageWaiting(d._presQueue.front()));
    send(EVENT_WORLD_DELTA, { WORLD_DELTA_GAMETIME, 0, 41, 0, 0, 0 });
    assert(displayed.empty() && d._presQueue.size() == 1);
    assert(gameTimeGetTime() == 41); // ordinary clock ticks do not release the heal
    hp(7, 33); // another actor's unrelated HP does not drain the result
    assert(displayed.empty() && d._dudeHpAuth == 10);
    assert(!d.dudeHpWaitsForPresentation());
    // The patient sees the message and heal together, without the healer's fade.
    gesture = false;
    auto queued = std::move(d._presQueue.front());
    d._presQueue.pop_front();
    d.applySkillResult(queued, false);
    assert(displayed == std::vector<std::string>({ "heal" }));
    assert(d._dudeHpAuth == 20);
    assert(gameTimeGetTime() == 41); // the delayed result cannot rewind the clock

    displayed.clear();
    gesture = true;
    result();
    hp(8, 14); // damage to the patient: release old heal, then apply damage NOW
    assert(d._presQueue.empty() && d._dudeHpAuth == 14);
    assert(displayed == std::vector<std::string>({ "heal" }));
    displayed.clear();
    result();
    d.screenPump(); // inventory: no frozen result or fade over the panel
    assert(d._presQueue.empty() && d._dudeHpAuth == 20);
    assert(displayed == std::vector<std::string>({ "heal" }));

    displayed.clear();
    result();
    now += 12000;
    d.flushSkillResults(0, true);
    assert(d._presQueue.empty() && displayed.size() == 1);

    // Healer's own screen: approach/gesture first, then black -> state -> visible.
    displayed.clear();
    gDude = &actor;
    actor.data.critter.hp = 10;
    d.applyDudeHp(7, 10);
    send(EVENT_SKILL_RESULT_BEGIN, { 7, 0, 0, 0 });
    send(EVENT_FADE_OUT, { 7, 0, 0, 0 });
    hp(7, 25);
    send(EVENT_CONSOLE, { 4, 0, 'h', 'e', 'a', 'l' });
    send(EVENT_FADE_IN, { 7, 0, 0, 0 });
    send(EVENT_SKILL_RESULT_END, {});
    assert(displayed.empty() && actor.data.critter.hp == 10);
    gesture = false;
    moving = true;
    assert(d.skillMessageWaiting(d._presQueue.front()));
    d.presentationPump();
    assert(displayed.empty());
    moving = false;
    assert(!d.skillMessageWaiting(d._presQueue.front()));
    expectedVisibleHp = 25;
    d.presentationPump();
    expectedVisibleHp = -1;
    assert(displayed == std::vector<std::string>({ "black", "heal", "visible" }));
    assert(actor.data.critter.hp == 25 && d._fadeBlackSinceMs == 0);

    displayed.clear();
    gesture = true;
    result(); // heal patient 8 while this viewer controls actor 7
    hp(7, 17);
    assert(d._dudeHpAuth == 17 && d._presQueue.size() == 1);
    assert(displayed.empty()); // our own unrelated HP is independent of their result
    hp(8, 0); // a later fatal hit must not be undone by the older heal
    assert(d._presQueue.empty() && patient.data.critter.hp == 0);
    assert(displayed == std::vector<std::string>({ "heal" }));

    // A truncated result has bounded retention and does not leave black behind.
    displayed.clear();
    send(EVENT_SKILL_RESULT_BEGIN, { 7, 0, 0, 0 });
    send(EVENT_FADE_OUT, { 7, 0, 0, 0 });
    hp(7, 30);
    now += 12000;
    d.expireSkillCapture();
    assert(d._skillCapture.consoleSkillActor == 0 && d._dudeHpAuth == 30);
    assert(displayed.empty());
    // Excessive input is applied early rather than retained without a bound.
    send(EVENT_SKILL_RESULT_BEGIN, { 7, 0, 0, 0 });
    for (int i = 0; i < 257; ++i) hp(7, 31);
    assert(d._skillCapture.consoleSkillActor == 0 && d._dudeHpAuth == 31);
    send(EVENT_SKILL_RESULT_END, {});
    gDude = nullptr;
}
#endif
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
#ifdef SKILL_RESULT_INTEGRATION_TEST
    checkSkillResults();
#endif
    puts("Skill message ordering tests passed.");
}
