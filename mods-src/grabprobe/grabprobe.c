// grabprobe -- diagnostic mod: records the two fighters' memory every frame so we can find the
// grab state, its timer and the HP fields (first step of the "grab until the opponent breaks free" QTE mod).
//
// How it works
//   * every frame kick it reads the active-players table at 0x31C640 (two fighter pointers; documented in
//     ps2xRuntime/src/lib/game_overrides.cpp, PS2X_CAMPROBE) and copies FIGHTER_BYTES of each fighter
//     into a ring buffer holding the last RING_FRAMES frames, together with both pads.
//   * P1 holding SELECT + R3 for ~half a second writes the ring to mods/grabprobe_NNN.bin.
//
// Recording protocol (one file per move):
//   1. start a 1P vs 2P (or vs CPU) fight, stand still ~3 seconds
//   2. do ONE grab / Dragon Thunder / Life Absorb and let it play out completely
//   3. stand still ~3 seconds more, then hold SELECT + R3 -> "[grabprobe] wrote ..." in the log
//
// Build: see README.md next to this file. Depends only on ps2xRuntime/include/ps2x_mod_api.h.
#include "ps2x_mod_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PLAYER_TABLE   0x31C640u   // guest address: uint32 fighter pointers [0], [1]
#define FIGHTER_BYTES  0x1800u     // PCSX2 shows the fighters 0x1680 apart; grab a bit more
#define RING_FRAMES    450u        // ~15 s at the engine's 30 fps
#define HOLD_FRAMES    15u         // how long SELECT+R3 must be held

// PS2 pad word is active-low: a pressed button reads 0.
#define BTN_SELECT 0x0001u
#define BTN_R3     0x0004u

typedef struct {
    uint64_t frame;
    uint32_t pad[2];
    uint32_t ptr[2];
    uint8_t  mem[2][FIGHTER_BYTES];
} Sample;

static const Ps2xModApi *g_api;
static Sample *g_ring;
static uint32_t g_head, g_count, g_hold, g_fileNo;

static uint32_t rd32(const uint8_t *rdram, uint32_t addr)
{
    uint32_t a = addr & 0x1FFFFFFFu, v = 0;
    if (a + 4u <= g_api->ramSize) memcpy(&v, rdram + a, 4);
    return v;
}

static void logPath(const char *what, const char *path)
{
    char line[1200];
    snprintf(line, sizeof line, "[grabprobe] %.80s %.1100s", what, path);
    g_api->log(line);
}

static void dumpRing(void)
{
    char path[1024];
    const char *dir = g_api->exeDir ? g_api->exeDir() : ".";
    snprintf(path, sizeof path, "%s/mods/grabprobe_%03u.bin", dir ? dir : ".", g_fileNo++);
    FILE *f = fopen(path, "wb");
    if (!f) { logPath("could not open", path); return; }

    // header: magic, version, frames, bytes per fighter
    const uint32_t hdr[4] = { 0x50425247u /* "GRBP" */, 1u, g_count, FIGHTER_BYTES };
    fwrite(hdr, sizeof hdr, 1, f);
    const uint32_t first = (g_head + RING_FRAMES - g_count) % RING_FRAMES;
    for (uint32_t i = 0; i < g_count; ++i)
    {
        const Sample *s = &g_ring[(first + i) % RING_FRAMES];
        fwrite(&s->frame, 8, 1, f);
        fwrite(s->pad, 4, 2, f);
        fwrite(s->ptr, 4, 2, f);
        fwrite(s->mem, FIGHTER_BYTES, 2, f);
    }
    fclose(f);
    logPath("wrote the last ~15 s of the fight to", path);
}

static void onFrame(uint8_t *rdram, R5900Context *ctx, PS2Runtime *rt)
{
    (void)ctx; (void)rt;
    Sample *s = &g_ring[g_head];
    s->frame = g_api->frameCount();
    int live = 0;
    for (int p = 0; p < 2; ++p)
    {
        s->pad[p] = g_api->livePadButtons(p, NULL, NULL, NULL, NULL) & 0xFFFFu;
        const uint32_t fp = rd32(rdram, PLAYER_TABLE + 4u * (uint32_t)p);
        const uint32_t a = fp & 0x1FFFFFFFu;
        s->ptr[p] = fp;
        if (fp && a + FIGHTER_BYTES <= g_api->ramSize) { memcpy(s->mem[p], rdram + a, FIGHTER_BYTES); live = 1; }
        else memset(s->mem[p], 0, FIGHTER_BYTES);
    }
    if (!live) return;                       // menus: keep the ring for the fight
    g_head = (g_head + 1u) % RING_FRAMES;
    if (g_count < RING_FRAMES) ++g_count;

    // trigger: P1 holds SELECT + R3 (active-low -> both bits clear)
    if ((s->pad[0] & (BTN_SELECT | BTN_R3)) == 0u)
    {
        if (++g_hold == HOLD_FRAMES) dumpRing();
    }
    else g_hold = 0;
}

PS2X_MOD_EXPORT int ps2xModInit(const Ps2xModApi *api)
{
    if (!api || api->version < 1u || api->size < sizeof(Ps2xModApi)) return 1;
    g_api = api;
    g_ring = (Sample *)calloc(RING_FRAMES, sizeof(Sample));
    if (!g_ring) { api->log("[grabprobe] out of memory"); return 1; }
    api->setFrameHook(&onFrame);
    api->log("[grabprobe] loaded: hold SELECT + R3 (P1) after a grab to save the last ~15 s");
    return 0;
}
