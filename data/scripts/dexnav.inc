@ Handed over by whichever rival the player battles at Rustboro, on either of
@ the two maps that encounter can happen on, and only the first time.
Common_EventScript_GiveDexNav::
	goto_if_set FLAG_SYS_DEXNAV_GET, Common_EventScript_DexNavAlreadyGiven
	setflag FLAG_SYS_DEXNAV_GET
	playfanfare MUS_OBTAIN_ITEM
	msgbox Common_Text_ReceivedDexNav, MSGBOX_DEFAULT
	waitfanfare
	closemessage
Common_EventScript_DexNavAlreadyGiven:
	return

Common_Text_ReceivedDexNav:
	.string "{PLAYER} received the DexNav!\p"
	.string "Open the START menu to search for\n"
	.string "POKéMON hiding nearby!$"

EventScript_StartDexNavBattle::
	lock
	playse SE_PIN
	applymovement OBJ_EVENT_ID_PLAYER Common_Movement_ExclamationMark
	waitmovement 0
	waitse
	dowildbattle	
	release
	end

EventScript_NotFoundNearby::
	msgbox sText_NotFoundNearby, MSGBOX_SIGN
	end

EventScript_MovedTooFast::
	msgbox sText_TryMovingSlower, MSGBOX_SIGN
	end

EventScript_PokemonGotAway::
	msgbox sText_PokemonGotAway, MSGBOX_SIGN
	end

EventScript_LostSignal::
	msgbox sText_LostSignal, MSGBOX_SIGN
	end

EventScript_TooDark::
	msgbox sText_TooDark, MSGBOX_SIGN
	end

sText_NotFoundNearby:
	.string "It couldn't be found nearby.\n"
	.string "Try looking in a different area!$"

sText_TryMovingSlower:
	.string "The Pokémon got away!\n"
	.string "Try moving more slowly.$"

sText_PokemonGotAway:
	.string "The Pokémon got away!$"

sText_LostSignal:
	.string "There is no reaction.\n"
	.string "The signal was lost!$"

sText_TooDark:
	.string "It's too dark to search\nfor a Pokémon!$"
