#ifndef GUARD_WILD_ENCOUNTER_H
#define GUARD_WILD_ENCOUNTER_H

#include "rtc.h"
#include "constants/wild_encounter.h"
#include "wild_encounter_ow.h"

#define HEADER_NONE 0xFFFF

// Which set of encounter tables a map uses. Stored in VAR_ENCOUNTER_MODE.
// A map can define extra tables by adding entries to wild_encounters.json whose
// base_label is the vanilla one plus a mode suffix (e.g. gRoute101 -> gRoute101_Modern),
// placed right after the vanilla entry of the same map.
enum EncounterMode {
    ENCOUNTER_MODE_VANILLA,   // Vanilla Emerald tables
    ENCOUNTER_MODE_MODERN,    // Alternate tables, always
    ENCOUNTER_MODE_POST_GAME, // Vanilla tables until the player becomes Champion, alternate ones afterwards
    ENCOUNTER_MODE_COUNT
};

// How many headers to skip past the vanilla one for each mode
#define ENCOUNTER_MODE_TABLE_OFFSET 1

enum WildPokemonArea {
    WILD_AREA_LAND,
    WILD_AREA_WATER,
    WILD_AREA_ROCKS,
    WILD_AREA_FISHING,
    WILD_AREA_HIDDEN
};

struct WildPokemon
{
    u8 minLevel;
    u8 maxLevel;
    enum Species species;
};

struct WildPokemonInfo
{
    u8 encounterRate;
    const struct WildPokemon *wildPokemon;
};

struct WildEncounterTypes
{
    const struct WildPokemonInfo *landMonsInfo;
    const struct WildPokemonInfo *waterMonsInfo;
    const struct WildPokemonInfo *rockSmashMonsInfo;
    const struct WildPokemonInfo *fishingMonsInfo;
    const struct WildPokemonInfo *hiddenMonsInfo;
};

struct WildPokemonHeader
{
    u8 mapGroup;
    u8 mapNum;
    const struct WildEncounterTypes encounterTypes[TIMES_OF_DAY_COUNT];
};


extern const struct WildPokemonHeader gWildMonHeaders[];
extern const struct WildPokemonHeader gBattlePikeWildMonHeaders[];
extern const struct WildPokemonHeader gBattlePyramidWildMonHeaders[];
extern const struct WildPokemon gWildFeebas;
extern bool8 gIsFishingEncounter;
extern bool8 gIsSurfingEncounter;
extern u8 gChainFishingDexNavStreak;

u32 ApplyEncounterModeToHeaderId(u32 headerId);
bool32 IsWildMonHeaderActiveForEncounterMode(u32 headerId);
u8 ChooseWildMonLevel(const struct WildPokemon *wildPokemon, u8 wildMonIndex, enum WildPokemonArea area);
void DisableWildEncounters(bool8 disabled);
bool8 StandardWildEncounter(u16 curMetatileBehavior, u16 prevMetatileBehavior);
bool8 SweetScentWildEncounter(void);
bool8 DoesCurrentMapHaveFishingMons(void);
void FishingWildEncounter(u8 rod);
u16 GetLocalWildMon(bool8 *isWaterMon);
u16 GetLocalWaterMon(void);
bool8 UpdateRepelCounter(void);
bool8 IsWildLevelAllowedByRepel(u8 wildLevel);
bool8 IsAbilityAllowingEncounter(u8 level);
bool8 TryDoDoubleWildBattle(void);
bool8 StandardWildEncounter_Debug(void);
u32 CalculateChainFishingShinyRolls(void);
void CreateWildMon(enum Species species, u8 level);
bool8 TryGenerateWildMon(const struct WildPokemonInfo *wildMonInfo, enum WildPokemonArea area, u8 flags);
bool8 SetUpMassOutbreakEncounter(u8 flags);
bool8 DoMassOutbreakEncounterTest(void);
bool8 AreLegendariesInSootopolisPreventingEncounters(void);
u16 GetCurrentMapWildMonHeaderId(void);
bool8 CheckFeebasAtCoords(s16 x, s16 y);
u32 ChooseWildMonIndex_Land(void);
u32 ChooseWildMonIndex_Water(void);
u32 ChooseWildMonIndex_Rocks(void);
u32 ChooseHiddenMonIndex(void);
bool32 MapHasNoEncounterData(void);
enum TimeOfDay GetTimeOfDayForEncounters(u32 headerId, enum WildPokemonArea area);

u8 GetLandEncounterSlotForMatchCall(void);
u8 GetWaterEncounterSlotForMatchCall(void);

#endif // GUARD_WILD_ENCOUNTER_H
