#include "global.h"
#include "run_settings_menu.h"
#include "bg.h"
#include "difficulty.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "main.h"
#include "menu.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "wild_encounter.h"
#include "window.h"
#include "constants/difficulty.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define tMenuSelection data[0]
#define tEncounterMode data[1]
#define tDifficulty    data[2]

enum
{
    MENUITEM_ENCOUNTERS,
    MENUITEM_DIFFICULTY,
    MENUITEM_CANCEL,
    MENUITEM_COUNT,
};

enum
{
    WIN_HEADER,
    WIN_OPTIONS
};

#define YPOS_ENCOUNTERS (MENUITEM_ENCOUNTERS * 16)
#define YPOS_DIFFICULTY (MENUITEM_DIFFICULTY * 16)

static void Task_RunSettingsFadeIn(u8 taskId);
static void Task_RunSettingsProcessInput(u8 taskId);
static void Task_RunSettingsSave(u8 taskId);
static void Task_RunSettingsFadeOut(u8 taskId);
static void HighlightRunSettingsItem(u8 selection);
static u8 ThreeWay_ProcessInput(u8 selection, u8 count);
static void Encounters_DrawChoices(u8 selection);
static void Difficulty_DrawChoices(u8 selection);
static void DrawHeaderText(void);
static void DrawRunSettingsTexts(void);
static void DrawBgWindowFrames(void);

EWRAM_DATA static bool8 sArrowPressed = FALSE;

static const u8 sText_Header[]          = _("RUN SETTINGS");
static const u8 sText_Encounters[]      = _("ENCOUNTERS");
static const u8 sText_Difficulty[]      = _("DIFFICULTY");
static const u8 sText_Cancel[]          = _("DONE");

static const u8 sText_EncVanilla[]      = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}CLASSIC");
static const u8 sText_EncModern[]       = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}MODERN");
static const u8 sText_EncPostGame[]     = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}POST-GAME");

static const u8 sText_DiffEasy[]        = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}EASY");
static const u8 sText_DiffNormal[]      = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}NORMAL");
static const u8 sText_DiffHard[]        = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}HARD");

static const u16 sRunSettingsText_Pal[] = INCGFX_U16("graphics/interface/option_menu_text.pal", ".gbapal");

static const u8 *const sRunSettingsItemNames[MENUITEM_COUNT] =
{
    [MENUITEM_ENCOUNTERS] = sText_Encounters,
    [MENUITEM_DIFFICULTY] = sText_Difficulty,
    [MENUITEM_CANCEL]     = sText_Cancel,
};

static const u8 *const sEncounterChoices[ENCOUNTER_MODE_COUNT] =
{
    [ENCOUNTER_MODE_VANILLA]   = sText_EncVanilla,
    [ENCOUNTER_MODE_MODERN]    = sText_EncModern,
    [ENCOUNTER_MODE_POST_GAME] = sText_EncPostGame,
};

static const u8 *const sDifficultyChoices[3] =
{
    [DIFFICULTY_EASY]   = sText_DiffEasy,
    [DIFFICULTY_NORMAL] = sText_DiffNormal,
    [DIFFICULTY_HARD]   = sText_DiffHard,
};

static const struct WindowTemplate sRunSettingsWinTemplates[] =
{
    [WIN_HEADER] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 1,
        .width = 26,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 2
    },
    [WIN_OPTIONS] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 5,
        .width = 26,
        .height = 14,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sRunSettingsBgTemplates[] =
{
    {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 0,
        .charBaseIndex = 1,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    }
};

static const u16 sRunSettingsBg_Pal[] = {RGB(17, 18, 31)};

static void MainCB2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

void CB2_InitRunSettingsMenu(void)
{
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void *)(VRAM), VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sRunSettingsBgTemplates, ARRAY_COUNT(sRunSettingsBgTemplates));
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        ChangeBgX(1, 0, BG_COORD_SET);
        ChangeBgY(1, 0, BG_COORD_SET);
        ChangeBgX(2, 0, BG_COORD_SET);
        ChangeBgY(2, 0, BG_COORD_SET);
        ChangeBgX(3, 0, BG_COORD_SET);
        ChangeBgY(3, 0, BG_COORD_SET);
        InitWindows(sRunSettingsWinTemplates);
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_BG1 | WINOUT_WIN01_CLR);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_DARKEN);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 4);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        ShowBg(1);
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        gMain.state++;
        break;
    case 3:
        LoadBgTiles(1, GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->tiles, 0x120, 0x1A2);
        gMain.state++;
        break;
    case 4:
        LoadPalette(sRunSettingsBg_Pal, BG_PLTT_ID(0), sizeof(sRunSettingsBg_Pal));
        LoadPalette(GetWindowFrameTilesPal(gSaveBlock2Ptr->optionsWindowFrameType)->pal, BG_PLTT_ID(7), PLTT_SIZE_4BPP);
        gMain.state++;
        break;
    case 5:
        LoadPalette(sRunSettingsText_Pal, BG_PLTT_ID(1), sizeof(sRunSettingsText_Pal));
        gMain.state++;
        break;
    case 6:
        PutWindowTilemap(WIN_HEADER);
        DrawHeaderText();
        gMain.state++;
        break;
    case 7:
        gMain.state++;
        break;
    case 8:
        PutWindowTilemap(WIN_OPTIONS);
        DrawRunSettingsTexts();
        gMain.state++;
    case 9:
        DrawBgWindowFrames();
        gMain.state++;
        break;
    case 10:
    {
        u8 taskId = CreateTask(Task_RunSettingsFadeIn, 0);

        gTasks[taskId].tMenuSelection = 0;
        gTasks[taskId].tEncounterMode = VarGet(VAR_ENCOUNTER_MODE);
        if (gTasks[taskId].tEncounterMode >= ENCOUNTER_MODE_COUNT)
            gTasks[taskId].tEncounterMode = ENCOUNTER_MODE_VANILLA;
        gTasks[taskId].tDifficulty = GetCurrentDifficultyLevel();
        if (gTasks[taskId].tDifficulty > DIFFICULTY_HARD)
            gTasks[taskId].tDifficulty = DIFFICULTY_NORMAL;

        Encounters_DrawChoices(gTasks[taskId].tEncounterMode);
        Difficulty_DrawChoices(gTasks[taskId].tDifficulty);
        HighlightRunSettingsItem(gTasks[taskId].tMenuSelection);

        CopyWindowToVram(WIN_OPTIONS, COPYWIN_FULL);
        gMain.state++;
        break;
    }
    case 11:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB);
        SetMainCallback2(MainCB2);
        return;
    }
}

static void Task_RunSettingsFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_RunSettingsProcessInput;
}

static void Task_RunSettingsProcessInput(u8 taskId)
{
    if (JOY_NEW(A_BUTTON))
    {
        if (gTasks[taskId].tMenuSelection == MENUITEM_CANCEL)
            gTasks[taskId].func = Task_RunSettingsSave;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_RunSettingsSave;
    }
    else if (JOY_NEW(DPAD_UP))
    {
        if (gTasks[taskId].tMenuSelection > 0)
            gTasks[taskId].tMenuSelection--;
        else
            gTasks[taskId].tMenuSelection = MENUITEM_CANCEL;
        HighlightRunSettingsItem(gTasks[taskId].tMenuSelection);
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (gTasks[taskId].tMenuSelection < MENUITEM_CANCEL)
            gTasks[taskId].tMenuSelection++;
        else
            gTasks[taskId].tMenuSelection = 0;
        HighlightRunSettingsItem(gTasks[taskId].tMenuSelection);
    }
    else
    {
        u8 previousOption;

        switch (gTasks[taskId].tMenuSelection)
        {
        case MENUITEM_ENCOUNTERS:
            previousOption = gTasks[taskId].tEncounterMode;
            gTasks[taskId].tEncounterMode = ThreeWay_ProcessInput(gTasks[taskId].tEncounterMode, ENCOUNTER_MODE_COUNT);

            if (previousOption != gTasks[taskId].tEncounterMode)
                Encounters_DrawChoices(gTasks[taskId].tEncounterMode);
            break;
        case MENUITEM_DIFFICULTY:
            previousOption = gTasks[taskId].tDifficulty;
            gTasks[taskId].tDifficulty = ThreeWay_ProcessInput(gTasks[taskId].tDifficulty, DIFFICULTY_HARD + 1);

            if (previousOption != gTasks[taskId].tDifficulty)
                Difficulty_DrawChoices(gTasks[taskId].tDifficulty);
            break;
        default:
            return;
        }

        if (sArrowPressed)
        {
            sArrowPressed = FALSE;
            CopyWindowToVram(WIN_OPTIONS, COPYWIN_GFX);
        }
    }
}

static void Task_RunSettingsSave(u8 taskId)
{
    VarSet(VAR_ENCOUNTER_MODE, gTasks[taskId].tEncounterMode);
    SetCurrentDifficultyLevel(gTasks[taskId].tDifficulty);
    FlagSet(FLAG_RUN_SETTINGS_SET);

    // The imported legendary scripts read this flag to scale their stats
    if (gTasks[taskId].tDifficulty == DIFFICULTY_HARD)
        FlagSet(FLAG_DIFFICULTY_HARD);
    else
        FlagClear(FLAG_DIFFICULTY_HARD);

    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_RunSettingsFadeOut;
}

static void Task_RunSettingsFadeOut(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        SetMainCallback2(gMain.savedCallback);
    }
}

static void HighlightRunSettingsItem(u8 index)
{
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(16, DISPLAY_WIDTH - 16));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(index * 16 + 40, index * 16 + 56));
}

static void DrawChoice(const u8 *text, u8 x, u8 y, u8 style)
{
    u8 dst[16];
    u16 i;

    for (i = 0; *text != EOS && i <= 14; i++)
    {
        dst[i] = *text;
        text++;
    }

    if (style != 0)
    {
        dst[2] = TEXT_COLOR_RED;
        dst[4] = TEXT_COLOR_LIGHT_RED;
    }

    dst[i] = EOS;
    AddTextPrinterParameterized(WIN_OPTIONS, FONT_NORMAL, dst, x, y + 1, TEXT_SKIP_DRAW, NULL);
}

static u8 ThreeWay_ProcessInput(u8 selection, u8 count)
{
    if (JOY_NEW(DPAD_RIGHT))
    {
        if (selection < count - 1)
            selection++;
        else
            selection = 0;

        sArrowPressed = TRUE;
    }
    if (JOY_NEW(DPAD_LEFT))
    {
        if (selection != 0)
            selection--;
        else
            selection = count - 1;

        sArrowPressed = TRUE;
    }
    return selection;
}

// Draws three choices side by side: left, middle, and right aligned to the edge
static void DrawThreeChoices(const u8 *const *choices, u8 selection, u8 y)
{
    s32 widthLeft, widthMid, widthRight, xMid;
    u8 styles[3];

    styles[0] = 0;
    styles[1] = 0;
    styles[2] = 0;
    styles[selection] = 1;

    DrawChoice(choices[0], 104, y, styles[0]);

    widthLeft = GetStringWidth(FONT_NORMAL, choices[0], 0);
    widthMid = GetStringWidth(FONT_NORMAL, choices[1], 0);
    widthRight = GetStringWidth(FONT_NORMAL, choices[2], 0);

    widthMid -= 94;
    xMid = (widthLeft - widthMid - widthRight) / 2 + 104;
    DrawChoice(choices[1], xMid, y, styles[1]);

    DrawChoice(choices[2], GetStringRightAlignXOffset(FONT_NORMAL, choices[2], 198), y, styles[2]);
}

static void Encounters_DrawChoices(u8 selection)
{
    DrawThreeChoices(sEncounterChoices, selection, YPOS_ENCOUNTERS);
}

static void Difficulty_DrawChoices(u8 selection)
{
    DrawThreeChoices(sDifficultyChoices, selection, YPOS_DIFFICULTY);
}

static void DrawHeaderText(void)
{
    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, sText_Header, 8, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
}

static void DrawRunSettingsTexts(void)
{
    u8 i;

    FillWindowPixelBuffer(WIN_OPTIONS, PIXEL_FILL(1));
    for (i = 0; i < MENUITEM_COUNT; i++)
        AddTextPrinterParameterized(WIN_OPTIONS, FONT_NORMAL, sRunSettingsItemNames[i], 8, (i * 16) + 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_OPTIONS, COPYWIN_FULL);
}

#define TILE_TOP_CORNER_L 0x1A2
#define TILE_TOP_EDGE     0x1A3
#define TILE_TOP_CORNER_R 0x1A4
#define TILE_LEFT_EDGE    0x1A5
#define TILE_RIGHT_EDGE   0x1A7
#define TILE_BOT_CORNER_L 0x1A8
#define TILE_BOT_EDGE     0x1A9
#define TILE_BOT_CORNER_R 0x1AA

static void DrawBgWindowFrames(void)
{
    //                     bg, tile,              x, y, width, height, palNum
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1,  0,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2,  0, 27,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28,  0,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1,  1,  1,  2,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28,  1,  1,  2,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1,  3,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2,  3, 27,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28,  3,  1,  1,  7);

    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1,  4,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2,  4, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28,  4,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1,  5,  1, 18,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28,  5,  1, 18,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1, 19,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2, 19, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28, 19,  1,  1,  7);

    CopyBgTilemapBufferToVram(1);
}

// Saves made before the difficulty var was enabled hold 0 in it, which reads as
// DIFFICULTY_EASY. Anything that never went through this menu is set to Normal,
// the level those saves were played at.
void RunSettings_EnsureInitialized(void)
{
    if (FlagGet(FLAG_RUN_SETTINGS_SET))
        return;

    SetCurrentDifficultyLevel(DIFFICULTY_NORMAL);
    FlagSet(FLAG_RUN_SETTINGS_SET);
}
