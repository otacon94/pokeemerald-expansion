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
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "wild_encounter.h"
#include "window.h"
#include "constants/difficulty.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define tSelection data[0]
#define tPage      data[1]

// Every setting is backed by a single flag or var, so nothing needs to be cached:
// a change is applied as soon as it is made.
enum SettingKind
{
    SETTING_FLAG,      // choice 0 = flag clear, choice 1 = flag set
    SETTING_FLAG_INV,  // same, but the choice labels are reversed
    SETTING_VAR,       // choice = var value
};

struct RunSetting
{
    const u8 *name;
    const u8 *description;
    const u8 *const *choices;
    u8 choiceCount;
    u8 kind;
    u16 id;
};

#define SETTINGS_PER_PAGE 5
#define PAGE_COUNT 4
// The last row of every page leaves the menu
#define ROW_DONE SETTINGS_PER_PAGE

enum
{
    WIN_HEADER,
    WIN_OPTIONS,
    WIN_DESCRIPTION
};

static void Task_RunSettingsFadeIn(u8 taskId);
static void Task_RunSettingsProcessInput(u8 taskId);
static void Task_RunSettingsFadeOut(u8 taskId);
static void HighlightRunSettingsItem(u8 selection);
static void DrawHeaderText(u8 page);
static void DrawRunSettingsTexts(u8 page);
static void DrawSettingValue(u8 page, u8 row);
static void DrawDescription(u8 page, u8 row);
static void DrawBgWindowFrames(void);
static u16 GetSettingValue(const struct RunSetting *setting);
static void SetSettingValue(const struct RunSetting *setting, u16 value);

static const u8 sText_Header[] = _("RUN SETTINGS");
static const u8 sText_PageOf[] = _("{STR_VAR_1}/{STR_VAR_2} {L_BUTTON}{R_BUTTON}");
static const u8 sText_Done[]   = _("DONE");

static const u8 sText_Off[]        = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}OFF");
static const u8 sText_On[]         = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ON");
static const u8 sText_EncVanilla[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}CLASSIC");
static const u8 sText_EncModern[]  = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}MODERN");
static const u8 sText_EncPost[]    = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}POST-GAME");
static const u8 sText_DiffEasy[]   = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}EASY");
static const u8 sText_DiffNormal[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}NORMAL");
static const u8 sText_DiffHard[]   = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}HARD");
static const u8 sText_BagAlways[]  = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ALWAYS");
static const u8 sText_BagTrainer[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}NOT VS TRAINERS");
static const u8 sText_BagNever[]   = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}NEVER");
static const u8 sText_Shiny8192[]  = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}1/8192");
static const u8 sText_Shiny4096[]  = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}1/4096");
static const u8 sText_Shiny2048[]  = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}1/2048");
static const u8 sText_Shiny1024[]  = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}1/1024");
static const u8 sText_Shiny512[]   = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}1/512");
static const u8 sText_EvNormal[]   = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}NORMAL");
static const u8 sText_EvBadges[]   = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}BY BADGES");
static const u8 sText_EvNone[]     = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}NONE");
static const u8 sText_ShinyNormal[]= _("{COLOR GREEN}{SHADOW LIGHT_GREEN}NORMAL");
static const u8 sText_ShinyAlways[]= _("{COLOR GREEN}{SHADOW LIGHT_GREEN}ALWAYS");
static const u8 sText_ShinyNever[] = _("{COLOR GREEN}{SHADOW LIGHT_GREEN}NEVER");

static const u8 sName_Encounters[] = _("ENCOUNTERS");
static const u8 sName_Difficulty[] = _("DIFFICULTY");
static const u8 sName_LevelCap[]   = _("LEVEL CAP");
static const u8 sName_EvCap[]      = _("EV CAP");
static const u8 sName_ExpShare[]   = _("EXP SHARE");
static const u8 sName_DexNav[]     = _("DEXNAV");
static const u8 sName_SleepClause[]= _("SLEEP CLAUSE");
static const u8 sName_Inverse[]    = _("INVERSE TYPES");
static const u8 sName_Bag[]        = _("BAG IN BATTLE");
static const u8 sName_Catching[]   = _("CATCHING");
static const u8 sName_WhiteOut[]   = _("WHITE OUT");
static const u8 sName_DoubleWild[] = _("DOUBLE WILDS");
static const u8 sName_WildBattles[]= _("WILD BATTLES");
static const u8 sName_ShinyRate[]  = _("SHINY RATE");
static const u8 sName_Followers[]  = _("FOLLOWERS");
static const u8 sName_EggMoves[]   = _("EGG MOVES");
static const u8 sName_TutorMoves[] = _("TUTOR MOVES");
static const u8 sName_IvEvInfo[]   = _("IV/EV INFO");
static const u8 sName_ShinyOdds[]  = _("SHINY ODDS");
static const u8 sName_EvGain[]     = _("EV GAIN");
static const u8 sName_SmartAi[]    = _("SMART WILD AI");


static const u8 sDesc_Encounters[]  = _("Which wild Pokémon tables are used.\nPOST-GAME switches after the League.");
static const u8 sDesc_Difficulty[]  = _("Trainers use their easy, normal or hard\nteam where one is defined.");
static const u8 sDesc_LevelCap[]    = _("Pokémon above the badge level cap\nstop gaining experience.");
static const u8 sDesc_EvGain[]      = _("BY BADGES raises the EV limit as you\nearn badges. NONE blocks EVs entirely.");
static const u8 sDesc_ExpShare[]    = _("The whole party gains experience,\neven Pokémon that did not fight.");
static const u8 sDesc_DexNav[]      = _("Shows the DexNav in the start menu\nto track wild Pokémon.");
static const u8 sDesc_SleepClause[] = _("Only one Pokémon per side can be\nasleep at a time.");
static const u8 sDesc_Inverse[]     = _("Type match-ups are reversed:\nfire beats water, and so on.");
static const u8 sDesc_Bag[]         = _("When the bag can be used in battle.\nItems from the party still work.");
static const u8 sDesc_Catching[]    = _("Turn off to forbid catching\nwild Pokémon.");
static const u8 sDesc_WhiteOut[]    = _("Turn off and losing to a trainer no\nlonger sends you back. No auto healing.");
static const u8 sDesc_DoubleWild[]  = _("Wild Pokémon can show up in pairs\nfor double battles.");
static const u8 sDesc_WildBattles[] = _("Turn off to walk through grass and\nwater without any wild battles.");
static const u8 sDesc_ShinyOdds[]   = _("Chance of a wild Pokémon being shiny.\nDoes not change Pokémon you own.");
static const u8 sDesc_ShinyRate[]   = _("ALWAYS and NEVER force every wild and\ngift Pokémon, ignoring the odds above.");
static const u8 sDesc_Followers[]   = _("Your first Pokémon walks behind you\non the overworld.");
static const u8 sDesc_EggMoves[]    = _("The move relearner can also teach\negg moves.");
static const u8 sDesc_TutorMoves[]  = _("The move relearner can also teach\ntutor moves.");
static const u8 sDesc_IvEvInfo[]    = _("Cycle stats, IVs and EVs on the\nsummary screen skills page.");
static const u8 sDesc_SmartAi[]     = _("Wild Pokémon use the same battle AI\nas trainers.");
static const u8 sDesc_Done[]        = _("Leave the menu. Every choice is\nsaved as soon as it is made.");

static const u8 *const sChoices_OffOn[] = { sText_Off, sText_On };
static const u8 *const sChoices_OnOff[] = { sText_On, sText_Off };
static const u8 *const sChoices_Enc[]   = { sText_EncVanilla, sText_EncModern, sText_EncPost };
static const u8 *const sChoices_Diff[]  = { sText_DiffEasy, sText_DiffNormal, sText_DiffHard };
static const u8 *const sChoices_Bag[]   = { sText_BagAlways, sText_BagTrainer, sText_BagNever };
static const u8 *const sChoices_Shiny[] = { sText_ShinyNormal, sText_ShinyAlways, sText_ShinyNever };
static const u8 *const sChoices_ShinyOdds[] = { sText_Shiny8192, sText_Shiny4096, sText_Shiny2048, sText_Shiny1024, sText_Shiny512 };
static const u8 *const sChoices_EvGain[] = { sText_EvNormal, sText_EvBadges, sText_EvNone };

// Page 1: how the run plays. Page 2: battle rules. Page 3: wild Pokemon and comforts.
static const struct RunSetting sRunSettings[PAGE_COUNT][SETTINGS_PER_PAGE] =
{
    // Page 1: how the run plays
    {
        { sName_Encounters,  sDesc_Encounters,  sChoices_Enc,       ARRAY_COUNT(sChoices_Enc),       SETTING_VAR,      VAR_ENCOUNTER_MODE },
        { sName_Difficulty,  sDesc_Difficulty,  sChoices_Diff,      ARRAY_COUNT(sChoices_Diff),      SETTING_VAR,      VAR_RUN_DIFFICULTY },
        { sName_LevelCap,    sDesc_LevelCap,    sChoices_OffOn,     ARRAY_COUNT(sChoices_OffOn),     SETTING_FLAG,     FLAG_LEVEL_CAP },
        { sName_EvGain,      sDesc_EvGain,      sChoices_EvGain,    ARRAY_COUNT(sChoices_EvGain),    SETTING_VAR,      VAR_EV_CAP_MODE },
        { sName_ExpShare,    sDesc_ExpShare,    sChoices_OffOn,     ARRAY_COUNT(sChoices_OffOn),     SETTING_FLAG,     FLAG_EXP_SHARE_ON },
    },
    // Page 2: battle rules
    {
        { sName_SleepClause, sDesc_SleepClause, sChoices_OffOn,     ARRAY_COUNT(sChoices_OffOn),     SETTING_FLAG,     FLAG_SLEEP_CLAUSE },
        { sName_Inverse,     sDesc_Inverse,     sChoices_OffOn,     ARRAY_COUNT(sChoices_OffOn),     SETTING_FLAG,     FLAG_INVERSE_BATTLE },
        { sName_Bag,         sDesc_Bag,         sChoices_Bag,       ARRAY_COUNT(sChoices_Bag),       SETTING_VAR,      VAR_NO_BAG_USE },
        { sName_Catching,    sDesc_Catching,    sChoices_OnOff,     ARRAY_COUNT(sChoices_OnOff),     SETTING_FLAG_INV, FLAG_NO_CATCHING },
        { sName_WhiteOut,    sDesc_WhiteOut,    sChoices_OnOff,     ARRAY_COUNT(sChoices_OnOff),     SETTING_FLAG_INV, FLAG_NO_WHITEOUT },
    },
    // Page 3: wild Pokemon
    {
        { sName_WildBattles, sDesc_WildBattles, sChoices_OnOff,     ARRAY_COUNT(sChoices_OnOff),     SETTING_FLAG_INV, FLAG_NO_WILD_ENCOUNTERS },
        { sName_DoubleWild,  sDesc_DoubleWild,  sChoices_OffOn,     ARRAY_COUNT(sChoices_OffOn),     SETTING_FLAG,     FLAG_DOUBLE_WILD },
        { sName_SmartAi,     sDesc_SmartAi,     sChoices_OffOn,     ARRAY_COUNT(sChoices_OffOn),     SETTING_FLAG,     FLAG_SMART_WILD_AI },
        { sName_ShinyOdds,   sDesc_ShinyOdds,   sChoices_ShinyOdds, ARRAY_COUNT(sChoices_ShinyOdds), SETTING_VAR,      VAR_SHINY_ODDS },
        { sName_ShinyRate,   sDesc_ShinyRate,   sChoices_Shiny,     ARRAY_COUNT(sChoices_Shiny),     SETTING_VAR,      VAR_SHINY_RATE },
    },
    // Page 4: comforts
    {
        { sName_DexNav,      sDesc_DexNav,      sChoices_OffOn,     ARRAY_COUNT(sChoices_OffOn),     SETTING_FLAG,     DN_FLAG_DEXNAV_GET },
        { sName_Followers,   sDesc_Followers,   sChoices_OnOff,     ARRAY_COUNT(sChoices_OnOff),     SETTING_FLAG_INV, FLAG_FOLLOWERS_DISABLED },
        { sName_EggMoves,    sDesc_EggMoves,    sChoices_OffOn,     ARRAY_COUNT(sChoices_OffOn),     SETTING_FLAG,     FLAG_RELEARN_EGG_MOVES },
        { sName_TutorMoves,  sDesc_TutorMoves,  sChoices_OffOn,     ARRAY_COUNT(sChoices_OffOn),     SETTING_FLAG,     FLAG_RELEARN_TUTOR_MOVES },
        { sName_IvEvInfo,    sDesc_IvEvInfo,    sChoices_OffOn,     ARRAY_COUNT(sChoices_OffOn),     SETTING_FLAG,     FLAG_SUMMARY_IV_EV_INFO },
    },
};

static const u16 sRunSettingsText_Pal[] = INCGFX_U16("graphics/interface/option_menu_text.pal", ".gbapal");

static const struct WindowTemplate sRunSettingsWinTemplates[] =
{
    [WIN_HEADER] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 0,
        .width = 26,
        .height = 2,
        .paletteNum = 1,
        .baseBlock = 2
    },
    [WIN_OPTIONS] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 3,
        .width = 26,
        .height = 10,
        .paletteNum = 1,
        .baseBlock = 0x36
    },
    [WIN_DESCRIPTION] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 14,
        .width = 26,
        .height = 5,
        .paletteNum = 1,
        .baseBlock = 0x140
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
        DrawHeaderText(0);
        gMain.state++;
        break;
    case 7:
        gMain.state++;
        break;
    case 8:
        PutWindowTilemap(WIN_OPTIONS);
        PutWindowTilemap(WIN_DESCRIPTION);
        DrawRunSettingsTexts(0);
        gMain.state++;
    case 9:
        DrawBgWindowFrames();
        gMain.state++;
        break;
    case 10:
    {
        u8 taskId = CreateTask(Task_RunSettingsFadeIn, 0);

        gTasks[taskId].tSelection = 0;
        gTasks[taskId].tPage = 0;
        HighlightRunSettingsItem(0);
        DrawDescription(0, 0);
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

static u16 GetSettingValue(const struct RunSetting *setting)
{
    u16 value;

    switch (setting->kind)
    {
    case SETTING_FLAG:
    case SETTING_FLAG_INV:
        return FlagGet(setting->id) ? 1 : 0;
    case SETTING_VAR:
    default:
        value = VarGet(setting->id);
        return value < setting->choiceCount ? value : 0;
    }
}

static void SetSettingValue(const struct RunSetting *setting, u16 value)
{
    switch (setting->kind)
    {
    case SETTING_FLAG:
    case SETTING_FLAG_INV:
        if (value)
            FlagSet(setting->id);
        else
            FlagClear(setting->id);
        break;
    case SETTING_VAR:
        VarSet(setting->id, value);
        break;
    }

    // Two settings drive more than the flag or var they are stored in
    if (setting->id == VAR_RUN_DIFFICULTY)
    {
        if (value == DIFFICULTY_HARD)
            FlagSet(FLAG_DIFFICULTY_HARD);
        else
            FlagClear(FLAG_DIFFICULTY_HARD);
    }
    else if (setting->id == VAR_SHINY_RATE)
    {
        FlagClear(FLAG_FORCE_SHINY);
        FlagClear(FLAG_FORCE_NO_SHINY);
        if (value == 1)
            FlagSet(FLAG_FORCE_SHINY);
        else if (value == 2)
            FlagSet(FLAG_FORCE_NO_SHINY);
    }
}

static void Task_RunSettingsFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_RunSettingsProcessInput;
}

static void ChangePage(u8 taskId, s8 delta)
{
    gTasks[taskId].tPage = (gTasks[taskId].tPage + PAGE_COUNT + delta) % PAGE_COUNT;
    gTasks[taskId].tSelection = 0;
    DrawHeaderText(gTasks[taskId].tPage);
    DrawRunSettingsTexts(gTasks[taskId].tPage);
    HighlightRunSettingsItem(0);
    DrawDescription(gTasks[taskId].tPage, 0);
    PlaySE(SE_SELECT);
}

static void CloseMenu(u8 taskId)
{
    FlagSet(FLAG_RUN_SETTINGS_SET);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_RunSettingsFadeOut;
}

static void Task_RunSettingsProcessInput(u8 taskId)
{
    u8 page = gTasks[taskId].tPage;
    u8 row = gTasks[taskId].tSelection;
    const struct RunSetting *setting;
    u16 value;

    if (JOY_NEW(A_BUTTON) && row == ROW_DONE)
    {
        CloseMenu(taskId);
    }
    else if (JOY_NEW(B_BUTTON))
    {
        CloseMenu(taskId);
    }
    else if (JOY_NEW(L_BUTTON))
    {
        ChangePage(taskId, -1);
    }
    else if (JOY_NEW(R_BUTTON))
    {
        ChangePage(taskId, 1);
    }
    else if (JOY_NEW(DPAD_UP))
    {
        gTasks[taskId].tSelection = (row == 0) ? ROW_DONE : row - 1;
        HighlightRunSettingsItem(gTasks[taskId].tSelection);
        DrawDescription(page, gTasks[taskId].tSelection);
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        gTasks[taskId].tSelection = (row == ROW_DONE) ? 0 : row + 1;
        HighlightRunSettingsItem(gTasks[taskId].tSelection);
        DrawDescription(page, gTasks[taskId].tSelection);
    }
    else if (row != ROW_DONE && (JOY_NEW(DPAD_LEFT) || JOY_NEW(DPAD_RIGHT)))
    {
        setting = &sRunSettings[page][row];
        value = GetSettingValue(setting);

        if (JOY_NEW(DPAD_RIGHT))
            value = (value + 1) % setting->choiceCount;
        else
            value = (value + setting->choiceCount - 1) % setting->choiceCount;

        SetSettingValue(setting, value);
        DrawSettingValue(page, row);
        CopyWindowToVram(WIN_OPTIONS, COPYWIN_GFX);
        PlaySE(SE_SELECT);
    }
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

// The description of the highlighted row, in the window under the list
static void DrawDescription(u8 page, u8 row)
{
    const u8 *text = (row == ROW_DONE) ? sDesc_Done : sRunSettings[page][row].description;

    FillWindowPixelBuffer(WIN_DESCRIPTION, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_DESCRIPTION, FONT_NORMAL, text, 8, 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_DESCRIPTION, COPYWIN_FULL);
}

static void HighlightRunSettingsItem(u8 index)
{
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(16, DISPLAY_WIDTH - 16));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(index * 16 + 24, index * 16 + 40));
}

// Only the current choice is drawn, right aligned, so even long labels fit
static void DrawSettingValue(u8 page, u8 row)
{
    const struct RunSetting *setting = &sRunSettings[page][row];
    const u8 *text = setting->choices[GetSettingValue(setting)];
    u8 y = row * 16;

    FillWindowPixelRect(WIN_OPTIONS, PIXEL_FILL(1), 96, y, 104, 16);
    AddTextPrinterParameterized(WIN_OPTIONS, FONT_NORMAL, text,
                                GetStringRightAlignXOffset(FONT_NORMAL, text, 198), y + 1, TEXT_SKIP_DRAW, NULL);
}

static void DrawHeaderText(u8 page)
{
    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(1));
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, sText_Header, 8, 1, TEXT_SKIP_DRAW, NULL);
    ConvertIntToDecimalStringN(gStringVar1, page + 1, STR_CONV_MODE_LEFT_ALIGN, 1);
    ConvertIntToDecimalStringN(gStringVar2, PAGE_COUNT, STR_CONV_MODE_LEFT_ALIGN, 1);
    StringExpandPlaceholders(gStringVar4, sText_PageOf);
    AddTextPrinterParameterized(WIN_HEADER, FONT_NORMAL, gStringVar4,
                                GetStringRightAlignXOffset(FONT_NORMAL, gStringVar4, 198), 1, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
}

static void DrawRunSettingsTexts(u8 page)
{
    u8 i;

    FillWindowPixelBuffer(WIN_OPTIONS, PIXEL_FILL(1));
    for (i = 0; i < SETTINGS_PER_PAGE; i++)
    {
        AddTextPrinterParameterized(WIN_OPTIONS, FONT_NORMAL, sRunSettings[page][i].name, 8, (i * 16) + 1, TEXT_SKIP_DRAW, NULL);
        DrawSettingValue(page, i);
    }
    AddTextPrinterParameterized(WIN_OPTIONS, FONT_NORMAL, sText_Done, 8, (ROW_DONE * 16) + 1, TEXT_SKIP_DRAW, NULL);
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
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1,  1,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28,  1,  1,  1,  7);

    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_L,  1,  2,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_EDGE,      2,  2, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_TOP_CORNER_R, 28,  2,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1,  3,  1, 10,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28,  3,  1, 10,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1, 13,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2, 13, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28, 13,  1,  1,  7);

    // Description box
    FillBgTilemapBufferRect(1, TILE_LEFT_EDGE,     1, 14,  1,  5,  7);
    FillBgTilemapBufferRect(1, TILE_RIGHT_EDGE,   28, 14,  1,  5,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_L,  1, 19,  1,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_EDGE,      2, 19, 26,  1,  7);
    FillBgTilemapBufferRect(1, TILE_BOT_CORNER_R, 28, 19,  1,  1,  7);

    CopyBgTilemapBufferToVram(1);
}

// Saves made before these settings existed hold 0 in the difficulty var, which reads
// as DIFFICULTY_EASY. Anything that never went through this menu is set to Normal,
// the level those saves were played at.
void RunSettings_EnsureInitialized(void)
{
    if (FlagGet(FLAG_RUN_SETTINGS_SET))
        return;

    SetCurrentDifficultyLevel(DIFFICULTY_NORMAL);
    FlagSet(FLAG_RUN_SETTINGS_SET);
}
