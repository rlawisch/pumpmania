#include "global.h"
#include "ScreenSelectPlayModeBranch.h"
#include "ScreenManager.h"
#include "ProfileManager.h"
#include "Profile.h"
#include "GameState.h"
#include "MemoryCardManager.h"
#include "InputEventPlus.h"
#include "ThemeManager.h"
#include "GameSoundManager.h"
#include "MessageManager.h"

REGISTER_SCREEN_CLASS( ScreenSelectPlayModeBranch );

// Recreated from STEP_P1.exe (ScreenSelectPlayModeBranch::Init at 0x647c60).
void ScreenSelectPlayModeBranch::Init()
{
	ScreenWithMenuElements::Init();
	FOREACH_PlayerNumber( p )
	{
		m_bAlreadyJoined[p] = false;
		if( !GAMESTATE->m_bHasProfile[p] )
		{
			m_iSelectedProfiles[p] = -1;
			continue;
		}
		m_bAlreadyJoined[p] = true;
		m_iSelectedProfiles[p] = GAMESTATE->m_iProfileIndex[p];
		Message msg( "PlayerAlreadyJoined" );
		msg.SetParam( "Player", p );
		MESSAGEMAN->Broadcast( msg );
	}
	m_bTimedOut = false;
	m_bWaitingForFirstStart = true;
}

// Recreated from STEP_P1.exe (ScreenSelectPlayModeBranch::Input at 0x647f8b).
bool ScreenSelectPlayModeBranch::Input( const InputEventPlus &input )
{
	PlayerNumber pn = input.pn;
	if( IsTransitioning() )
		return false;
	if( m_fLockInputSecs > 0 )
		return false;
	if( input.type != IET_FIRST_PRESS )
		return false;
	if( GAMESTATE->m_bHasProfile[pn] )
		return false;

	switch( input.MenuI )
	{
	case GAME_BUTTON_MENULEFT:
	case PUMP_BUTTON_DOWNLEFT:
	{
		if( !GAMESTATE->IsHumanPlayer(pn) )
			return false;
		int iIndex = m_iSelectedProfiles[pn];
		if( iIndex < 1 )
			return true;
		if( iIndex == 1 && SetProfileIndex(pn, -1) )
		{
			Message msg( "HideProfileChanges" );
			msg.SetParam( "pn", pn );
			MESSAGEMAN->Broadcast( msg );
			return true;
		}
		if( !SetProfileIndex(pn, m_iSelectedProfiles[pn] - 1) )
			return true;
		BroadcastLocalProfileChange( pn, m_iSelectedProfiles[pn] - 1 );
		return true;
	}
	case GAME_BUTTON_MENURIGHT:
	case PUMP_BUTTON_DOWNRIGHT:
	{
		if( m_iSelectedProfiles[pn] == -1 )
			m_iSelectedProfiles[pn] = 0;
		if( !SetProfileIndex(pn, m_iSelectedProfiles[pn] + 1) )
			return true;
		BroadcastLocalProfileChange( pn, m_iSelectedProfiles[pn] - 1 );
		return true;
	}
	case GAME_BUTTON_START:
	case PUMP_BUTTON_CENTER:
	{
		if( m_bWaitingForFirstStart )
		{
			SCREENMAN->PlayStartSound();
			Message msg( "PlayerStartedSelectProfile" );
			msg.SetParam( "Player", pn );
			MESSAGEMAN->Broadcast( msg );
			m_bWaitingForFirstStart = false;
			if( !m_bTimedOut && PROFILEMAN->GetNumLocalProfiles() != 0 )
				return true;
			return Finish();
		}

		// both players picked the same local profile
		if( GAMESTATE->GetNumPlayersEnabled() == 2 &&
			m_iSelectedProfiles[PLAYER_1] != -1 &&
			m_iSelectedProfiles[PLAYER_1] == m_iSelectedProfiles[PLAYER_2] )
		{
			SOUND->PlayOnce( THEME->GetPathS("", "Sounds/BAD.mp3") );
			return true;
		}

		if( !Finish() )
			return true;
		FOREACH_PlayerNumber( p )
		{
			GAMESTATE->m_bHasProfile[p] = true;
			GAMESTATE->m_iProfileIndex[p] = m_iSelectedProfiles[p];
		}
		MESSAGEMAN->Broadcast( "FinishSelectProfile" );
		return true;
	}
	default:
		return false;
	}
}

// Recreated from STEP_P1.exe (ScreenSelectPlayModeBranch::Finish at 0x647d3a).
// Unlike ScreenSelectProfile::Finish, players that already chose a profile are
// left loaded and memory cards are not mounted or locked here.
bool ScreenSelectPlayModeBranch::Finish()
{
	if( GAMESTATE->GetNumPlayersEnabled() == 0 )
		return false;

	// if profile indexes are the same for both players
	if( GAMESTATE->GetNumPlayersEnabled() == 2 && m_iSelectedProfiles[0] == m_iSelectedProfiles[1] && m_iSelectedProfiles[0] > 0 )
		return false;

	FOREACH_PlayerNumber( p )
	{
		// card not ready
		if( m_iSelectedProfiles[p] == 0 && MEMCARDMAN->GetCardState( p ) != MemoryCardState_Ready )
			return false;

		// profile index too big
		if( m_iSelectedProfiles[p] > PROFILEMAN->GetNumLocalProfiles() )
			return false;
	}

	FOREACH_PlayerNumber( p )
	{
		if( m_bAlreadyJoined[p] )
			continue;

		PROFILEMAN->UnloadProfile( p );

		if( m_iSelectedProfiles[p] > 0 )
		{
			PROFILEMAN->m_sDefaultLocalProfileID[p].Set( PROFILEMAN->GetLocalProfileIDFromIndex( m_iSelectedProfiles[p] - 1 ) );
			PROFILEMAN->LoadLocalProfileFromMachine( p );
			GAMESTATE->LoadCurrentSettingsFromProfile(p);
		}
		if( m_iSelectedProfiles[p] == 0 && PROFILEMAN->LoadProfileFromMemoryCard( p, true ) )
		{
			GAMESTATE->LoadCurrentSettingsFromProfile(p);
			PROFILEMAN->SaveProfile( p );
		}
	}

	if( m_iSelectedProfiles[PLAYER_1] != -1 || m_iSelectedProfiles[PLAYER_2] != -1 )
		GAMESTATE->SetBasicMode( false );

	// Hidden noteskins: a good grade on these songs unlocks hns_1 / hns_2.
	FOREACH_PlayerNumber( p )
	{
		if( !PROFILEMAN->IsPersistentProfile(p) )
			continue;
		const Profile *pProfile = PROFILEMAN->GetProfile( p );
		if( pProfile == nullptr )
			continue;
		if( pProfile->HasGoodGradeInSongDir("Songs/11-EXCEED~ZERO/(1) C12 - What's Goin On/", 0, 2016) )
			GAMESTATE->m_bNoteSkin1Unlocked = true;
		else if( pProfile->HasGoodGradeInSongDir("Songs/14-FIESTA/1028 - Phantom -Intermezzo-/", 5, 2016) )
			GAMESTATE->m_bNoteSkin2Unlocked = true;
	}

	SOUND->PlayOnce( THEME->GetPathS("", "Sounds/START_2.MP3") );
	StartTransitioningScreen( SM_GoToNextScreen );
	return true;
}

// Recreated from STEP_P1.exe (ScreenSelectPlayModeBranch::HandleScreenMessage at 0x647baf).
void ScreenSelectPlayModeBranch::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_MenuTimer )
		m_bTimedOut = true;

	ScreenWithMenuElements::HandleScreenMessage( SM );
}
