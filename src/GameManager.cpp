#include "global.h"
#include "StepMania.h"
#include "arch/Dialog/Dialog.h"
#include "GameManager.h"
#include "GameConstantsAndTypes.h"
#include "GameInput.h"	// for GameButton constants
#include "GameLoop.h"  // for ChangeGame
#include "RageLog.h"
#include "RageUtil.h"
#include "NoteSkinManager.h"
#include "RageInputDevice.h"
#include "ThemeManager.h"
#include "LightsManager.h"	// for NUM_CabinetLight
#include "Game.h"
#include "Style.h"

GameManager*	GAMEMAN = nullptr;	// global and accessable from anywhere in our program

enum
{
	TRACK_1 = 0,
	TRACK_2,
	TRACK_3,
	TRACK_4,
	TRACK_5,
	TRACK_6,
	TRACK_7,
	TRACK_8,
	TRACK_9,
	TRACK_10,
	TRACK_11,
	TRACK_12,
	TRACK_13,
	TRACK_14,
	TRACK_15,
	TRACK_16,
	// 16 tracks needed for beat-double7 and techno-double8
};

RString StepsTypeInfo::GetLocalizedString() const
{
	if( THEME->HasString( "StepsType", szName ) )
		return THEME->GetString( "StepsType", szName );
	return szName;
}

static const StepsTypeInfo g_StepsTypeInfos[] = {
	// pump
	{ "pump-single",	5,	true,	StepsTypeCategory_Single },
	{ "pump-halfdouble",	6,	true,	StepsTypeCategory_Double },
	{ "pump-double",	10,	true,	StepsTypeCategory_Double },
	{ "pump-couple",	10,	true,	StepsTypeCategory_Couple },
	{ "pump-routine",	10,	true,	StepsTypeCategory_Routine },
	// cabinet lights and other fine StepsTypes that don't exist lol
	{ "lights-cabinet",	NUM_CabinetLight,	false,	StepsTypeCategory_Single }, // XXX disable lights autogen for now
};


// Important:  Every game must define the buttons: "Start", "Back", "MenuLeft", "Operator" and "MenuRight"
static const AutoMappings g_AutoKeyMappings_Pump = AutoMappings (
	"",
	"",
	"",
	AutoMappingEntry( 0, KEY_Cq,		PUMP_BUTTON_UPLEFT,		false ),
	AutoMappingEntry( 0, KEY_Ce,		PUMP_BUTTON_UPRIGHT,		false ),
	AutoMappingEntry( 0, KEY_Cs,		PUMP_BUTTON_CENTER,		false ),
	AutoMappingEntry( 0, KEY_Cz,		PUMP_BUTTON_DOWNLEFT,		false ),
	AutoMappingEntry( 0, KEY_Cc,		PUMP_BUTTON_DOWNRIGHT,		false ),
	AutoMappingEntry( 0, KEY_KP_C7,		PUMP_BUTTON_UPLEFT,		true ),
	AutoMappingEntry( 0, KEY_KP_C9,		PUMP_BUTTON_UPRIGHT,		true ),
	AutoMappingEntry( 0, KEY_KP_C5,		PUMP_BUTTON_CENTER,		true ),
	AutoMappingEntry( 0, KEY_KP_C1,		PUMP_BUTTON_DOWNLEFT,		true ),
	AutoMappingEntry( 0, KEY_KP_C3,		PUMP_BUTTON_DOWNRIGHT,		true ),

	// unmap confusing default MenuButtons
	AutoMappingEntry( 0, KEY_KP_C8,		GameButton_Invalid,		false ),
	AutoMappingEntry( 0, KEY_KP_C2,		GameButton_Invalid,		false ),
	AutoMappingEntry( 0, KEY_KP_C4,		GameButton_Invalid,		false ),
	AutoMappingEntry( 0, KEY_KP_C6,		GameButton_Invalid,		false )
);

// PIU Defaults: RowSpacing = 60; ColSpacing = 52; ArrowSize = 54;
// apparently column spacing is 48px
//static ThemeMetric<int>	PUMP_COL_SPACING	("ColumnSpacing","Pump");
// xMAx - Changed from 48 to 50
static const int PUMP_COL_SPACING = 50;

static const Style g_Style_Pump_Single =
{	// STYLE_PUMP_SINGLE
	true,				// m_bUsedForGameplay
	true,				// m_bUsedForEdit
	false,				// m_bUsedForDemonstration
	true,				// m_bUsedForHowToPlay
	"single",			// m_szName
	StepsType_pump_single,		// m_StepsType
	StyleType_OnePlayerOneSide,		// m_StyleType
	5,				// m_iColsPerPlayer
	{	// m_ColumnInfo[NUM_PLAYERS][MAX_COLS_PER_PLAYER];
		{	// PLAYER_1
			{ TRACK_1,	-PUMP_COL_SPACING*2.0f, nullptr },
			{ TRACK_2,	-PUMP_COL_SPACING*1.0f, nullptr },
			{ TRACK_3,	+PUMP_COL_SPACING*0.0f, nullptr },
			{ TRACK_4,	+PUMP_COL_SPACING*1.0f, nullptr },
			{ TRACK_5,	+PUMP_COL_SPACING*2.0f, nullptr },
		},
		{	// PLAYER_2
			{ TRACK_1,	-PUMP_COL_SPACING*2.0f, nullptr },
			{ TRACK_2,	-PUMP_COL_SPACING*1.0f, nullptr },
			{ TRACK_3,	+PUMP_COL_SPACING*0.0f, nullptr },
			{ TRACK_4,	+PUMP_COL_SPACING*1.0f, nullptr },
			{ TRACK_5,	+PUMP_COL_SPACING*2.0f, nullptr },
		},
	},
	{	// m_iInputColumn[NUM_GameController][NUM_GameButton]
		{ 1, 3, 2, 0, 4, Style::END_MAPPING },
		{ 1, 3, 2, 0, 4, Style::END_MAPPING },
	},
	{	// m_iColumnDrawOrder[MAX_COLS_PER_PLAYER];
		2,1,3,0,4
	},
	false, // m_bCanUseBeginnerHelper
	false, // m_bLockDifficulties
};

static const Style g_Style_Pump_Versus =
{	// STYLE_PUMP_VERSUS
	true,				// m_bUsedForGameplay
	false,				// m_bUsedForEdit
	true,				// m_bUsedForDemonstration
	false,				// m_bUsedForHowToPlay
	"versus",			// m_szName
	StepsType_pump_single,		// m_StepsType
	StyleType_TwoPlayersTwoSides,		// m_StyleType
	5,				// m_iColsPerPlayer
	{	// m_ColumnInfo[NUM_PLAYERS][MAX_COLS_PER_PLAYER];
		{	// PLAYER_1
			{ TRACK_1,	-PUMP_COL_SPACING*2.0f, nullptr },
			{ TRACK_2,	-PUMP_COL_SPACING*1.0f, nullptr },
			{ TRACK_3,	+PUMP_COL_SPACING*0.0f, nullptr },
			{ TRACK_4,	+PUMP_COL_SPACING*1.0f, nullptr },
			{ TRACK_5,	+PUMP_COL_SPACING*2.0f, nullptr },
		},
		{	// PLAYER_2
			{ TRACK_1,	-PUMP_COL_SPACING*2.0f, nullptr },
			{ TRACK_2,	-PUMP_COL_SPACING*1.0f, nullptr },
			{ TRACK_3,	+PUMP_COL_SPACING*0.0f, nullptr },
			{ TRACK_4,	+PUMP_COL_SPACING*1.0f, nullptr },
			{ TRACK_5,	+PUMP_COL_SPACING*2.0f, nullptr },
		},
	},
	{	// m_iInputColumn[NUM_GameController][NUM_GameButton]
		{ 1, 3, 2, 0, 4, Style::END_MAPPING },
		{ 1, 3, 2, 0, 4, Style::END_MAPPING },
	},
	{	// m_iColumnDrawOrder[MAX_COLS_PER_PLAYER];
		2,1,3,0,4
	},
	false, // m_bCanUseBeginnerHelper
	false, // m_bLockDifficulties
};

static const Style g_Style_Pump_HalfDouble =
{	// STYLE_PUMP_HALFDOUBLE
	true,				// m_bUsedForGameplay
	true,				// m_bUsedForEdit
	false,				// m_bUsedForDemonstration
	false,				// m_bUsedForHowToPlay
	"halfdouble",			// m_szName
	StepsType_pump_halfdouble,	// m_StepsType
	StyleType_OnePlayerTwoSides,		// m_StyleType
	6,				// m_iColsPerPlayer
	{	// m_ColumnInfo[NUM_PLAYERS][MAX_COLS_PER_PLAYER];
		{	// PLAYER_1
			{ TRACK_1,	-PUMP_COL_SPACING*2.5f-4, nullptr },
			{ TRACK_2,	-PUMP_COL_SPACING*1.5f-4, nullptr },
			{ TRACK_3,	-PUMP_COL_SPACING*0.5f-4, nullptr },
			{ TRACK_4,	+PUMP_COL_SPACING*0.5f+4, nullptr },
			{ TRACK_5,	+PUMP_COL_SPACING*1.5f+4, nullptr },
			{ TRACK_6,	+PUMP_COL_SPACING*2.5f+4, nullptr },
		},
		{	// PLAYER_2
			{ TRACK_1,	-PUMP_COL_SPACING*2.5f-4, nullptr },
			{ TRACK_2,	-PUMP_COL_SPACING*1.5f-4, nullptr },
			{ TRACK_3,	-PUMP_COL_SPACING*0.5f-4, nullptr },
			{ TRACK_4,	+PUMP_COL_SPACING*0.5f+4, nullptr },
			{ TRACK_5,	+PUMP_COL_SPACING*1.5f+4, nullptr },
			{ TRACK_6,	+PUMP_COL_SPACING*2.5f+4, nullptr },
		},
	},
	{	// m_iInputColumn[NUM_GameController][NUM_GameButton]
		{ Style::NO_MAPPING, 1, 0, Style::NO_MAPPING, 2, Style::END_MAPPING },
		{ 4, Style::NO_MAPPING, 5, 3, Style::NO_MAPPING, Style::END_MAPPING }
	},
	{	// m_iColumnDrawOrder[MAX_COLS_PER_PLAYER];
		2,3,1,4,0,5
	},
	false, // m_bCanUseBeginnerHelper
	false, // m_bLockDifficulties
};

static const Style g_Style_Pump_Double =
{	// STYLE_PUMP_DOUBLE
	true,				// m_bUsedForGameplay
	true,				// m_bUsedForEdit
	false,				// m_bUsedForDemonstration
	false,				// m_bUsedForHowToPlay
	"double",			// m_szName
	StepsType_pump_double,		// m_StepsType
	StyleType_OnePlayerTwoSides,		// m_StyleType
	10,				// m_iColsPerPlayer
	{	// m_ColumnInfo[NUM_PLAYERS][MAX_COLS_PER_PLAYER];
		{	// PLAYER_1
      // xMAx - Change 4 to 1
			{ TRACK_1,	-PUMP_COL_SPACING*4.5f-1, nullptr },
			{ TRACK_2,	-PUMP_COL_SPACING*3.5f-1, nullptr },
			{ TRACK_3,	-PUMP_COL_SPACING*2.5f-1, nullptr },
			{ TRACK_4,	-PUMP_COL_SPACING*1.5f-1, nullptr },
			{ TRACK_5,	-PUMP_COL_SPACING*0.5f-1, nullptr },
			{ TRACK_6,	+PUMP_COL_SPACING*0.5f+1, nullptr },
			{ TRACK_7,	+PUMP_COL_SPACING*1.5f+1, nullptr },
			{ TRACK_8,	+PUMP_COL_SPACING*2.5f+1, nullptr },
			{ TRACK_9,	+PUMP_COL_SPACING*3.5f+1, nullptr },
			{ TRACK_10,	+PUMP_COL_SPACING*4.5f+1, nullptr },
		},
		{	// PLAYER_2
			{ TRACK_1,	-PUMP_COL_SPACING*4.5f-1, nullptr },
			{ TRACK_2,	-PUMP_COL_SPACING*3.5f-1, nullptr },
			{ TRACK_3,	-PUMP_COL_SPACING*2.5f-1, nullptr },
			{ TRACK_4,	-PUMP_COL_SPACING*1.5f-1, nullptr },
			{ TRACK_5,	-PUMP_COL_SPACING*0.5f-1, nullptr },
			{ TRACK_6,	+PUMP_COL_SPACING*0.5f+1, nullptr },
			{ TRACK_7,	+PUMP_COL_SPACING*1.5f+1, nullptr },
			{ TRACK_8,	+PUMP_COL_SPACING*2.5f+1, nullptr },
			{ TRACK_9,	+PUMP_COL_SPACING*3.5f+1, nullptr },
			{ TRACK_10,	+PUMP_COL_SPACING*4.5f+1, nullptr },
		},
	},
	{	// m_iInputColumn[NUM_GameController][NUM_GameButton]
		{ 1, 3, 2, 0, 4, Style::END_MAPPING },
		{ 6, 8, 7, 5, 9, Style::END_MAPPING },
	},
	{	// m_iColumnDrawOrder[MAX_COLS_PER_PLAYER];
		2,1,3,0,4, 2+5,1+5,3+5,0+5,4+5
	},
	false, // m_bCanUseBeginnerHelper
	false, // m_bLockDifficulties
};

static const Style g_Style_Pump_Couple =
{	// STYLE_PUMP_COUPLE
	true,				// m_bUsedForGameplay
	false,				// m_bUsedForEdit
	false,				// m_bUsedForDemonstration
	false,				// m_bUsedForHowToPlay
	"couple",			// m_szName
	StepsType_pump_couple,		// m_StepsType
	StyleType_TwoPlayersTwoSides,		// m_StyleType
	5,				// m_iColsPerPlayer
	{	// m_ColumnInfo[NUM_PLAYERS][MAX_COLS_PER_PLAYER];
		{	// PLAYER_1
			{ TRACK_1,	-PUMP_COL_SPACING*2.0f, nullptr },
			{ TRACK_2,	-PUMP_COL_SPACING*1.0f, nullptr },
			{ TRACK_3,	+PUMP_COL_SPACING*0.0f, nullptr },
			{ TRACK_4,	+PUMP_COL_SPACING*1.0f, nullptr },
			{ TRACK_5,	+PUMP_COL_SPACING*2.0f, nullptr },
		},
		{	// PLAYER_2
			{ TRACK_6,	-PUMP_COL_SPACING*2.0f, nullptr },
			{ TRACK_7,	-PUMP_COL_SPACING*1.0f, nullptr },
			{ TRACK_8,	+PUMP_COL_SPACING*0.0f, nullptr },
			{ TRACK_9,	+PUMP_COL_SPACING*1.0f, nullptr },
			{ TRACK_10,	+PUMP_COL_SPACING*2.0f, nullptr },
		},
	},
	{	// m_iInputColumn[NUM_GameController][NUM_GameButton]
		{ 1, 3, 2, 0, 4, Style::END_MAPPING },
		{ 1, 3, 2, 0, 4, Style::END_MAPPING },
	},
	{	// m_iColumnDrawOrder[MAX_COLS_PER_PLAYER];
		2,1,3,0,4
	},
	false, // m_bCanUseBeginnerHelper
	true, // m_bLockDifficulties
};

static const Style g_Style_Pump_Couple_Edit =
{	// STYLE_PUMP_EDIT_COUPLE
	false,				// m_bUsedForGameplay
	true,				// m_bUsedForEdit
	false,				// m_bUsedForDemonstration
	false,				// m_bUsedForHowToPlay
	"couple-edit",			// m_szName
	StepsType_pump_couple,		// m_StepsType
	StyleType_OnePlayerTwoSides,		// m_StyleType
	10,				// m_iColsPerPlayer
	{	// m_ColumnInfo[NUM_PLAYERS][MAX_COLS_PER_PLAYER];
		{	// PLAYER_1
			{ TRACK_1,	-PUMP_COL_SPACING*5.0f-4, nullptr },
			{ TRACK_2,	-PUMP_COL_SPACING*4.0f-4, nullptr },
			{ TRACK_3,	-PUMP_COL_SPACING*3.0f-4, nullptr },
			{ TRACK_4,	-PUMP_COL_SPACING*2.0f-4, nullptr },
			{ TRACK_5,	-PUMP_COL_SPACING*1.0f-4, nullptr },
			{ TRACK_6,	+PUMP_COL_SPACING*1.0f+4, nullptr },
			{ TRACK_7,	+PUMP_COL_SPACING*2.0f+4, nullptr },
			{ TRACK_8,	+PUMP_COL_SPACING*3.0f+4, nullptr },
			{ TRACK_9,	+PUMP_COL_SPACING*4.0f+4, nullptr },
			{ TRACK_10,	+PUMP_COL_SPACING*5.0f+4, nullptr },
		},
		{	// PLAYER_2
			{ TRACK_1,	-PUMP_COL_SPACING*5.0f-4, nullptr },
			{ TRACK_2,	-PUMP_COL_SPACING*4.0f-4, nullptr },
			{ TRACK_3,	-PUMP_COL_SPACING*3.0f-4, nullptr },
			{ TRACK_4,	-PUMP_COL_SPACING*2.0f-4, nullptr },
			{ TRACK_5,	-PUMP_COL_SPACING*1.0f-4, nullptr },
			{ TRACK_6,	+PUMP_COL_SPACING*1.0f+4, nullptr },
			{ TRACK_7,	+PUMP_COL_SPACING*2.0f+4, nullptr },
			{ TRACK_8,	+PUMP_COL_SPACING*3.0f+4, nullptr },
			{ TRACK_9,	+PUMP_COL_SPACING*4.0f+4, nullptr },
			{ TRACK_10,	+PUMP_COL_SPACING*5.0f+4, nullptr },
		},
	},
	{	// m_iInputColumn[NUM_GameController][NUM_GameButton]
		{ 1, 3, 2, 0, 4, Style::END_MAPPING },
		{ 6, 8, 7, 5, 9, Style::END_MAPPING },
	},
	{	// m_iColumnDrawOrder[MAX_COLS_PER_PLAYER];
		2,1,3,0,4, 2+5,1+5,3+5,0+5,4+5
	},
	false, // m_bCanUseBeginnerHelper
	false, // m_bLockDifficulties
};

static const Style g_Style_Pump_Routine =
{	// STYLE_PUMP_ROUTINE
	true,				// m_bUsedForGameplay
	true,				// m_bUsedForEdit
	false,				// m_bUsedForDemonstration
	false,				// m_bUsedForHowToPlay
	"routine",			// m_szName
	StepsType_pump_routine,		// m_StepsType
	StyleType_TwoPlayersSharedSides,	// m_StyleType
	10,				// m_iColsPerPlayer
	{	// m_ColumnInfo[NUM_PLAYERS][MAX_COLS_PER_PLAYER];
		{	// PLAYER_1
      // xMAx - Change 4 to 1
			{ TRACK_1,	-PUMP_COL_SPACING*4.5f-1, nullptr },
			{ TRACK_2,	-PUMP_COL_SPACING*3.5f-1, nullptr },
			{ TRACK_3,	-PUMP_COL_SPACING*2.5f-1, nullptr },
			{ TRACK_4,	-PUMP_COL_SPACING*1.5f-1, nullptr },
			{ TRACK_5,	-PUMP_COL_SPACING*0.5f-1, nullptr },
			{ TRACK_6,	+PUMP_COL_SPACING*0.5f+1, nullptr },
			{ TRACK_7,	+PUMP_COL_SPACING*1.5f+1, nullptr },
			{ TRACK_8,	+PUMP_COL_SPACING*2.5f+1, nullptr },
			{ TRACK_9,	+PUMP_COL_SPACING*3.5f+1, nullptr },
			{ TRACK_10,	+PUMP_COL_SPACING*4.5f+1, nullptr },
		},
		{	// PLAYER_2
			{ TRACK_1,	-PUMP_COL_SPACING*4.5f-1, nullptr },
			{ TRACK_2,	-PUMP_COL_SPACING*3.5f-1, nullptr },
			{ TRACK_3,	-PUMP_COL_SPACING*2.5f-1, nullptr },
			{ TRACK_4,	-PUMP_COL_SPACING*1.5f-1, nullptr },
			{ TRACK_5,	-PUMP_COL_SPACING*0.5f-1, nullptr },
			{ TRACK_6,	+PUMP_COL_SPACING*0.5f+1, nullptr },
			{ TRACK_7,	+PUMP_COL_SPACING*1.5f+1, nullptr },
			{ TRACK_8,	+PUMP_COL_SPACING*2.5f+1, nullptr },
			{ TRACK_9,	+PUMP_COL_SPACING*3.5f+1, nullptr },
			{ TRACK_10,	+PUMP_COL_SPACING*4.5f+1, nullptr },
		},
	},
	{	// m_iInputColumn[NUM_GameController][NUM_GameButton]
		{ 1, 3, 2, 0, 4, Style::END_MAPPING },
		{ 6, 8, 7, 5, 9, Style::END_MAPPING },
	},
	{	// m_iColumnDrawOrder[MAX_COLS_PER_PLAYER];
		2,1,3,0,4,7,6,8,5,9
	},
	false, // m_bCanUseBeginnerHelper
	true, // m_bLockDifficulties
};

static const Style *g_apGame_Pump_Styles[] =
{
	&g_Style_Pump_Single,
	&g_Style_Pump_Versus,
	&g_Style_Pump_HalfDouble,
	&g_Style_Pump_Double,
	&g_Style_Pump_Couple,
	&g_Style_Pump_Couple_Edit,
	&g_Style_Pump_Routine,
	nullptr
};

static const Game g_Game_Pump =
{
	"pump",						// m_szName
	g_apGame_Pump_Styles,				// m_apStyles
	false,						// m_bCountNotesSeparately
	true, // m_bTickHolds
	false, // m_PlayersHaveSeparateStyles
	{						// m_InputScheme
		"pump",					// m_szName
		NUM_PUMP_BUTTONS,			// m_iButtonsPerController
		{	// m_szButtonNames
			{ "UpLeft",		GAME_BUTTON_UP },
			{ "UpRight",		GAME_BUTTON_DOWN },
			{ "Center",		GAME_BUTTON_START },
			{ "DownLeft",		GAME_BUTTON_LEFT },
			{ "DownRight",		GAME_BUTTON_RIGHT },
		},
		&g_AutoKeyMappings_Pump
	},
	{
		{ GameButtonType_Step },
		{ GameButtonType_Step },
		{ GameButtonType_Step },
		{ GameButtonType_Step },
		{ GameButtonType_Step },
	},
	TNS_W1,	// m_mapW1To
	TNS_W2,	// m_mapW2To
	TNS_W3,	// m_mapW3To
	TNS_W4,	// m_mapW4To
	TNS_W5,	// m_mapW5To
};

/** Lights *******************************************************************/

static const int LIGHTS_COL_SPACING = 64;

static const AutoMappings g_AutoKeyMappings_Lights = AutoMappings (
	"",
	"",
	"",
	AutoMappingEntry( 0, KEY_Cq,		LIGHTS_BUTTON_MARQUEE_UP_LEFT,	false ),
	AutoMappingEntry( 0, KEY_Cw,		LIGHTS_BUTTON_MARQUEE_UP_RIGHT,	false ),
	AutoMappingEntry( 0, KEY_Ce,		LIGHTS_BUTTON_MARQUEE_LR_LEFT,	false ),
	AutoMappingEntry( 0, KEY_Cr,		LIGHTS_BUTTON_MARQUEE_LR_RIGHT,	false ),
	AutoMappingEntry( 0, KEY_Ct,		LIGHTS_BUTTON_BUTTONS_LEFT,	false ),
	AutoMappingEntry( 0, KEY_Cy,		LIGHTS_BUTTON_BUTTONS_RIGHT,	false ),
	AutoMappingEntry( 0, KEY_Cu,		LIGHTS_BUTTON_BASS_LEFT,	false ),
	AutoMappingEntry( 0, KEY_Ci,		LIGHTS_BUTTON_BASS_RIGHT,	false )
);

static const Style g_Style_Lights_Cabinet =
{	// STYLE_LIGHTS_CABINET
	true,				// m_bUsedForGameplay
	true,				// m_bUsedForEdit
	false,				// m_bUsedForDemonstration
	false,				// m_bUsedForHowToPlay
	"cabinet",			// m_szName
	StepsType_lights_cabinet,	// m_StepsType
	StyleType_OnePlayerOneSide,		// m_StyleType
	NUM_CabinetLight,		// m_iColsPerPlayer
	{	// m_ColumnInfo[NUM_PLAYERS][MAX_COLS_PER_PLAYER];
		{	// PLAYER_1
			{ TRACK_1,	-LIGHTS_COL_SPACING*3.5f, nullptr },
			{ TRACK_2,	-LIGHTS_COL_SPACING*2.5f, nullptr },
			{ TRACK_3,	-LIGHTS_COL_SPACING*1.5f, nullptr },
			{ TRACK_4,	-LIGHTS_COL_SPACING*0.5f, nullptr },
			{ TRACK_5,	+LIGHTS_COL_SPACING*0.5f, nullptr },
			{ TRACK_6,	+LIGHTS_COL_SPACING*1.5f, nullptr },
			{ TRACK_7,	+LIGHTS_COL_SPACING*2.5f, nullptr },
			{ TRACK_8,	+LIGHTS_COL_SPACING*3.5f, nullptr },
		},
		{	// PLAYER_2
			{ TRACK_1,	-LIGHTS_COL_SPACING*3.5f, nullptr },
			{ TRACK_2,	-LIGHTS_COL_SPACING*2.5f, nullptr },
			{ TRACK_3,	-LIGHTS_COL_SPACING*1.5f, nullptr },
			{ TRACK_4,	-LIGHTS_COL_SPACING*0.5f, nullptr },
			{ TRACK_5,	+LIGHTS_COL_SPACING*0.5f, nullptr },
			{ TRACK_6,	+LIGHTS_COL_SPACING*1.5f, nullptr },
			{ TRACK_7,	+LIGHTS_COL_SPACING*2.5f, nullptr },
			{ TRACK_8,	+LIGHTS_COL_SPACING*3.5f, nullptr },
		},
	},
	{	// m_iInputColumn[NUM_GameController][NUM_GameButton]
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, Style::END_MAPPING },
		{ 0, 1, 2, 3, 4, 5, 6, 7, 8, Style::END_MAPPING },
	},
	{	// m_iColumnDrawOrder[MAX_COLS_PER_PLAYER];
		0,1,2,3,4,5,6,7
	},
	false, // m_bCanUseBeginnerHelper
	false, // m_bLockDifficulties
};

static const Style *g_apGame_Lights_Styles[] =
{
	&g_Style_Lights_Cabinet,
	nullptr
};

static const Game g_Game_Lights =
{
	"lights",					// m_szName
	g_apGame_Lights_Styles,				// m_apStyles
	false,						// m_bCountNotesSeparately
	false, // m_bTickHolds
	false, // m_PlayersHaveSeparateStyles
	{						// m_InputScheme
		"lights",				// m_szName
		NUM_LIGHTS_BUTTONS,			// m_iButtonsPerController
		{	// m_szButtonNames
			{ "MarqueeUpLeft",	GAME_BUTTON_LEFT },
			{ "MarqueeUpRight",	GAME_BUTTON_RIGHT },
			{ "MarqueeLrLeft",	GAME_BUTTON_UP },
			{ "MarqueeLrRight",	GAME_BUTTON_DOWN },
			{ "ButtonsLeft",	GameButton_Invalid },
			{ "ButtonsRight",	GameButton_Invalid },
			{ "BassLeft",		GameButton_Invalid },
			{ "BassRight",		GameButton_Invalid },
		},
		&g_AutoKeyMappings_Lights
	},
	{
		{ GameButtonType_Step },
		{ GameButtonType_Step },
		{ GameButtonType_Step },
		{ GameButtonType_Step },
		{ GameButtonType_Step },
		{ GameButtonType_Step },
		{ GameButtonType_Step },
		{ GameButtonType_Step },
	},
	TNS_W1,	// m_mapW1To
	TNS_W2,	// m_mapW2To
	TNS_W3,	// m_mapW3To
	TNS_W4,	// m_mapW4To
	TNS_W5,	// m_mapW5To
};

static const Game *g_Games[] =
{
	&g_Game_Pump,
	&g_Game_Lights,
};

GameManager::GameManager()
{
	// Register with Lua.
	{
		Lua *L = LUA->Get();
		lua_pushstring( L, "GAMEMAN" );
		this->PushSelf( L );
		lua_settable( L, LUA_GLOBALSINDEX );
		LUA->Release( L );
	}
}

GameManager::~GameManager()
{
	// Unregister with Lua.
	LUA->UnsetGlobal( "GAMEMAN" );
}


void GameManager::GetStylesForGame( const Game *pGame, vector<const Style*>& aStylesAddTo, bool editor )
{
	for( int s=0; pGame->m_apStyles[s]; ++s )
	{
		const Style *style = pGame->m_apStyles[s];
		if( !editor && !style->m_bUsedForGameplay )
			continue;
		if( editor && !style->m_bUsedForEdit )
			continue;

		aStylesAddTo.push_back( style );
	}
}

const Game *GameManager::GetGameForStyle( const Style *pStyle )
{
	for( size_t g=0; g<ARRAYLEN(g_Games); ++g )
	{
		const Game *pGame = g_Games[g];
		for( int s=0; pGame->m_apStyles[s]; ++s )
		{
			if( pGame->m_apStyles[s] == pStyle )
				return pGame;
		}
	}
	FAIL_M(pStyle->m_szName);
}

const Style* GameManager::GetEditorStyleForStepsType( StepsType st )
{
	for( size_t g=0; g<ARRAYLEN(g_Games); ++g )
	{
		const Game *pGame = g_Games[g];
		for( int s=0; pGame->m_apStyles[s]; ++s )
		{
			const Style *style = pGame->m_apStyles[s];
			if( style->m_StepsType == st && style->m_bUsedForEdit )
				return style;
		}
	}

	ASSERT_M(0, ssprintf("The current game cannot use this Style with the editor!"));
	return nullptr;
}


void GameManager::GetStepsTypesForGame( const Game *pGame, vector<StepsType>& aStepsTypeAddTo )
{
	for( int i=0; pGame->m_apStyles[i]; ++i )
	{
		StepsType st = pGame->m_apStyles[i]->m_StepsType;
		ASSERT(st < NUM_StepsType);

		// Some Styles use the same StepsType (e.g. single and versus) so check
		// that we aren't doubling up.
		bool found = false;
		for( unsigned j=0; j < aStepsTypeAddTo.size(); j++ )
			if( st == aStepsTypeAddTo[j] ) { found = true; break; }
		if(found) continue;

		aStepsTypeAddTo.push_back( st );
	}
}

void GameManager::GetDemonstrationStylesForGame( const Game *pGame, vector<const Style*> &vpStylesOut )
{
	vpStylesOut.clear();

	for( int s=0; pGame->m_apStyles[s]; ++s )
	{
		const Style *style = pGame->m_apStyles[s];
		if( style->m_bUsedForDemonstration )
			vpStylesOut.push_back( style );
	}

	ASSERT( vpStylesOut.size()>0 );	// this Game is missing a Style that can be used with the demonstration
}

const Style* GameManager::GetHowToPlayStyleForGame( const Game *pGame )
{
	for( int s=0; pGame->m_apStyles[s]; ++s )
	{
		const Style *style = pGame->m_apStyles[s];
		if( style->m_bUsedForHowToPlay )
			return style;
	}

	FAIL_M(ssprintf("Game has no Style that can be used with HowToPlay: %s", pGame->m_szName));
}

void GameManager::GetCompatibleStyles( const Game *pGame, int iNumPlayers, vector<const Style*> &vpStylesOut )
{
	FOREACH_ENUM( StyleType, styleType )
	{
		int iNumPlayersRequired;
		switch( styleType )
		{
		DEFAULT_FAIL( styleType );
		case StyleType_OnePlayerOneSide:
		case StyleType_OnePlayerTwoSides:
		case StyleType_TwoPlayersSharedSides: // xMAx - Joined for Double Performance / CoOp
			iNumPlayersRequired = 1;
			break;
		case StyleType_TwoPlayersTwoSides:
		// case StyleType_TwoPlayersSharedSides: // xMAx
			iNumPlayersRequired = 2;
			break;
		}

		if( iNumPlayers != iNumPlayersRequired )
			continue;

		for( int s=0; pGame->m_apStyles[s]; ++s )
		{
			const Style *style = pGame->m_apStyles[s];
			if( style->m_StyleType != styleType )
				continue;
			if( !style->m_bUsedForGameplay )
				continue;

			vpStylesOut.push_back( style );
		}
	}
}

const Style *GameManager::GetFirstCompatibleStyle( const Game *pGame, int iNumPlayers, StepsType st )
{
	vector<const Style*> vpStyles;
	GetCompatibleStyles( pGame, iNumPlayers, vpStyles );
	for (Style const *s : vpStyles)
	{
		if( s->m_StepsType == st )
		{
			return s;
		}
	}
	return nullptr;
}


void GameManager::GetEnabledGames( vector<const Game*>& aGamesOut )
{
	for( size_t g=0; g<ARRAYLEN(g_Games); ++g )
	{
		const Game *pGame = g_Games[g];
		if( IsGameEnabled( pGame ) )
			aGamesOut.push_back( pGame );
	}
}

const Game* GameManager::GetDefaultGame()
{
	const Game *pDefault = nullptr;
	if( pDefault == nullptr )
	{
		for( size_t i=0; pDefault == nullptr && i < ARRAYLEN(g_Games); ++i )
		{
			if( IsGameEnabled(g_Games[i]) )
				pDefault = g_Games[i];
		}

		if( pDefault == nullptr )
			RageException::Throw( "No NoteSkins found" );
	}

	return pDefault;
}

int GameManager::GetIndexFromGame( const Game* pGame )
{
	for( size_t g=0; g<ARRAYLEN(g_Games); ++g )
	{
		if( g_Games[g] == pGame )
			return g;
	}
	FAIL_M(ssprintf("Game not found: %s", pGame->m_szName));
}

const Game* GameManager::GetGameFromIndex( int index )
{
	ASSERT( index >= 0 );
	ASSERT( index < (int) ARRAYLEN(g_Games) );
	return g_Games[index];
}

bool GameManager::IsGameEnabled( const Game *pGame )
{
	return NOTESKIN->DoNoteSkinsExistForGame( pGame );
}

const StepsTypeInfo &GameManager::GetStepsTypeInfo( StepsType st )
{
	ASSERT( ARRAYLEN(g_StepsTypeInfos) == NUM_StepsType );
	ASSERT_M( st < NUM_StepsType, ssprintf("StepsType %d < NUM_StepsType (%d)", st, NUM_StepsType) );
	return g_StepsTypeInfos[st];
}

StepsType GameManager::StringToStepsType( RString sStepsType )
{
	sStepsType.MakeLower();

	for( int i=0; i<NUM_StepsType; i++ )
		if( g_StepsTypeInfos[i].szName == sStepsType )
			return StepsType(i);

	return StepsType_Invalid;
}

RString GameManager::StyleToLocalizedString( const Style* style )
{
	RString s = style->m_szName;
	s = Capitalize( s );
	if( THEME->HasString( "Style", s ) )
		return THEME->GetString( "Style", s );
	else
		return s;
}

const Game* GameManager::StringToGame( RString sGame )
{
	for( size_t i=0; i<ARRAYLEN(g_Games); ++i )
		if( !sGame.CompareNoCase(g_Games[i]->m_szName) )
			return g_Games[i];

	return nullptr;
}


const Style* GameManager::GameAndStringToStyle( const Game *game, RString sStyle )
{
	for( int s=0; game->m_apStyles[s]; ++s )
	{
		const Style* style = game->m_apStyles[s];
		if( sStyle.CompareNoCase(style->m_szName) == 0 )
			return style;
	}

	return nullptr;
}

// lua start
#include "LuaBinding.h"

/** @brief Allow Lua to have access to the GameManager. */
class LunaGameManager: public Luna<GameManager>
{
public:
	static int StepsTypeToLocalizedString( T* p, lua_State *L )	{ lua_pushstring(L, p->GetStepsTypeInfo(Enum::Check<StepsType>(L, 1)).GetLocalizedString() ); return 1; }
	static int GetFirstStepsTypeForGame( T* p, lua_State *L )
	{
		Game *pGame = Luna<Game>::check( L, 1 );

		vector<StepsType> vstAddTo;
		p->GetStepsTypesForGame( pGame, vstAddTo );
		ASSERT( !vstAddTo.empty() );
		StepsType st = vstAddTo[0];
		LuaHelpers::Push( L, st );
		return 1;
	}
	static int IsGameEnabled( T* p, lua_State *L )
	{
		const Game *pGame = p->StringToGame(SArg(1));
		if(pGame)
			lua_pushboolean(L, p->IsGameEnabled( pGame ) );
		else
			lua_pushnil(L);

		return 1;
	}
	static int GetStylesForGame( T* p, lua_State *L )
	{
		RString game_name= SArg(1);
		const Game *pGame = p->StringToGame(game_name);
		if(!pGame)
		{
			luaL_error(L, "GetStylesForGame: Invalid Game: '%s'", game_name.c_str());
		}
		vector<Style*> aStyles;
		lua_createtable(L, 0, 0);
		for( int s=0; pGame->m_apStyles[s]; ++s )
		{
			Style *pStyle = const_cast<Style *>( pGame->m_apStyles[s] );
			pStyle->PushSelf(L);
			lua_rawseti(L, -2, s+1);
		}
		return 1;
	}
	static int GetEnabledGames( T* p, lua_State *L )
	{
		vector<const Game*> aGames;
		p->GetEnabledGames( aGames );
		lua_createtable(L, aGames.size(), 0);
		for(size_t i= 0; i < aGames.size(); ++i)
		{
			lua_pushstring(L, aGames[i]->m_szName);
			lua_rawseti(L, -2, i+1);
		}
		return 1;
	}

	static int SetGame( T* p, lua_State *L )
	{
		RString game_name= SArg(1);
		const Game *pGame = p->StringToGame(game_name);
		if(!pGame)
		{
			luaL_error(L, "SetGame: Invalid Game: '%s'", game_name.c_str());
		}
		RString theme;
		if( lua_gettop(L) >= 2 && !lua_isnil(L, 2) )
		{
			theme = SArg(2);
			if(!THEME->IsThemeSelectable(theme))
			{
				luaL_error(L, "SetGame: Invalid Theme: '%s'", theme.c_str());
			}
		}
		GameLoop::ChangeGame(game_name, theme);
		return 0;
	}

	LunaGameManager()
	{
		ADD_METHOD( StepsTypeToLocalizedString );
		ADD_METHOD( GetFirstStepsTypeForGame );
		ADD_METHOD( IsGameEnabled );
		ADD_METHOD( GetStylesForGame );
		ADD_METHOD( GetEnabledGames );
		ADD_METHOD( SetGame );
	};
};

LUA_REGISTER_CLASS( GameManager )
// lua end


/*
 * (c) 2001-2006 Chris Danford, Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
