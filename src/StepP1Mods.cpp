#include "global.h"
#include "StepP1Mods.h"
#include "PlayerOptions.h"
#include "PlayerState.h"
#include "GameState.h"
#include "SongOptions.h"
#include "NoteSkinManager.h"
#include "CommonMetrics.h"
#include "MessageManager.h"
#include "GameManager.h"
#include "RageUtil.h"

// Recreated from STEP_P1.exe: PlayerOptions 0x49ef58 / 0x49e36b / 0x49e22f / 0x49e15a.

namespace
{
	const short g_SpeedIcons[]		= { 0, 1, 2, 3, 4, 5, 87, 6, 7, 8, 9, 10, 11 };
	const short g_AVIcons[]			= { 110, 111, 112, 113, 114, 115 };
	const short g_DisplayIcons[]	= { 12, 13, 14, 15, 16, 17, 79, 80, 18, 117 };
	const short g_PathIcons[]		= { 49, 50, 51, 52, 53, 54, 55, 118 };
	const short g_AlternateIcons[]	= { 57, 59 };
	const short g_JudgeIcons[]		= { 61, 63, 64, 65, 66, 67, 69, 88 };
	const short g_RateIcons[]		= { 71, 72, 73, 74, 75, 76, 70 };
	const short g_BattleIcons[]		= { 89, 90, 91, 92, 93 };
	const short g_StepsTypeIcons[]	= { 94, 95, 96 };

	const short g_SpeedTexts[]		= { 24, 32, 40, 48, 56, 64, 102, 72, 80, 88, 96, 104, 1 };
	const short g_AVTexts[]			= { 121, 122, 123, 124, 125, 126 };
	const short g_DisplayTexts[]	= { 9, 17, 25, 33, 41, 15, 23, 31, 49, 127 };
	const short g_PathTexts[]		= { 107, 4, 12, 20, 36, 44, 52, 119 };
	const short g_AlternateTexts[]	= { 76, 92 };
	const short g_JudgeTexts[]		= { 13, 29, 37, 45, 53, 61, 69, 77 };
	const short g_RateTexts[]		= { 14, 22, 38, 46, 54, 62, 70 };
	const short g_BattleTexts[]		= { 0, 0, 0, 0, 0 };
	const short g_StepsTypeTexts[]	= { 0, 0, 0 };

	const float g_fRates[] = { 0.8f, 0.9f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f };

	struct FixedNoteSkin { const char *szName; short iIcon; short iText; };
	const FixedNoteSkin g_FixedNoteSkins[] =
	{
		{ "flower", 19, 65 }, { "extra_new", 20, 73 }, { "ez", 21, 81 }, { "slime", 22, 89 },
		{ "notes", 23, 97 }, { "canon", 24, 105 }, { "poker", 25, 2 }, { "nx", 26, 10 },
		{ "nx2-sheep", 27, 18 }, { "nx2-horse", 28, 26 }, { "nx2-dog", 29, 34 }, { "nx2-personhee", 30, 42 },
		{ "nx2-fire", 31, 50 }, { "nx2-ice", 32, 58 }, { "nx2-interference", 33, 66 }, { "double_per_p1", 34, 74 },
		{ "double_per_p2", 35, 82 }, { "double_per_p3", 36, 90 }, { "nxa", 37, 98 }, { "nx2", 38, 106 },
		{ "default-light", 39, 3 }, { "default-drum", 40, 11 }, { "missile", 41, 19 }, { "default-drum_p2", 42, 27 },
		{ "default-drum_p1", 43, 35 }, { "default-drum_p3", 44, 43 }, { "soccer", 45, 51 }, { "rebirth", 46, 59 },
		{ "basic_mode", 47, 67 }, { "default", 48, 75 }, { "fiesta_2", 77, 83 }, { "extra_mix", 78, 39 },
		{ "jump", 81, 47 }, { "pro", 82, 55 }, { "infinity", 83, 63 }, { "hns_1", 84, 87 },
		{ "hns_2", 84, 95 }, { "psp_3", 97, 103 }, { "psp_4", 98, 78 }, { "psp_5", 99, 86 },
		{ "extra_mix-dj", 100, 109 }, { "extra_mix-dung", 101, 110 }, { "extra_mix-ice", 102, 111 }, { "extra_mix-octo", 103, 113 },
		{ "extra_mix-moon", 104, 112 }, { "extra_mix-pine", 105, 114 }, { "extra_mix-snow", 106, 115 }, { "extra_mix-ufo", 107, 116 },
		{ "extra_mix-ultraman", 108, 117 },
	};

	template<size_t N> int TableLookup( const short (&table)[N], int iItem )
	{
		if( iItem < 0 || iItem >= (int)N )
			return 0;
		return table[iItem];
	}

	// Turning one speed mod on clears the others (the block repeated in 0x49ef58).
	void ClearSpeedMods( PlayerOptions &po )
	{
		po.m_iAutoVelocity = 0;
		po.m_fMaxScrollBPM = 0;
		po.m_bEW = false;
		po.m_fTimeSpacing = 0;
		po.m_fAccels[PlayerOptions::ACCEL_EXPAND] = 0;
		po.m_fAccels[PlayerOptions::ACCEL_BOOST] = 0;
		po.m_fAccels[PlayerOptions::ACCEL_BRAKE] = 0;
		po.m_fRandomSpeed = 0;
		po.m_SpeedfMaxScrollBPM = 1.0f;
		po.m_fScrollSpeed = 2.0f;
	}

	bool ToggleFloat( float &f, float fOn, bool &bOn )
	{
		if( f != fOn )
		{
			f = fOn;
			return true;
		}
		f = 0;
		bOn = false;
		return false;
	}

	void SetDefaultNoteSkin( PlayerOptions &po )
	{
		po.m_sNoteSkin = CommonMetrics::DEFAULT_NOTESKIN_NAME;
		po.m_iNoteSkinIndex = -1;
	}

	SongOptions &PreferredSongOptions() { return GAMESTATE->m_SongOptions.GetPreferred(); }
	const SongOptions &CurrentSongOptions() { return GAMESTATE->m_SongOptions.GetCurrent(); }
}

const vector<StepP1Mods::NoteSkinEntry> &StepP1Mods::GetNoteSkins()
{
	static vector<NoteSkinEntry> s_vEntries;
	if( !s_vEntries.empty() )
		return s_vEntries;

	for( unsigned i = 0; i < ARRAYLEN(g_FixedNoteSkins); ++i )
	{
		NoteSkinEntry e = { g_FixedNoteSkins[i].szName, g_FixedNoteSkins[i].iIcon, g_FixedNoteSkins[i].iText, true };
		s_vEntries.push_back( e );
	}

	// Other installed noteskins go at the end with a generic icon.
	vector<RString> vsInstalled;
	NOTESKIN->GetNoteSkinNames( vsInstalled );
	for( unsigned i = 0; i < vsInstalled.size(); ++i )
	{
		bool bKnown = vsInstalled[i] == CommonMetrics::DEFAULT_NOTESKIN_NAME.GetValue();
		for( unsigned j = 0; !bKnown && j < s_vEntries.size(); ++j )
			bKnown = s_vEntries[j].sName == vsInstalled[i];
		if( bKnown )
			continue;
		NoteSkinEntry e = { vsInstalled[i], 0x3c, 0x5e, false };
		s_vEntries.push_back( e );
	}
	return s_vEntries;
}

int StepP1Mods::GetNumItems( int iCategory )
{
	switch( iCategory )
	{
	case CAT_SPEED:		return 13;
	case CAT_AV:		return 6;
	case CAT_DISPLAY:	return 10;
	case CAT_NOTESKIN:	return GetNoteSkins().size();
	case CAT_PATH:		return 8;
	case CAT_ALTERNATE:	return 2;
	case CAT_JUDGE:		return 8;
	case CAT_RATE:		return 7;
	case CAT_BATTLE:	return 5;
	case CAT_STEPSTYPES:return 3;
	default:			return 0;
	}
}

int StepP1Mods::GetIcon( int iCategory, int iItem )
{
	switch( iCategory )
	{
	case CAT_SPEED:		return TableLookup( g_SpeedIcons, iItem );
	case CAT_AV:		return TableLookup( g_AVIcons, iItem );
	case CAT_DISPLAY:	return TableLookup( g_DisplayIcons, iItem );
	case CAT_NOTESKIN:
	{
		const vector<NoteSkinEntry> &v = GetNoteSkins();
		if( iItem < 0 || iItem >= (int)v.size() )
			return 0;
		if( v[iItem].sName == "hns_1" && GAMESTATE->m_bNoteSkin1Unlocked )
			return 0x55;
		if( v[iItem].sName == "hns_2" && GAMESTATE->m_bNoteSkin2Unlocked )
			return 0x56;
		return v[iItem].iIcon;
	}
	case CAT_PATH:		return TableLookup( g_PathIcons, iItem );
	case CAT_ALTERNATE:	return TableLookup( g_AlternateIcons, iItem );
	case CAT_JUDGE:		return TableLookup( g_JudgeIcons, iItem );
	case CAT_RATE:		return TableLookup( g_RateIcons, iItem );
	case CAT_BATTLE:	return TableLookup( g_BattleIcons, iItem );
	case CAT_STEPSTYPES:return TableLookup( g_StepsTypeIcons, iItem );
	default:			return 0;
	}
}

int StepP1Mods::GetText( int iCategory, int iItem )
{
	switch( iCategory )
	{
	case CAT_SPEED:		return TableLookup( g_SpeedTexts, iItem );
	case CAT_AV:		return TableLookup( g_AVTexts, iItem );
	case CAT_DISPLAY:	return TableLookup( g_DisplayTexts, iItem );
	case CAT_NOTESKIN:
	{
		const vector<NoteSkinEntry> &v = GetNoteSkins();
		if( iItem < 0 || iItem >= (int)v.size() )
			return 0;
		return v[iItem].iText;
	}
	case CAT_PATH:		return TableLookup( g_PathTexts, iItem );
	case CAT_ALTERNATE:	return TableLookup( g_AlternateTexts, iItem );
	case CAT_JUDGE:		return TableLookup( g_JudgeTexts, iItem );
	case CAT_RATE:		return TableLookup( g_RateTexts, iItem );
	case CAT_BATTLE:	return TableLookup( g_BattleTexts, iItem );
	case CAT_STEPSTYPES:return TableLookup( g_StepsTypeTexts, iItem );
	default:			return 7;
	}
}

void StepP1Mods::GetActiveIcons( const PlayerOptions &po, int iCategory, vector<int> &vOut )
{
	vOut.clear();
	if( iCategory == CAT_AV )
	{
		if( po.m_iAutoVelocity != 0 )
			vOut.push_back( 0x74 );
		return;
	}
	if( iCategory == CAT_NOTESKIN )
	{
		if( po.m_iNoteSkinIndex != -1 )
			vOut.push_back( GetIcon(CAT_NOTESKIN, po.m_iNoteSkinIndex) );
		return;
	}
	int iNum = GetNumItems( iCategory );
	for( int i = 0; i < iNum; ++i )
		if( IsActive(po, iCategory, i) )
			vOut.push_back( GetIcon(iCategory, i) );
}

void StepP1Mods::GetAllIcons( int iCategory, vector<int> &vOut )
{
	vOut.clear();
	int iNum = GetNumItems( iCategory );
	for( int i = 0; i < iNum; ++i )
		vOut.push_back( GetIcon(iCategory, i) );
}

bool StepP1Mods::IsGlobal( int iCategory, int iItem )
{
	return iCategory == CAT_RATE ||
		(iCategory == CAT_DISPLAY && (iItem == 8 || iItem == 9)) ||
		iCategory == CAT_BATTLE ||
		iCategory == CAT_STEPSTYPES;
}

void StepP1Mods::Apply( PlayerOptions &po, int iCategory, int iItem, bool &bOn, bool &bSpecial )
{
	bOn = true;
	switch( iCategory )
	{
	case CAT_SPEED:
	{
		bool bAV = po.m_iAutoVelocity != 0;
		if( iItem != 9 && po.m_fRandomSpeed == 1.0f )
			ClearSpeedMods( po );
		if( iItem != 10 && bAV )
			ClearSpeedMods( po );
		if( iItem != 8 && po.m_bEW )
			ClearSpeedMods( po );
		if( po.m_iAutoVelocity != 0 && iItem >= 0 && (iItem < 9 || (iItem > 10 && iItem < 13)) )
			ClearSpeedMods( po );

		float &fSpeed = po.m_fScrollSpeed;
		switch( iItem )
		{
		case 0: case 1: case 2: case 3: case 4: case 5:
			fSpeed = float(iItem + 1);
			return;
		case 6:
		{
			float fWhole = floorf( fSpeed );
			if( fSpeed == fWhole + 0.25f )		{ fSpeed = fWhole; bOn = false; }
			else if( fSpeed == fWhole + 0.75f )	{ fSpeed = fWhole + 0.5f; bOn = false; }
			else								fSpeed += 0.25f;
			return;
		}
		case 7:
		{
			float fWhole = floorf( fSpeed );
			if( fSpeed == fWhole + 0.5f )		{ fSpeed = fWhole; bOn = false; }
			else if( fSpeed == fWhole + 0.75f )	{ fSpeed = fWhole + 0.25f; bOn = false; }
			else								fSpeed += 0.5f;
			return;
		}
		case 8:
			po.m_fMaxScrollBPM = 0;
			po.m_fTimeSpacing = 0;
			po.m_fAccels[PlayerOptions::ACCEL_BOOST] = 0;
			po.m_fAccels[PlayerOptions::ACCEL_BRAKE] = 0;
			po.m_fRandomSpeed = 0;
			po.m_iAutoVelocity = 0;
			if( !po.m_bEW )
			{
				po.m_bEW = true;
				po.m_fAccels[PlayerOptions::ACCEL_EXPAND] = 1.0f;
				po.m_SpeedfMaxScrollBPM = 1.0f;
			}
			else
			{
				po.m_bEW = false;
				bOn = false;
				po.m_fAccels[PlayerOptions::ACCEL_EXPAND] = 0;
				po.m_SpeedfMaxScrollBPM = 1.0f;
			}
			fSpeed = 2.0f;
			return;
		case 9:
		case 10:
			bSpecial = true;
			return;
		case 11:
			if( po.m_fAccels[PlayerOptions::ACCEL_BOOST] != 1.0f )
			{
				po.m_fAccels[PlayerOptions::ACCEL_BRAKE] = 0;
				po.m_fAccels[PlayerOptions::ACCEL_BOOST] = 1.0f;
				return;
			}
			po.m_fAccels[PlayerOptions::ACCEL_BOOST] = 0;
			bOn = false;
			return;
		case 12:
			if( po.m_fAccels[PlayerOptions::ACCEL_BRAKE] != 1.0f )
			{
				po.m_fAccels[PlayerOptions::ACCEL_BOOST] = 0;
				po.m_fAccels[PlayerOptions::ACCEL_BRAKE] = 1.0f;
				return;
			}
			po.m_fAccels[PlayerOptions::ACCEL_BRAKE] = 0;
			bOn = false;
			return;
		default:
			bOn = false;
			return;
		}
	}

	case CAT_AV:
	{
		static const float fDelta[] = { 100, 10, 1, -1, -10, -100 };
		if( iItem < 0 || iItem >= 6 )
		{
			bOn = false;
			return;
		}
		po.m_fScrollBPM = 200;
		po.m_fMaxScrollBPM += fDelta[iItem];
		if( po.m_fMaxScrollBPM >= 1000.0f )
			po.m_fMaxScrollBPM = 999.0f;
		if( po.m_fMaxScrollBPM <= 300.0f )
			po.m_fMaxScrollBPM = 300.0f;
		po.m_iAutoVelocity = (int)po.m_fMaxScrollBPM;
		po.m_fTimeSpacing = 0;
		po.m_bEW = false;
		po.m_fScrollSpeed = 2.15f;
		po.m_fAccels[PlayerOptions::ACCEL_EXPAND] = 0;
		po.m_fAccels[PlayerOptions::ACCEL_BOOST] = 0;
		po.m_fAccels[PlayerOptions::ACCEL_BRAKE] = 0;
		po.m_fRandomSpeed = 0;
		return;
	}

	case CAT_DISPLAY:
	{
		float *fAppear = po.m_fAppearances;
		switch( iItem )
		{
		case 0:
			ToggleFloat( fAppear[PlayerOptions::APPEARANCE_HIDDEN], 1.0f, bOn );
			fAppear[PlayerOptions::APPEARANCE_SUDDEN] = 0;
			return;
		case 1:
			ToggleFloat( fAppear[PlayerOptions::APPEARANCE_SUDDEN], 1.0f, bOn );
			fAppear[PlayerOptions::APPEARANCE_HIDDEN] = 0;
			return;
		case 2:
			ToggleFloat( fAppear[PlayerOptions::APPEARANCE_STEALTH], 1.0f, bOn );
			fAppear[PlayerOptions::APPEARANCE_SUDDEN] = 0;
			fAppear[PlayerOptions::APPEARANCE_HIDDEN] = 0;
			return;
		case 3:
			ToggleFloat( po.m_fDark, 1.0f, bOn );
			return;
		case 4:
			ToggleFloat( fAppear[PlayerOptions::APPEARANCE_BLINK], 1.0f, bOn );
			return;
		case 5:
		case 6:
		case 7:
		{
			bool *pFlag = iItem == 5 ? &po.m_bRandomSkin : iItem == 6 ? &po.m_bAutoNoteskin : &po.m_bRouletteNoteskin;
			bool bWasOn = *pFlag;
			if( bWasOn )
				bOn = false;
			po.m_bRandomSkin = false;
			po.m_bAutoNoteskin = false;
			po.m_bRouletteNoteskin = false;
			*pFlag = !bWasOn;
			SetDefaultNoteSkin( po );
			return;
		}
		case 8:
		case 9:
		{
			// BGA off and BGA dark exclude each other.
			bool bWasOn = iItem == 8 ? CurrentSongOptions().m_bUseBGAOff : CurrentSongOptions().m_bUseBGADark;
			if( iItem == 8 )
			{
				SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_bUseBGAOff, !bWasOn );
				SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_bUseBGADark, false );
			}
			else
			{
				SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_bUseBGADark, !bWasOn );
				SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_bUseBGAOff, false );
			}
			if( bWasOn )
				bOn = false;
			return;
		}
		default:
			bOn = false;
			return;
		}
	}

	case CAT_NOTESKIN:
	{
		const vector<NoteSkinEntry> &v = GetNoteSkins();
		if( iItem >= 0 && iItem < (int)v.size() )
		{
			po.m_bRandomSkin = false;
			po.m_bAutoNoteskin = false;
			po.m_bRouletteNoteskin = false;
			if( v[iItem].sName != po.m_sNoteSkin )
			{
				po.m_sNoteSkin = v[iItem].sName;
				po.m_iNoteSkinIndex = iItem;
				return;
			}
			SetDefaultNoteSkin( po );
		}
		bOn = false;
		return;
	}

	case CAT_PATH:
	{
		float *fEffects = po.m_fEffects;
		switch( iItem )
		{
		case 0: ToggleFloat( fEffects[PlayerOptions::EFFECT_XMODE], 1.0f, bOn ); return;
		case 1: po.m_bNX = !po.m_bNX; bOn = po.m_bNX; return;
		case 2: ToggleFloat( po.m_fScrolls[PlayerOptions::SCROLL_UNDER_ATTACK], 1.0f, bOn ); return;
		case 3: ToggleFloat( po.m_fScrolls[PlayerOptions::SCROLL_DROP], 1.0f, bOn ); return;
		case 4: ToggleFloat( fEffects[PlayerOptions::EFFECT_BUMPY], 3.0f, bOn ); return;
		case 5: ToggleFloat( fEffects[PlayerOptions::EFFECT_BUMPY], -3.0f, bOn ); return;
		case 6: ToggleFloat( fEffects[PlayerOptions::EFFECT_TORNADO], 0.25f, bOn ); return;
		case 7:
			po.m_bMinis = !po.m_bMinis;
			fEffects[PlayerOptions::EFFECT_MINI] = po.m_bMinis ? 0.5f : 0;
			bOn = po.m_bMinis;
			return;
		default:
			bOn = false;
			return;
		}
	}

	case CAT_ALTERNATE:
		if( iItem == 0 )
		{
			po.m_bTurns[PlayerOptions::TURN_SUPER_SHUFFLE] = false;
			po.m_bTurns[PlayerOptions::TURN_BACKWARDS] = !po.m_bTurns[PlayerOptions::TURN_BACKWARDS];
		}
		else if( iItem == 1 )
		{
			po.m_bTurns[PlayerOptions::TURN_BACKWARDS] = false;
			po.m_bTurns[PlayerOptions::TURN_SUPER_SHUFFLE] = !po.m_bTurns[PlayerOptions::TURN_SUPER_SHUFFLE];
		}
		return;

	case CAT_JUDGE:
	{
		// Each judge toggles between itself and the normal judge.
		PlayerOptions::Judgment &j = po.m_iJudgment;
		switch( iItem )
		{
		case 0: j = j == PlayerOptions::EASY_JUDGMENT ? PlayerOptions::NORMAL_JUDGMENT : PlayerOptions::EASY_JUDGMENT; return;
		case 1: j = j == PlayerOptions::HARD_JUDGMENT ? PlayerOptions::NORMAL_JUDGMENT : PlayerOptions::HARD_JUDGMENT; return;
		case 2: j = j == PlayerOptions::VERY_HARD_JUDGMENT ? PlayerOptions::NORMAL_JUDGMENT : PlayerOptions::VERY_HARD_JUDGMENT; return;
		case 3: j = j == PlayerOptions::XTRA_HARD_JUDGMENT ? PlayerOptions::NORMAL_JUDGMENT : PlayerOptions::XTRA_HARD_JUDGMENT; return;
		case 4: j = j == PlayerOptions::ULTRA_HARD_JUDGMENT ? PlayerOptions::NORMAL_JUDGMENT : PlayerOptions::ULTRA_HARD_JUDGMENT; return;
		case 5: po.m_bJudgmentReverse = !po.m_bJudgmentReverse; return;
		case 6: po.m_bJudgeByNote = !po.m_bJudgeByNote; bOn = po.m_bJudgeByNote; return;
		case 7: po.m_bFreePerformance = !po.m_bFreePerformance; bOn = po.m_bFreePerformance; return;
		default: bOn = false; return;
		}
	}

	case CAT_RATE:
		if( iItem > 6 )
			return;
		if( g_fRates[iItem] != CurrentSongOptions().m_fMusicRate )
		{
			SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_fMusicRate, g_fRates[iItem] );
			return;
		}
		SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_fMusicRate, 1.0f );
		bOn = false;
		return;

	case CAT_BATTLE:
		if( iItem < 5 )
		{
			GAMESTATE->m_BattleMode.Set( (BattleMode)iItem );
			return;
		}
		bOn = false;
		return;

	case CAT_STEPSTYPES:
	{
		if( iItem > 2 )
			return;
		bool bShow[3] = { CurrentSongOptions().m_bShowSingles, CurrentSongOptions().m_bShowHalfDoubles, CurrentSongOptions().m_bShowDoubles };
		// At least one of the three must stay on.
		if( !bShow[(iItem + 1) % 3] && !bShow[(iItem + 2) % 3] )
		{
			bSpecial = true;
			bOn = false;
			return;
		}
		if( bShow[iItem] )
			bOn = false;
		bool bNew = !bShow[iItem];
		switch( iItem )
		{
		case 0: SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_bShowSingles, bNew ); break;
		case 1: SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_bShowHalfDoubles, bNew ); break;
		case 2: SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_bShowDoubles, bNew ); break;
		}
		MESSAGEMAN->Broadcast( Message_StepTypesToShowChanged );
		return;
	}

	default:
		bOn = false;
		return;
	}
}

void StepP1Mods::ApplyToPlayer( PlayerNumber pn, int iCategory, int iItem, bool &bOn, bool &bSpecial )
{
	if( IsGlobal(iCategory, iItem) )
	{
		Apply( GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions.GetCurrent(), iCategory, iItem, bOn, bSpecial );
		return;
	}
	ModsGroup<PlayerOptions> &group = GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions;
	Apply( group.GetCurrent(), iCategory, iItem, bOn, bSpecial );
	FOREACH_ENUM( ModsLevel, level )
	{
		if( level == ModsLevel_Current )
			break;
		Apply( group.Get(level), iCategory, iItem, bOn, bSpecial );
	}
}

bool StepP1Mods::IsActive( const PlayerOptions &po, int iCategory, int iItem )
{
	switch( iCategory )
	{
	case CAT_SPEED:
	{
		if( po.m_iAutoVelocity != 0 )
			return false;
		float fSpeed = po.m_fScrollSpeed;
		float fWhole = floorf( fSpeed );
		switch( iItem )
		{
		case 0: return fWhole == 1.0f;
		case 1:
			if( po.m_bEW || po.m_fRandomSpeed == 1.0f )
				return false;
			return fWhole == 2.0f;
		case 2: return fWhole == 3.0f;
		case 3: return fWhole == 4.0f;
		case 4: return fWhole == 5.0f;
		case 5: return fWhole == 6.0f;
		case 6: return fSpeed == fWhole + 0.25f || fSpeed == fWhole + 0.75f;
		case 7: return fSpeed == fWhole + 0.5f || fSpeed == fWhole + 0.75f;
		case 8: return po.m_bEW;
		case 9: return po.m_fRandomSpeed == 1.0f;
		case 11: return po.m_fAccels[PlayerOptions::ACCEL_BOOST] == 1.0f;
		case 12: return po.m_fAccels[PlayerOptions::ACCEL_BRAKE] == 1.0f;
		default: return false;
		}
	}
	case CAT_AV:
		return po.m_iAutoVelocity != 0;
	case CAT_DISPLAY:
		switch( iItem )
		{
		case 0: return po.m_fAppearances[PlayerOptions::APPEARANCE_HIDDEN] == 1.0f;
		case 1: return po.m_fAppearances[PlayerOptions::APPEARANCE_SUDDEN] == 1.0f;
		case 2: return po.m_fAppearances[PlayerOptions::APPEARANCE_STEALTH] == 1.0f;
		case 3: return po.m_fDark == 1.0f;
		case 4: return po.m_fAppearances[PlayerOptions::APPEARANCE_BLINK] == 1.0f;
		case 5: return po.m_bRandomSkin;
		case 6: return po.m_bAutoNoteskin;
		case 7: return po.m_bRouletteNoteskin;
		case 8: return CurrentSongOptions().m_bUseBGAOff;
		case 9: return CurrentSongOptions().m_bUseBGADark;
		default: return false;
		}
	case CAT_NOTESKIN:
		return iItem >= 0 && iItem < (int)GetNoteSkins().size() && po.m_iNoteSkinIndex == iItem;
	case CAT_PATH:
		switch( iItem )
		{
		case 0: return po.m_fEffects[PlayerOptions::EFFECT_XMODE] == 1.0f;
		case 1: return po.m_bNX;
		case 2: return po.m_fScrolls[PlayerOptions::SCROLL_UNDER_ATTACK] == 1.0f;
		case 3: return po.m_fScrolls[PlayerOptions::SCROLL_DROP] == 1.0f;
		case 4: return po.m_fEffects[PlayerOptions::EFFECT_BUMPY] == 3.0f;
		case 5: return po.m_fEffects[PlayerOptions::EFFECT_BUMPY] == -3.0f;
		case 6: return po.m_fEffects[PlayerOptions::EFFECT_TORNADO] == 0.25f;
		case 7: return po.m_bMinis;
		default: return false;
		}
	case CAT_ALTERNATE:
		if( iItem == 0 )
			return po.m_bTurns[PlayerOptions::TURN_BACKWARDS];
		if( iItem == 1 )
			return po.m_bTurns[PlayerOptions::TURN_SUPER_SHUFFLE];
		return false;
	case CAT_JUDGE:
		switch( iItem )
		{
		case 0: return po.m_iJudgment == PlayerOptions::EASY_JUDGMENT;
		case 1: return po.m_iJudgment == PlayerOptions::HARD_JUDGMENT;
		case 2: return po.m_iJudgment == PlayerOptions::VERY_HARD_JUDGMENT;
		case 3: return po.m_iJudgment == PlayerOptions::XTRA_HARD_JUDGMENT;
		case 4: return po.m_iJudgment == PlayerOptions::ULTRA_HARD_JUDGMENT;
		case 5: return po.m_bJudgmentReverse;
		case 6: return po.m_bJudgeByNote;
		case 7: return po.m_bFreePerformance;
		default: return false;
		}
	case CAT_RATE:
		if( iItem < 0 || iItem > 6 )
			return false;
		return CurrentSongOptions().m_fMusicRate == g_fRates[iItem];
	case CAT_BATTLE:
		return iItem < 5 && (int)GAMESTATE->m_BattleMode.Get() == iItem;
	case CAT_STEPSTYPES:
		switch( iItem )
		{
		case 0: return CurrentSongOptions().m_bShowSingles;
		case 1: return CurrentSongOptions().m_bShowHalfDoubles;
		case 2: return CurrentSongOptions().m_bShowDoubles;
		default: return false;
		}
	default:
		return false;
	}
}

void StepP1Mods::Reset( PlayerNumber pn, bool &bStepsTypesChanged )
{
	const SongOptions &so = CurrentSongOptions();
	bStepsTypesChanged = !so.m_bShowDoubles || !so.m_bShowSingles || !so.m_bShowHalfDoubles;

	ModsGroup<PlayerOptions> &po = GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions;
	PO_GROUP_CALL( po, ModsLevel_Preferred, Init );
	PO_GROUP_ASSIGN( po, ModsLevel_Preferred, m_fScrollSpeed, 2.0f );
	PO_GROUP_ASSIGN( po, ModsLevel_Preferred, m_sNoteSkin, RString("fiesta_2-prime") );
	SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_bUseBGAOff, false );
	SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_bUseBGADark, false );
	SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_fMusicRate, 1.0f );
	SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_bShowSingles, true );
	SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_bShowHalfDoubles, true );
	SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_bShowDoubles, true );
	if( GAMESTATE->m_PlayMode == PLAY_MODE_BATTLE )
		GAMESTATE->m_BattleMode.Set( (BattleMode)0 );
}
