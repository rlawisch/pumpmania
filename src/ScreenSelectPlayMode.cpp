#include "global.h"
#include "ScreenSelectPlayMode.h"
#include "ScreenManager.h"
#include "GameState.h"
#include "GameSoundManager.h"
#include "ThemeManager.h"
#include "InputEventPlus.h"
#include "MessageManager.h"

// Recreated from STEP_P1.exe (ScreenSelectPlayMode at 0x64742b-0x647b15).

REGISTER_SCREEN_CLASS( ScreenSelectPlayMode );

static void PlaySound( const RString &sPath )
{
	SOUND->PlayOnce( THEME->GetPathS("", sPath) );
}

void ScreenSelectPlayMode::Init()
{
	m_bDone = false;
	m_bCentered = false;
	m_Focus = FOCUS_ARCADE;
	ScreenWithMenuElements::Init();
}

void ScreenSelectPlayMode::TweenOnScreen()
{
	MESSAGEMAN->Broadcast( "NewScreen" );
	ScreenWithMenuElements::TweenOnScreen();
}

bool ScreenSelectPlayMode::Input( const InputEventPlus &input )
{
	if( m_bDone )
		return false;
	if( input.type != IET_FIRST_PRESS && input.type != IET_REPEAT )
		return false;

	switch( input.MenuI )
	{
	case GAME_BUTTON_MENULEFT:
	case PUMP_BUTTON_DOWNLEFT:
		if( m_Focus == FOCUS_BATTLE )
		{
			m_Focus = FOCUS_ARCADE;
			MESSAGEMAN->Broadcast( "FocusArcade" );
			PlaySound( "Sounds/ST_SELECT.WAV" );
			if( m_bCentered )
			{
				m_bCentered = false;
				this->PlayCommand( "HidePlat" );
				MESSAGEMAN->Broadcast( "Release" );
			}
			return true;
		}
		if( !m_bCentered )
			return false;
		m_bCentered = false;
		this->PlayCommand( "HidePlat" );
		MESSAGEMAN->Broadcast( "Release" );
		PlaySound( "Sounds/BAD.mp3" );
		return true;

	case GAME_BUTTON_MENURIGHT:
	case PUMP_BUTTON_DOWNRIGHT:
		if( m_Focus == FOCUS_ARCADE )
		{
			m_Focus = FOCUS_BATTLE;
			MESSAGEMAN->Broadcast( "FocusBattle" );
			PlaySound( "Sounds/ST_SELECT.WAV" );
			if( m_bCentered )
			{
				m_bCentered = false;
				this->PlayCommand( "HidePlat" );
				MESSAGEMAN->Broadcast( "Release" );
			}
			return true;
		}
		if( !m_bCentered )
			return false;
		m_bCentered = false;
		this->PlayCommand( "HidePlat" );
		MESSAGEMAN->Broadcast( "Release" );
		PlaySound( "Sounds/BAD.mp3" );
		return true;

	case GAME_BUTTON_START:
	case PUMP_BUTTON_CENTER:
		if( !m_bCentered )
		{
			MESSAGEMAN->Broadcast( "ShowPlat" );
			MESSAGEMAN->Broadcast( m_Focus == FOCUS_ARCADE ? "CenterForArcade" : "CenterForBattle" );
			PlaySound( "Sounds/CENTER.mp3" );
			m_bCentered = true;
			return true;
		}
		MESSAGEMAN->Broadcast( "ScreenFinish" );
		if( m_Focus == FOCUS_ARCADE )
		{
			MESSAGEMAN->Broadcast( "ScreenFinishForArcade" );
		}
		else
		{
			GAMESTATE->m_PlayMode.Set( PLAY_MODE_BATTLE );
			GAMESTATE->m_BattleMode.Set( (BattleMode)0 );
			MESSAGEMAN->Broadcast( "ScreenFinishForBattle" );
		}
		PlaySound( "Sounds/ST_OUT.mp3" );
		StartTransitioningScreen( SM_GoToNextScreen );
		m_bDone = true;
		return true;

	default:
		return false;
	}
}
