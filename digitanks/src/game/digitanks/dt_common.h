#ifndef DT_COMMON
#define DT_COMMON

#include <EASTL/string.h>
#include <color.h>

typedef enum
{
	MODE_NONE = 0,
	MODE_MOVE,
	MODE_TURN,
	MODE_AIM,
	MODE_BUILD,
} controlmode_t;

typedef enum
{
	AIM_NONE = 0,
	AIM_NORMAL,
	AIM_NORANGE,
	AIM_MOVEMENT,
} aimtype_t;

typedef enum
{
	MENUMODE_MAIN,
	MENUMODE_PROMOTE,
	MENUMODE_LOADERS,
} menumode_t;

typedef enum
{
	UPDATECLASS_EMPTY = 0,
	UPDATECLASS_STRUCTURE,
	UPDATECLASS_STRUCTUREUPDATE,
	UPDATECLASS_UNITSKILL,
} updateclass_t;

typedef enum
{
	UPDATETYPE_NONE = 0,
	UPDATETYPE_PRODUCTION,
	UPDATETYPE_BANDWIDTH,
	UPDATETYPE_FLEETSUPPLY,
	UPDATETYPE_SUPPORTENERGY,
	UPDATETYPE_SUPPORTRECHARGE,
	UPDATETYPE_TANKATTACK,
	UPDATETYPE_TANKDEFENSE,
	UPDATETYPE_TANKMOVEMENT,
	UPDATETYPE_TANKHEALTH,
	UPDATETYPE_TANKRANGE,
	UPDATETYPE_SIZE,
	UPDATETYPE_SKILL_CLOAK,
	UPDATETYPE_WEAPON_CHARGERAM,
	UPDATETYPE_WEAPON_AOE,
	UPDATETYPE_WEAPON_CLUSTER,
	UPDATETYPE_WEAPON_ICBM,
	UPDATETYPE_WEAPON_DEVASTATOR,
} updatetype_t;

typedef enum
{
	UNITTYPE_UNDEFINED = 0,
	STRUCTURE_CPU,
	STRUCTURE_MINIBUFFER,
	STRUCTURE_BUFFER,
	STRUCTURE_BATTERY,
	STRUCTURE_PSU,
	STRUCTURE_INFANTRYLOADER,
	STRUCTURE_TANKLOADER,
	STRUCTURE_ARTILLERYLOADER,
	STRUCTURE_ELECTRONODE,
	STRUCTURE_AUTOTURRET,
	UNIT_MOBILECPU,
	UNIT_INFANTRY,
	UNIT_TANK,
	UNIT_ARTILLERY,
	UNIT_SCOUT,
	UNIT_BUGTURRET,
	UNIT_GRIDBUG,
	MAX_UNITS,
} unittype_t;

typedef enum
{
	WEAPON_NONE = 0,
	PROJECTILE_SMALL,
	PROJECTILE_MEDIUM,
	PROJECTILE_LARGE,
	PROJECTILE_AOE,
	PROJECTILE_EMP,
	PROJECTILE_TRACTORBOMB,
	PROJECTILE_SPLOOGE,
	PROJECTILE_ICBM,
	PROJECTILE_GRENADE,
	PROJECTILE_DAISYCHAIN,
	PROJECTILE_CLUSTERBOMB,
	PROJECTILE_EARTHSHAKER,

	// Strategy mode projectiles
	PROJECTILE_FLAK,		// For infantry
	PROJECTILE_TREECUTTER,	// For infantry
	WEAPON_INFANTRYLASER,
	PROJECTILE_TORPEDO,		// For scouts
	PROJECTILE_ARTILLERY,	// For guess who
	PROJECTILE_ARTILLERY_AOE,// For guess who
	PROJECTILE_ARTILLERY_ICBM,// For guess who
	PROJECTILE_DEVASTATOR,	// Artillery weapon

	// Special/odd weapons
	PROJECTILE_CAMERAGUIDED,
	WEAPON_LASER,
	WEAPON_MISSILEDEFENSE,
	WEAPON_TURRETMISSILE,
	WEAPON_CHARGERAM,

	PROJECTILE_AIRSTRIKE,
	PROJECTILE_FIREWORKS,

	// If you add them here, add their energy prices too in projectile.cpp
	WEAPON_MAX,
} weapon_t;

typedef enum
{
	LOSE_NOTANKS,
	LOSE_NOCPU,
} losecondition_t;

extern Color g_aclrTeamColors[];
extern eastl::string16 g_aszTeamNames[];

#endif
