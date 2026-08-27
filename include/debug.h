#ifndef GUARD_DEBUG_H
#define GUARD_DEBUG_H

void Debug_ShowMainMenu(void);
extern const u8 Debug_FlagsAndVarNotSetBattleConfigMessage[];
const u8 *GetWeatherName(u32 weatherId);
const struct Trainer* GetDebugAiTrainer(void);

void DebugNative_GetAbilityNames(void);
void DebugNative_Party_SetFriendship(void);
void DebugNative_Party_ChangeGender(void);
void DebugNative_Party_ToggleShiny(void);
void DebugNative_Party_SetNature(void);
void DebugNative_Party_SetLevel(void);
void DebugNative_Party_SetIVs(void);
void DebugNative_Party_SetEVs(void);
void DebugNative_Party_PushMoveSlots(void);
void DebugNative_Party_PushLegalMoves(void);
void DebugNative_Party_SetMove(void);
void DebugNative_Party_DeleteMove(void);
void DebugNative_Party_PrepareRelease(void);
void DebugNative_Party_ReleaseMon(void);
void DebugNative_Storage_PushBoxes(void);
void DebugNative_Storage_PushBoxMons(void);
void DebugNative_Storage_BufferMonName(void);
void DebugNative_Storage_ReleaseMon(void);

extern EWRAM_DATA bool8 gIsDebugBattle;
extern EWRAM_DATA u64 gDebugAIFlags;


void DebugPkmCreator_Init(u8 mode, u8 index);
#endif // GUARD_DEBUG_H
