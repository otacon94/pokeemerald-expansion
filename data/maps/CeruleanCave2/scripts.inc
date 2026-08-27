CeruleanCave2_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, CeruleanCave2_OnTransition
	map_script MAP_SCRIPT_ON_RESUME, CeruleanCave2_OnTransition
	.byte 0

CeruleanCave2_OnTransition::
	goto_if_eq VAR_ENCOUNTER_MODE, 0, CeruleanCave2_HideFossilsFromNativeEncounters @ 0 = incontri vanilla
	end

CeruleanCave2_HideFossilsFromNativeEncounters::
	setflag FLAG_HELIX_FOSSIL_ALTERING
	setflag FLAG_OLD_AMBER_ALTERING_CAVE
	setflag FLAG_DOME_FOSSIL_ALTERING_CAVE
	end
