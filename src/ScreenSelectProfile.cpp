#include "global.h"
#include "ScreenSelectProfile.h"
#include "ScreenManager.h"
#include "ProfileManager.h"
#include "Profile.h"
#include "GameState.h"
#include "MemoryCardManager.h"
#include "InputEventPlus.h"
#include "ScreenAttract.h"
#include "ThemeManager.h"
#include "GameSoundManager.h"
#include "MessageManager.h"

REGISTER_SCREEN_CLASS( ScreenSelectProfile );

// Recreated from STEP_P1.exe (ScreenSelectProfile::Init at 0x427474).
void ScreenSelectProfile::Init()
{
	FOREACH_PlayerNumber( p )
	{
		// no selection initially
		m_iSelectedProfiles[p] = -1;
	}
	m_bTimedOut = false;
	ScreenWithMenuElements::Init();
	m_bFinished = false;
	m_fLockInputSecs = 0;
	ScreenAttract::SetAttractVolume( true );
}

void ScreenSelectProfile::BroadcastLocalProfileChange( PlayerNumber pn, int iProfileIndex )
{
	const Profile *pProfile = PROFILEMAN->GetLocalProfileFromIndex( iProfileIndex );
	Message msg( "LocalProfileChange" );
	msg.SetParam( "name", pProfile->GetDisplayNameOrHighScoreName() );
	msg.SetParam( "pn", pn );
	msg.SetParam( "level", pProfile->m_iLevel );
	MESSAGEMAN->Broadcast( msg );
}

// Recreated from STEP_P1.exe (ScreenSelectProfile::Input at 0x427854).
// The pad buttons are checked as well as the menu buttons; no wrap-around and
// no repeat: only first presses count.
bool ScreenSelectProfile::Input( const InputEventPlus &input )
{
	PlayerNumber pn = input.pn;
	if( m_bFinished )
		return false;
	if( input.type != IET_FIRST_PRESS )
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
		if( !GAMESTATE->IsHumanPlayer(pn) )
			return false;
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
		if( !GAMESTATE->IsHumanPlayer(pn) )
		{
			GAMESTATE->JoinPlayer( pn );
			if( m_bTimedOut || PROFILEMAN->GetNumLocalProfiles() == 0 )
				return Finish();
			SCREENMAN->PlayStartSound();
			Message msg( "PlayerStartedSelectProfile" );
			msg.SetParam( "Player", pn );
			MESSAGEMAN->Broadcast( msg );
			return true;
		}

		// both players picked the same local profile
		if( GAMESTATE->GetNumPlayersEnabled() == 2 &&
			m_iSelectedProfiles[PLAYER_1] == m_iSelectedProfiles[PLAYER_2] &&
			m_iSelectedProfiles[PLAYER_1] != -1 )
		{
			SOUND->PlayOnce( THEME->GetPathS("", "Sounds/BAD.mp3") );
			return true;
		}

		if( !Finish() )
			return true;

		if( GAMESTATE->GetNumPlayersEnabled() != 2 )
		{
			GAMESTATE->m_bHasProfile[pn] = true;
			GAMESTATE->m_iProfileIndex[pn] = m_iSelectedProfiles[pn];
			MESSAGEMAN->Broadcast( "FinishSelectProfile" );
			m_fLockInputSecs = 10.0f;
			return true;
		}

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

bool ScreenSelectProfile::SetProfileIndex( PlayerNumber pn, int iProfileIndex )
{
	if( !GAMESTATE->IsHumanPlayer( pn ) )
	{
		if( iProfileIndex == -1 )
		{
			GAMESTATE->JoinPlayer( pn );
			SCREENMAN->PlayStartSound();
			return true;
		}
		return false;
	}

	if( iProfileIndex > PROFILEMAN->GetNumLocalProfiles() )
		return false;

	// wrong selection
	if( iProfileIndex < -2 )
		return false;

	// unload player
	if( iProfileIndex == -2 )
	{
		// GAMESTATE->UnjoinPlayer takes care of unloading the profile.
		GAMESTATE->UnjoinPlayer( pn );
		MEMCARDMAN->UnlockCard( pn );
		MEMCARDMAN->UnmountCard( pn );
		m_iSelectedProfiles[pn] = -1;
		return true;
	}

	m_iSelectedProfiles[pn] = iProfileIndex;
	return true;
}

// Recreated from STEP_P1.exe (ScreenSelectProfile::Finish at 0x4275a2).
bool ScreenSelectProfile::Finish()
{
	m_bFinished = true;

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

	// all ok - load profiles and go to next screen
	FOREACH_PlayerNumber( p )
	{
		MEMCARDMAN->UnlockCard( p );
		MEMCARDMAN->UnmountCard( p );
		PROFILEMAN->UnloadProfile( p );

		if( m_iSelectedProfiles[p] > 0 )
		{
			PROFILEMAN->m_sDefaultLocalProfileID[p].Set( PROFILEMAN->GetLocalProfileIDFromIndex( m_iSelectedProfiles[p] - 1 ) );
			PROFILEMAN->LoadLocalProfileFromMachine( p );
			GAMESTATE->LoadCurrentSettingsFromProfile(p);
		}
		if( m_iSelectedProfiles[p] == 0 )
		{
			MEMCARDMAN->WaitForCheckingToComplete();

			MEMCARDMAN->MountCard( p );
			bool bSuccess = PROFILEMAN->LoadProfileFromMemoryCard( p, true );	// load full profile
			MEMCARDMAN->UnmountCard( p );

			// Lock the card on successful load, so we won't allow it to be changed.
			if( bSuccess )
			{
				GAMESTATE->LoadCurrentSettingsFromProfile(p);
				MEMCARDMAN->MountCard( p );
				PROFILEMAN->SaveProfile( p );
				MEMCARDMAN->UnmountCard( p );
				MEMCARDMAN->LockCard( p );
			}
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

// Recreated from STEP_P1.exe (ScreenSelectProfile::HandleScreenMessage at 0x427515).
void ScreenSelectProfile::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_MenuTimer )
	{
		m_bTimedOut = true;
		MESSAGEMAN->Broadcast( "TimeOut" );
	}
	else if( SM == SM_LoseFocus )
	{
		ScreenAttract::SetAttractVolume( false );
	}

	ScreenWithMenuElements::HandleScreenMessage( SM );
}

// lua start
#include "LuaBinding.h"

/** @brief Allow Lua to have access to the ScreenSelectProfile. */
class LunaScreenSelectProfile: public Luna<ScreenSelectProfile>
{
public:
	static int SetProfileIndex( T* p, lua_State *L )
	{
		PlayerNumber pn = Enum::Check<PlayerNumber>(L, 1);
		int iProfileIndex = IArg(2);
		bool bRet = p->SetProfileIndex( pn, iProfileIndex );
		LuaHelpers::Push( L, bRet );
		return 1;
	}

	static int GetProfileIndex( T* p, lua_State *L )
	{
		PlayerNumber pn = Enum::Check<PlayerNumber>(L, 1);
		LuaHelpers::Push( L, p->GetProfileIndex( pn ) );
		return 1;
	}

	static int Finish( T* p, lua_State *L )
	{
		bool bRet = p->Finish();
		LuaHelpers::Push( L, bRet );
		return 1;
	}

	static int Cancel( T* p, lua_State *L )
	{
		p->Cancel( SM_GoToPrevScreen );
		return 1;
	}

	LunaScreenSelectProfile()
	{
		ADD_METHOD( SetProfileIndex );
		ADD_METHOD( GetProfileIndex );
		ADD_METHOD( Finish );
		ADD_METHOD( Cancel );
	}
};

LUA_REGISTER_DERIVED_CLASS( ScreenSelectProfile, ScreenWithMenuElements )

/*
 * Copyright (c) 2007 vdl
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
