// XMVSF Stage 13.1: apply controller bindings to active FBNeo game inputs.
// Installed in the SDL2 driver initialization path, AFTER GameInpDefault().
#ifndef XMVSF_STAGE13_1_GAME_BINDINGS_H
#define XMVSF_STAGE13_1_GAME_BINDINGS_H
#include "xmvsf_stage13_controls.h"
#include <ctype.h>

// Return the virtual joystick subcode for a CPS-2 action, or -1 if unknown.
static int xm131InputSubcode(const char* action) {
    if (!action) return -1;
    if (!strcmp(action, "left")) return 0x00;
    if (!strcmp(action, "right")) return 0x01;
    if (!strcmp(action, "up")) return 0x02;
    if (!strcmp(action, "down")) return 0x03;
    if (!strcmp(action, "coin")) return 0x86;
    if (!strcmp(action, "start")) return 0x87;
    if (!strncmp(action, "fire ", 5)) {
        char* end = NULL;
        long button = strtol(action + 5, &end, 10);
        if (end != action + 5 && *end == '\0' && button >= 1 && button <= 6)
            return 0x80 + (int)button - 1;
    }
    return -1;
}

// FBNeo has already loaded the game config and called GameInpDefault() here.
// Writes only the chosen two players' switch-code mappings; untouched inputs
// retain their previous settings. SDLinpState() resolves these virtual codes
// through the Stage13 per-device controller reader, with no extra input queue.
static void xm131ApplyGameInputMappings() {
    XM13_Settings mapping;
    int loaded = xm13Load(&mapping);
    int counts[2] = {0, 0};
    FILE* log = fopen("xmvsf_stage13_1_input.log", "w");
    if (log) fprintf(log,
        "Stage13.1 saved-settings-read=%d active-P1=%d active-P2=%d menu-P1=%d menu-P2=%d\n",
        loaded, mapping.player[0].active, mapping.player[1].active,
        xm249SessionPad[0].selected?1:0,xm249SessionPad[1].selected?1:0);
    for (unsigned int i = 0; i < nGameInpCount; i++) {
        struct BurnInputInfo bii = {};
        if (BurnDrvGetInputInfo(&bii, i) || bii.pVal == NULL || bii.szInfo == NULL)
            continue;
        const char* name = bii.szInfo;
        if (tolower((unsigned char)name[0]) != 'p' || name[1] < '1' || name[1] > '2' || name[2] != ' ')
            continue;
        const int player = name[1] - '1';
        // Boot-menu acceptance is an explicit, temporary controller
        // selection even when P1 has NO saved mapping (common with Xbox
        // and DS4Windows). Use virtual SDL P1 codes for this match rather
        // than leaving the FBNeo keyboard/legacy joystick defaults active.
        if (!XM249Policy::needsVirtualMap(
                mapping.player[player].active!=0,
                xm249SessionPad[player].selected))continue;
        const int subcode = xm131InputSubcode(name + 3);
        if (subcode < 0) continue;
        struct GameInp* gi = &GameInp[i];
        gi->nInput = GIT_SWITCH;
        gi->Input.Switch.nCode = (UINT16)(0x4000 | (player << 8) | subcode);
        counts[player]++;
        if (log) fprintf(log, "%s -> %04X\n", name, (unsigned int)gi->Input.Switch.nCode);
    }
    if (log) {
        fprintf(log, "P1 mapped=%d; P2 mapped=%d\n", counts[0], counts[1]);
        if (mapping.player[0].active && counts[0] < 10)
            fprintf(log, "WARNING: P1 expected at least 10 CPS-2 switch mappings.\n");
        if (mapping.player[1].active && counts[1] < 10)
            fprintf(log, "WARNING: P2 expected at least 10 CPS-2 switch mappings.\n");
        fclose(log);
    }
    printf("XMVSF Stage13.1 game inputs: P1=%d P2=%d mapping slots\n", counts[0], counts[1]);
}
#endif
