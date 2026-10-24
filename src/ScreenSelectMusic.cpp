#include "global.h"
#include "ScreenSelectMusic.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "SongManager.h"
#include "GameManager.h"
#include "GameSoundManager.h"
#include "RageLog.h"
#include "InputMapper.h"
#include "GameState.h"
#include "CodeDetector.h"
#include "ThemeManager.h"
#include "Steps.h"
#include "ActorUtil.h"
#include "Course.h"
#include "ProfileManager.h"
#include "MenuTimer.h"
#include "StepsUtil.h"
#include "Style.h"
#include "PlayerState.h"
#include "CommonMetrics.h"
#include "Song.h"
#include "SongUtil.h"
#include "InputEventPlus.h"
#include "NoteSkinManager.h"
#include "RageFileManager.h"
#include "MessageManager.h"
#include "StepP1Mods.h"
#include "ScreenDimensions.h"

// Recreated from STEP_P1.exe (ScreenSelectMusic at 0x41e5xx-0x4247xx).

static const char *SelectionStateNames[] = {
	"SelectingGroup",
	"SelectingSong",
	"SelectingSteps",
	"Finalized"
};
XToString( SelectionState );

AutoScreenMessage( SM_SongChanged );
AutoScreenMessage( SM_SortOrderChanging );
AutoScreenMessage( SM_SortOrderChanged );

static RageTimer g_PreviewTimer( RageZeroTimer );		// 0x7d93fc
static RageTimer g_GroupScrollTimer( RageZeroTimer );	// 0x7d9404
static RageTimer g_PlayModeBranchTimer( RageZeroTimer );// 0x7d940c
static RageTimer g_BackButtonsTimer( RageZeroTimer );

REGISTER_SCREEN_CLASS( ScreenSelectMusic );

ScreenSelectMusic::ScreenSelectMusic()
{
	m_iCurChannel = 0;
	m_bGoToPlayModeBranch = false;
	m_bGroupFastScroll = false;
	m_bGroupHeld = false;
	m_bPreviewPending = false;
	m_bQuestMode = false;
	m_bBattleMode = false;
	m_bBackSoundPlayed = false;
	m_bBackButtonsHeld = false;
	m_iGroupScrollDir = 1;
	m_iGroupScrollSpeed = 1;
	m_SelectionState = SelectionState_SelectingSong;
	m_CurCategory = SongCategory_Invalid;
	m_fSampleStartSeconds = 0;
	m_fSampleLengthSeconds = -1;
	m_iSampleVolume = 100;
	FOREACH_PlayerNumber( p )
	{
		m_bStepsChosen[p] = false;
		m_bSongChosen[p] = false;
		m_bAllowOptionsList[p] = true;
		m_iSelection[p] = 0;
	}
}

static void LoadSound( RageSound &snd, const RString &sPath, bool bSupportPan = false )
{
	RageSoundLoadParams params;
	params.m_bSupportPan = bSupportPan;
	snd.Load( THEME->GetPathS("", sPath), true, &params );
}

void ScreenSelectMusic::Init()
{
	CODES.Load( m_sName, "Codes" );

	m_GameButtonPreviousSong = INPUTMAPPER->GetInputScheme()->ButtonNameToIndex( THEME->GetMetric(m_sName, "PreviousSongButton") );
	m_GameButtonNextSong = INPUTMAPPER->GetInputScheme()->ButtonNameToIndex( THEME->GetMetric(m_sName, "NextSongButton") );
	m_GameButtonPrevState1 = INPUTMAPPER->GetInputScheme()->ButtonNameToIndex( THEME->GetMetric(m_sName, "PrevStateButton1") );
	m_GameButtonPrevState2 = INPUTMAPPER->GetInputScheme()->ButtonNameToIndex( THEME->GetMetric(m_sName, "PrevStateButton2") );

	LoadSound( m_soundOut, "Sounds/OUT", true );
	LoadSound( m_soundSongMove, "Sounds/MOVE", true );
	LoadSound( m_soundUnlock, "Sounds/N_TO_M", true );
	LoadSound( m_soundJoin, "Sounds/JOIN", true );
	LoadSound( m_soundCommandSet, "CW/S_CMD_SET", true );
	LoadSound( m_soundCheat, "Sounds/CHEAT", true );
	LoadSound( m_soundGroupMove, "Sounds/MOVE", true );
	LoadSound( m_soundBack, "Sounds/S_BACK", true );
	LoadSound( m_soundSelect, "Sounds/SELECT", true );
	LoadSound( m_soundStepsMove, "Sounds/D_MOVE", true );
	LoadSound( m_soundBad, "Sounds/BAD", true );
	FOREACH_ENUM( SongCategory, sc )
	{
		if( sc == SONGCATEGORY_USE_GENRE )
			continue;
		LoadSound( m_soundCategory[sc], "Categories/" + SongCategoryToString(sc), true );
	}

	ScreenWithMenuElements::Init();

	this->SubscribeToMessage( Message_PlayerJoined );
	this->SubscribeToMessage( Message_StepTypesToShowChanged );

	m_MusicWheel.SetName( "MusicWheel" );
	m_MusicWheel.Load( "MusicWheel" );
	m_MusicWheel.SetDrawOrder( 10 );
	LOAD_ALL_COMMANDS_AND_SET_XY( m_MusicWheel );
	this->AddChild( &m_MusicWheel );

	FOREACH_PlayerNumber( p )
	{
		if( GAMESTATE->m_bSideIsJoined[p] )
			m_ModList[p].Load( p, false );
		m_ModList[p].SetName( "ModList" + PlayerNumberToString(p) );
		m_ModList[p].SetXY( p == PLAYER_1 ? 24.0f : SCREEN_WIDTH - 24.0f, 130 );
		m_ModList[p].SetDrawOrder( 11 );
		this->AddChild( &m_ModList[p] );

		m_OptionsList[p].SetName( "OptionsList" + PlayerNumberToString(p) );
		m_OptionsList[p].SetModList( &m_ModList[p] );
		m_OptionsList[p].SetPlayer( p );
		m_OptionsList[p].SetDrawOrder( 92 );
		ActorUtil::LoadAllCommands( m_OptionsList[p], m_sName );
		m_OptionsList[p].SetVisible( false );
		this->AddChild( &m_OptionsList[p] );
	}
	m_OptionsList[PLAYER_1].Link( &m_OptionsList[PLAYER_2] );
	m_OptionsList[PLAYER_2].Link( &m_OptionsList[PLAYER_1] );

	m_Preview.SetName( "Preview" );
	LOAD_ALL_COMMANDS_AND_SET_XY( m_Preview );
	m_Preview.SetDrawOrder( 5 );
	this->AddChild( &m_Preview );
}

void ScreenSelectMusic::BeginScreen()
{
	m_vChannels.clear();
	SONGMAN->GetSongGroupNamesAvailables( m_vChannels );

	if( GAMESTATE->IsBasicMode() )
	{
		if( SONGMAN->DoesSongGroupExist(GAMESTATE->m_sBasicModeGroupName) )
		{
			GAMESTATE->m_sCurrentGroupName = GAMESTATE->m_sBasicModeGroupName;
		}
		else
		{
			SCREENMAN->SystemMessage( "BasicModeGroup is not installed. The game will show All Tunes instead" );
			GAMESTATE->m_sCurrentGroupName = "SO_ALLTUNES";
			m_iCurChannel = 0;
		}
	}
	else
	{
		bool bFound = false;
		for( unsigned i = 0; i < m_vChannels.size(); ++i )
		{
			if( m_vChannels[i] == GAMESTATE->m_sCurrentGroupName )
			{
				m_iCurChannel = i;
				bFound = true;
				break;
			}
		}
		if( !bFound )
		{
			GAMESTATE->m_sCurrentGroupName = m_vChannels[0];
			m_iCurChannel = 0;
		}
	}
	m_sCurChannel = GAMESTATE->m_sCurrentGroupName;

	if( GAMESTATE->m_sCurrentGroupName == "SO_QUEST" || GAMESTATE->m_sCurrentGroupName == "04-SKILLUP ZONE" )
		m_bQuestMode = true;

	// Preload the tap notes of every noteskin and the default preview.
	vector<RString> vsNoteSkins;
	NOTESKIN->GetNoteSkinNames( vsNoteSkins );
	for( unsigned i = 0; i < vsNoteSkins.size(); ++i )
	{
		LockNoteSkin l( vsNoteSkins[i] );
		m_TexturePreload.Load( NOTESKIN->GetPath("UpLeft", "Tap Note") );
	}
	m_TexturePreload.Load( THEME->GetPathG("", "Common preview") );

	if( GAMESTATE->m_PlayMode == PLAY_MODE_BATTLE )
		m_bBattleMode = true;

	vector<StepsType> vst;
	GAMEMAN->GetStepsTypesForGame( GAMESTATE->m_pCurGame, vst );
	const Style *pStyle = GAMEMAN->GetFirstCompatibleStyle( GAMESTATE->m_pCurGame, GAMESTATE->GetNumSidesJoined(), vst[0] );
	GAMESTATE->SetCurrentStyle( pStyle );
	if( GAMESTATE->GetCurrentStyle() == nullptr )
	{
		LuaHelpers::ReportScriptError( "The Style has not been set.  A theme must set the Style before loading ScreenSelectMusic." );
		pStyle = GAMEMAN->GetFirstCompatibleStyle( GAMESTATE->m_pCurGame, GAMESTATE->GetNumSidesJoined(), vst[0] );
		GAMESTATE->SetCurrentStyle( pStyle );
	}

	if( GAMESTATE->m_PlayMode == PlayMode_Invalid )
	{
		GAMESTATE->m_PlayMode.Set( PLAY_MODE_REGULAR );
		LOG->Trace( "ScreenSelectMusic::PlayMode not set, setting as regular." );
	}

	m_SelectionState = SelectionState_SelectingSong;
	m_MusicWheel.BeginScreen();
	m_MusicWheel.OpenGroupOrChangeSort( GAMESTATE->m_sCurrentGroupName );
	m_MusicWheel.CheckCurrentSongOnWheel();
	m_MusicWheel.CloseUpWheel( true );

	if( m_MusicWheel.GetSelectedSong() != nullptr )
	{
		m_CurCategory = m_MusicWheel.GetSelectedSong()->m_SongCategory;
		m_sCategoryName = "";
	}
	else if( m_MusicWheel.GetSelectedCourse() != nullptr )
	{
		m_CurCategory = SongCategory_Invalid;
		m_sCategoryName = "";
	}

	FOREACH_PlayerNumber( p )
	{
		m_bStepsChosen[p] = false;
		m_bSongChosen[p] = false;
		m_iSelection[p] = 0;
	}
	FOREACH_HumanPlayer( p )
		m_iSelection[p] = GAMESTATE->m_bPlayerChartIndex[p];

	AfterMusicChange();
	ScreenWithMenuElements::BeginScreen();
	g_PreviewTimer.Touch();
	if( GAMESTATE->m_PlayMode == PLAY_MODE_BATTLE )
		m_soundJoin.Play( false );
}

ScreenSelectMusic::~ScreenSelectMusic()
{
	LOG->Trace( "ScreenSelectMusic::~ScreenSelectMusic()" );
	SOUND->StopMusic();
}

SelectionState ScreenSelectMusic::GetNextSelectionState() const
{
	switch( m_SelectionState )
	{
	case SelectionState_SelectingGroup:	return SelectionState_SelectingSong;
	case SelectionState_SelectingSong:	return SelectionState_SelectingSteps;
	default:							return SelectionState_Finalized;
	}
}

SelectionState ScreenSelectMusic::GetPrevSelectionState( bool &bValid ) const
{
	switch( m_SelectionState )
	{
	case SelectionState_SelectingSong:	return SelectionState_SelectingGroup;
	case SelectionState_SelectingSteps:	return SelectionState_SelectingSong;
	default:
		bValid = false;
		return m_SelectionState;
	}
}

RString ScreenSelectMusic::NextChannel()
{
	++m_iCurChannel;
	if( m_iCurChannel >= (int)m_vChannels.size() )
		m_iCurChannel = 0;
	return m_vChannels[m_iCurChannel];
}

RString ScreenSelectMusic::PrevChannel()
{
	--m_iCurChannel;
	if( m_iCurChannel < 0 )
		m_iCurChannel = m_vChannels.size() - 1;
	return m_vChannels[m_iCurChannel];
}

void ScreenSelectMusic::ChangeChannel( int iDir )
{
	m_sCurChannel = iDir > 0 ? NextChannel() : PrevChannel();
	m_soundGroupMove.Stop();
	m_soundGroupMove.Play( false );

	Message msg( "ChangeGroup" );
	msg.SetParam( "Group", m_sCurChannel );
	msg.SetParam( "Dir", iDir );
	MESSAGEMAN->Broadcast( msg );
	MESSAGEMAN->Broadcast( iDir > 0 ? "NextGroup" : "PrevGroup" );
}

void ScreenSelectMusic::Update( float fDeltaTime )
{
	if( m_SelectionState == SelectionState_SelectingGroup )
	{
		// Holding both back buttons for 3 seconds leaves the screen.
		if( INPUTMAPPER->IsBeingPressed(m_GameButtonPrevState1, PLAYER_INVALID) &&
			INPUTMAPPER->IsBeingPressed(m_GameButtonPrevState2, PLAYER_INVALID) )
		{
			if( !m_bBackButtonsHeld )
				g_BackButtonsTimer.Touch();
			m_bBackButtonsHeld = true;
		}
		else
		{
			m_bBackButtonsHeld = false;
		}
		if( g_BackButtonsTimer.Ago() > 3.0f && m_bBackButtonsHeld )
		{
			if( !m_bBackSoundPlayed )
			{
				m_soundBack.Play( false );
				m_bBackSoundPlayed = true;
			}
			MenuBack( InputEventPlus() );
		}

		if( !m_bGroupFastScroll )
		{
			if( m_bGroupHeld )
			{
				bool bHeld = false;
				FOREACH_HumanPlayer( p )
				{
					if( INPUTMAPPER->IsBeingPressed(m_GameButtonPreviousSong, p) )
					{
						m_iGroupScrollDir = -1;
						bHeld = true;
					}
					if( INPUTMAPPER->IsBeingPressed(m_GameButtonNextSong, p) )
					{
						m_iGroupScrollDir = 1;
						bHeld = true;
					}
				}
				if( bHeld && g_GroupScrollTimer.Ago() > 0.8f )
				{
					m_bGroupFastScroll = true;
					m_iGroupScrollSpeed = 1;
					g_GroupScrollTimer.Touch();
					ChangeChannel( m_iGroupScrollDir );
				}
			}
		}
		else
		{
			float fDelay;
			switch( m_iGroupScrollSpeed )
			{
			case 1:		fDelay = 0.21f; break;
			case 2:		fDelay = 0.185f; break;
			case 3:		fDelay = 0.14f; break;
			default:	fDelay = 0.114f; break;
			}
			if( g_GroupScrollTimer.Ago() > fDelay )
			{
				if( m_iGroupScrollSpeed < 5 )
					++m_iGroupScrollSpeed;
				ChangeChannel( m_iGroupScrollDir );
				g_GroupScrollTimer.Touch();
			}
		}
	}

	ScreenWithMenuElements::Update( fDeltaTime );

	if( m_bGoToPlayModeBranch && g_PlayModeBranchTimer.Ago() >= 1.3f )
	{
		m_sNextScreen = "ScreenSelectPlayModeBranch";
		StartTransitioningScreen( SM_GoToNextScreen );
	}

	UpdatePreview( false );
}

void ScreenSelectMusic::UpdatePreview( bool bForce )
{
	if( IsTransitioning() || m_bGoToPlayModeBranch )
		return;
	if( m_SelectionState == SelectionState_SelectingGroup || m_SelectionState == SelectionState_Finalized )
		return;
	if( !(m_MusicWheel.IsSettled() || m_MusicWheel.WheelIsLocked() || bForce) || !m_bPreviewPending )
		return;
	if( g_PreviewTimer.Ago() < 0.8f && !bForce )
		return;

	m_bPreviewPending = false;
	if( !m_sPreviewPath.empty() )
	{
		m_Preview.SetDiffuseAlpha( 1 );
		m_Preview.LoadFromCachedBanner( m_sPreviewPath );
		m_Preview.EnableAnimation( true );
		if( m_Preview.GetTexture() != nullptr )
			m_Preview.GetTexture()->SetLooping( true );
	}
	else
	{
		m_Preview.Load( THEME->GetPathG("", "Common preview") );
		m_Preview.EnableAnimation( true );
	}
	m_Preview.SetDiffuseAlpha( 1 );
	MESSAGEMAN->Broadcast( "PreviewReady" );

	GameSoundManager::PlayMusicParams params;
	RString sIntro = GAMESTATE->m_pCurSong->GetSongDir() + "/INTRO.mp3";
	if( IsAFile(sIntro) )
	{
		params.sFile = sIntro;
	}
	else
	{
		params.sFile = m_sSampleMusicToPlay;
		params.fStartSecond = m_fSampleStartSeconds;
		params.fLengthSeconds = m_fSampleLengthSeconds;
		params.iVolume = m_iSampleVolume;
		params.pTiming = nullptr;
		params.bForceLoop = false;
		params.bApplyMusicRate = false;
		params.fFadeOutLengthSeconds = 1.5f;
	}
	SOUND->PlayMusic( params, GameSoundManager::PlayMusicParams() );
}

void ScreenSelectMusic::OpenOptionsList( PlayerNumber pn )
{
	if( pn == PLAYER_INVALID )
		return;
	m_MusicWheel.Move( 0 );
	m_OptionsList[pn].SetVisible( true );
	m_OptionsList[pn].Open();
	m_bGroupHeld = false;
}

// Recreated from STEP_P1.exe (0x42148d): a code that brings Basic Mode back to the full channel list.
bool ScreenSelectMusic::LeaveBasicMode( const InputEventPlus &input )
{
	if( !CodeDetector::EnteredCode(input.GameI.controller, CODE_CANCEL_ALL_PLAYER_OPTIONS) )
		return false;

	if( GAMESTATE->IsAnExtraStageAndSelectionLocked() )
	{
		m_soundBad.Play( false );
		return true;
	}
	if( GAMESTATE->IsBasicMode() )
	{
		m_sCurChannel = "SO_ALLTUNES";
		m_iCurChannel = 0;
		m_MusicWheel.OpenGroupOrChangeSort( m_sCurChannel );
		m_soundUnlock.Play( false );
		GAMESTATE->SetBasicMode( false );
		GAMESTATE->m_sCurrentGroupName = m_sCurChannel;
		MESSAGEMAN->Broadcast( "GoFullMode" );
		AfterMusicChange();
	}
	return true;
}

bool ScreenSelectMusic::Input( const InputEventPlus &input )
{
	if( m_fLockInputSecs > 0 )
	{
		if( input.type != IET_FIRST_PRESS )
			return false;
		return LeaveBasicMode( input );
	}

	// ESC ends the game: save the profiles and go to Game Over (PrevScreen).
	if( input.DeviceI.button == KEY_ESC && input.type == IET_FIRST_PRESS &&
		m_SelectionState != SelectionState_Finalized && !IsTransitioning() )
	{
		m_soundBack.Play( false );
		return MenuBack( input );
	}

	if( !input.GameI.IsValid() )
		return false;

	if( m_SelectionState != SelectionState_Finalized &&
		input.MenuI == GAME_BUTTON_START && input.type == IET_FIRST_PRESS &&
		GAMESTATE->JoinInput(input.pn) )
	{
		return true;
	}

	if( !GAMESTATE->IsHumanPlayer(input.pn) )
		return false;
	if( m_SelectionState == SelectionState_Finalized )
		return false;
	if( IsTransitioning() )
		return false;

	if( !m_MusicWheel.IsRouletting() )
	{
		PlayerNumber pn = input.pn;
		if( pn != PLAYER_INVALID )
		{
			if( m_OptionsList[pn].IsOpened() )
				return m_OptionsList[pn].Input( input );
			if( input.type == IET_FIRST_PRESS && input.MenuI == GAME_BUTTON_SELECT )
			{
				m_OptionsList[pn].Open();
				m_bGroupHeld = false;
				return true;
			}
		}

		if( input.type != IET_FIRST_PRESS ||
			(input.MenuI != m_GameButtonPrevState1 && input.MenuI != m_GameButtonPrevState2) )
		{
			switch( m_SelectionState )
			{
			case SelectionState_SelectingGroup:
				if( input.type == IET_RELEASE &&
					(input.MenuI == m_GameButtonNextSong || input.MenuI == m_GameButtonPreviousSong) )
				{
					m_bGroupFastScroll = false;
					g_GroupScrollTimer.Touch();
					m_bGroupHeld = false;
				}
				if( input.type != IET_FIRST_PRESS )
					break;
				if( input.MenuI == m_GameButtonPreviousSong )
				{
					m_bGroupFastScroll = false;
					g_GroupScrollTimer.Touch();
					m_bGroupHeld = true;
					ChangeChannel( -1 );
				}
				if( input.MenuI == m_GameButtonNextSong )
				{
					m_bGroupFastScroll = false;
					g_GroupScrollTimer.Touch();
					m_bGroupHeld = true;
					ChangeChannel( +1 );
				}
				break;

			case SelectionState_SelectingSong:
				if( input.MenuI == m_GameButtonNextSong || input.MenuI == m_GameButtonPreviousSong )
				{
					if( m_MusicWheel.IsRouletting() )
						return false;

					bool bLeftIsDown = false;
					bool bRightIsDown = false;
					FOREACH_HumanPlayer( p )
					{
						bLeftIsDown |= INPUTMAPPER->IsBeingPressed( m_GameButtonPreviousSong, p );
						bRightIsDown |= INPUTMAPPER->IsBeingPressed( m_GameButtonNextSong, p );
					}

					if( bLeftIsDown && bRightIsDown )
					{
						m_MusicWheel.Move( 0 );
						if( input.type == IET_FIRST_PRESS )
						{
							if( input.MenuI == m_GameButtonPreviousSong )
								m_MusicWheel.ChangeMusicUnlessLocked( -1 );
							else if( input.MenuI == m_GameButtonNextSong )
								m_MusicWheel.ChangeMusicUnlessLocked( +1 );
						}
					}
					else if( bLeftIsDown )
					{
						if( input.type != IET_RELEASE )
							m_MusicWheel.Move( -1 );
					}
					else if( bRightIsDown )
					{
						if( input.type != IET_RELEASE )
							m_MusicWheel.Move( +1 );
					}
					else
					{
						m_MusicWheel.Move( 0 );
					}

					if( input.type == IET_RELEASE )
					{
						INPUTMAPPER->ResetKeyRepeat( m_GameButtonPreviousSong, input.pn );
						INPUTMAPPER->ResetKeyRepeat( m_GameButtonNextSong, input.pn );
					}
				}
				break;

			case SelectionState_SelectingSteps:
				if( (input.MenuI == m_GameButtonNextSong || input.MenuI == m_GameButtonPreviousSong) &&
					input.type == IET_FIRST_PRESS )
				{
					// In battle both players move together.
					vector<PlayerNumber> vpns;
					if( m_bBattleMode )
					{
						FOREACH_PlayerNumber( p )
							vpns.push_back( p );
					}
					else
					{
						vpns.push_back( input.pn );
					}
					for( unsigned i = 0; i < vpns.size(); ++i )
					{
						PlayerNumber p = vpns[i];
						if( m_bStepsChosen[p] )
						{
							Message msg( "StepsPreselectedCancelled" );
							msg.SetParam( "Player", p );
							MESSAGEMAN->Broadcast( msg );
							m_bStepsChosen[p] = false;
						}
						if( input.MenuI == m_GameButtonNextSong )
							ChangeSteps( p, +1 );
						if( input.MenuI == m_GameButtonPreviousSong )
							ChangeSteps( p, -1 );
					}
				}
				break;

			case SelectionState_Finalized:
				return true;
			default:
				return false;
			}

			if( input.type == IET_FIRST_PRESS && LeaveBasicMode(input) )
				return true;
		}
		else
		{
			// Back one state with the PrevState buttons.
			bool bLocked = false;
			if( m_SelectionState == SelectionState_SelectingSong && GAMESTATE->IsBasicMode() )
				bLocked = true;
			else if( (m_SelectionState == SelectionState_SelectingSong || m_SelectionState == SelectionState_SelectingSteps) &&
				m_MusicWheel.WheelIsLocked() )
				bLocked = true;

			bool bValid = true;
			SelectionState prev = bLocked ? m_SelectionState : GetPrevSelectionState( bValid );
			if( bLocked || !bValid )
			{
				m_soundBad.Play( false );
				return true;
			}

			m_SelectionState = prev;
			m_MusicWheel.Move( 0 );
			Message msg( "GoBack" + SelectionStateToString(m_SelectionState) );
			m_MusicWheel.HandleMessageOver( msg );
			MESSAGEMAN->Broadcast( msg );

			if( m_SelectionState == SelectionState_SelectingSong )
			{
				m_soundSongMove.Play( false );
				FOREACH_HumanPlayer( p )
					m_bStepsChosen[p] = false;
				m_bGroupFastScroll = false;
				m_bGroupHeld = false;
				m_iGroupScrollSpeed = 1;
				g_GroupScrollTimer.Touch();
				m_MusicWheel.CloseUpWheel( false );
				FOREACH_PlayerNumber( p )
				{
					m_bAllowOptionsList[p] = true;
					m_OptionsList[p].SetEnabled( true );
				}
			}
			if( m_SelectionState == SelectionState_SelectingGroup )
			{
				m_soundBack.Play( false );
				m_MusicWheel.OpenUpWheel( true );
				SOUND->StopMusic();
			}
			else
			{
				MESSAGEMAN->Broadcast( input.MenuI == m_GameButtonPrevState1 ? "PrevStateButton1" : "PrevStateButton2" );
			}
			return true;
		}
	}

	return ScreenWithMenuElements::Input( input );
}

void ScreenSelectMusic::ChangeSteps( PlayerNumber pn, int dir )
{
	ASSERT_M( GAMESTATE->IsHumanPlayer(pn), "ScreenSelectMusic::Changed steps by a non-human player" );

	if( GAMESTATE->m_pCurSong )
	{
		m_iSelection[pn] += dir;
		wrap( m_iSelection[pn], m_vpSteps.size() );
		Steps *pSteps = m_vpSteps[m_iSelection[pn]];
		GAMESTATE->ChangePreferredDifficultyAndStepsType( pn, pSteps->GetDifficulty(), pSteps->m_StepsType );
	}
	else if( GAMESTATE->m_pCurCourse )
	{
		m_iSelection[pn] += dir;
		wrap( m_iSelection[pn], m_vpTrails.size() );
		Trail *pTrail = m_vpTrails[m_iSelection[pn]];
		GAMESTATE->ChangePreferredCourseDifficultyAndStepsType( pn, pTrail->m_CourseDifficulty, pTrail->m_StepsType );
	}
	else
	{
		if( CommonMetrics::AUTO_SET_STYLE )
			return;
		if( !GAMESTATE->ChangePreferredDifficulty(pn, dir) )
			return;
	}

	vector<PlayerNumber> vpns;
	FOREACH_HumanPlayer( p )
	{
		if( pn == p || GAMESTATE->DifficultiesLocked() )
		{
			m_iSelection[p] = m_iSelection[pn];
			vpns.push_back( p );
		}
	}
	AfterStepsOrTrailChange( vpns );

	m_soundStepsMove.Stop();
	float fBalance = 0;
	if( GAMESTATE->GetNumSidesJoined() == 2 )
		fBalance = pn == PLAYER_1 ? -1.0f : 1.0f;
	m_soundStepsMove.SetProperty( "Pan", fBalance );
	m_soundStepsMove.Play( false );

	Message msg( "ChangeSteps" );
	msg.SetParam( "Player", pn );
	msg.SetParam( "Direction", dir );
	MESSAGEMAN->Broadcast( msg );
}

void ScreenSelectMusic::AfterStepsOrTrailChange( const vector<PlayerNumber> &vpns )
{
	for( unsigned i = 0; i < vpns.size(); ++i )
	{
		PlayerNumber pn = vpns[i];
		ASSERT( GAMESTATE->IsHumanPlayer(pn) );

		if( GAMESTATE->m_pCurSong )
		{
			if( m_iSelection[pn] > (int)m_vpSteps.size() - 1 )
				m_iSelection[pn] = 0;
			Steps *pSteps = m_vpSteps.empty() ? nullptr : m_vpSteps[m_iSelection[pn]];
			GAMESTATE->m_pCurSteps[pn].Set( pSteps );
			GAMESTATE->m_pCurTrail[pn].Set( nullptr );
		}
		else if( GAMESTATE->m_pCurCourse )
		{
			CLAMP( m_iSelection[pn], 0, (int)m_vpTrails.size() - 1 );
			Trail *pTrail = m_vpTrails.empty() ? nullptr : m_vpTrails[m_iSelection[pn]];
			GAMESTATE->m_pCurSteps[pn].Set( nullptr );
			GAMESTATE->m_pCurTrail[pn].Set( pTrail );
		}
	}
}

void ScreenSelectMusic::UpdateCategory()
{
	Song *pSong = m_MusicWheel.GetSelectedSong();
	if( pSong == nullptr )
		return;

	if( pSong->m_SongCategory == m_CurCategory )
	{
		if( m_CurCategory == SONGCATEGORY_USE_GENRE && pSong->m_sGenre != m_sCategoryName )
		{
			m_sCategoryName = pSong->m_sGenre;
			Message msg( "CurrentCategoryChanged" );
			msg.SetParam( "Category", m_sCategoryName );
			MESSAGEMAN->Broadcast( msg );
		}
		return;
	}

	if( m_CurCategory != SONGCATEGORY_USE_GENRE && m_CurCategory != SongCategory_Invalid && m_soundCategory[m_CurCategory].IsPlaying() )
		m_soundCategory[m_CurCategory].Stop();

	m_CurCategory = pSong->m_SongCategory;
	Message msg( "CurrentCategoryChanged" );
	if( m_CurCategory == SONGCATEGORY_USE_GENRE )
	{
		m_sCategoryName = pSong->m_sGenre;
		msg.SetParam( "Category", pSong->m_sGenre );
	}
	else
	{
		msg.SetParam( "Category", SongCategoryToString(m_CurCategory) );
		if( !m_MusicWheel.IsRouletting() && m_CurCategory != SongCategory_Invalid )
			m_soundCategory[m_CurCategory].Play( false );
	}
	MESSAGEMAN->Broadcast( msg );
	if( !m_MusicWheel.IsRouletting() )
		m_MusicWheel.HandleMessageOver( msg );
}

void ScreenSelectMusic::AfterMusicChange()
{
	Song *pSong = m_MusicWheel.GetSelectedSong();
	GAMESTATE->m_pCurSong.Set( pSong );
	if( pSong )
		GAMESTATE->m_pPreferredSong = pSong;

	Course *pCourse = m_MusicWheel.GetSelectedCourse();
	GAMESTATE->m_pCurCourse.Set( pCourse );
	if( pCourse )
		GAMESTATE->m_pPreferredCourse = pCourse;

	m_vpSteps.clear();
	m_vpTrails.clear();

	if( m_MusicWheel.WheelIsLocked() )
	{
		FOREACH_PlayerNumber( p )
		{
			m_bAllowOptionsList[p] = false;
			m_OptionsList[p].SetEnabled( false );
		}
	}

	m_sPreviewPath = "";
	m_Preview.UnloadTexture();
	m_bPreviewPending = false;

	WheelItemDataType wtype = m_MusicWheel.GetSelectedType();
	switch( wtype )
	{
	case WheelItemDataType_Section:
		break;
	case WheelItemDataType_Song:
	{
		m_sSampleMusicToPlay = pSong->GetMusicPath();
		m_fSampleStartSeconds = pSong->m_fMusicSampleStartSeconds;
		m_fSampleLengthSeconds = pSong->m_fMusicSampleLengthSeconds;
		m_iSampleVolume = pSong->m_iVolume;
		m_bPreviewPending = true;
		UpdateCategory();

		SongUtil::GetPlayableSteps( pSong, m_vpSteps, StringToSortOrder(GAMESTATE->m_sCurrentGroupName) );
		int iMaxSteps = PREFSMAN->m_bExtendedStepsList ? 14 : 12;
		FOREACH_HumanPlayer( p )
		{
			if( m_iSelection[p] >= (int)m_vpSteps.size() || m_iSelection[p] >= iMaxSteps )
				m_iSelection[p] = 0;
		}
		MESSAGEMAN->Broadcast( "PlayableStepsChanged" );

		m_sPreviewPath = pSong->HasPreviewVid() ? pSong->GetPreviewVidPath() : pSong->GetBannerPath();
		break;
	}
	case WheelItemDataType_Roulette:
	case WheelItemDataType_Random:
	case WheelItemDataType_Portal:
		break;
	case WheelItemDataType_Course:
	{
		const Style *pStyle = nullptr;
		if( CommonMetrics::AUTO_SET_STYLE )
			pStyle = pCourse->GetCourseStyle( GAMESTATE->m_pCurGame, GAMESTATE->GetNumSidesJoined() );
		if( pStyle == nullptr )
			pStyle = GAMESTATE->GetCurrentStyle();
		pCourse->GetTrails( m_vpTrails, pStyle->m_StepsType );

		FOREACH_HumanPlayer( p )
			m_iSelection[p] = 0;
		m_fSampleStartSeconds = 0;
		m_iSampleVolume = 100;
		m_fSampleLengthSeconds = -1;
		MESSAGEMAN->Broadcast( "PlayableStepsChanged" );
		m_sPreviewPath = pCourse->GetBannerPath();
		break;
	}
	case WheelItemDataType_Sort:
		break;
	case WheelItemDataType_Custom:
		MESSAGEMAN->Broadcast( "PlayableStepsChanged" );
		break;
	default:
		FAIL_M( ssprintf("Invalid WheelItemDataType: %i", wtype) );
	}

	if( m_SelectionState != SelectionState_SelectingGroup )
		MESSAGEMAN->Broadcast( "PreviewChanged" );

	SOUND->StopMusic();
	g_PreviewTimer.Touch();

	vector<PlayerNumber> vpns;
	FOREACH_HumanPlayer( p )
		vpns.push_back( p );
	AfterStepsOrTrailChange( vpns );
}

bool ScreenSelectMusic::MenuStart( const InputEventPlus &input )
{
	if( input.type != IET_FIRST_PRESS )
		return false;
	if( input.pn != PLAYER_INVALID && INPUTMAPPER->IsBeingPressed(GAME_BUTTON_SELECT, input.pn) )
		return false;
	if( m_fLockInputSecs > 0 )
		return false;

	switch( m_SelectionState )
	{
	case SelectionState_SelectingGroup:
		if( m_sCurChannel != GAMESTATE->m_sCurrentGroupName )
		{
			GAMESTATE->m_sCurrentGroupName = m_sCurChannel;
			m_MusicWheel.OpenGroupOrChangeSort( m_sCurChannel );
		}
		if( GAMESTATE->m_sCurrentGroupName == "SO_QUEST" || GAMESTATE->m_sCurrentGroupName == "04-SKILLUP ZONE" )
		{
			m_bQuestMode = true;
			MESSAGEMAN->Broadcast( Message("ShowGreenStuffs") );
		}
		else if( m_bQuestMode )
		{
			m_bQuestMode = false;
			MESSAGEMAN->Broadcast( Message("HideGreenStuffs") );
		}
		m_bGroupFastScroll = false;
		m_bGroupHeld = false;
		m_iGroupScrollSpeed = 1;
		AfterMusicChange();
		m_MusicWheel.CloseUpWheel( true );
		break;

	case SelectionState_SelectingSong:
	{
		if( !m_MusicWheel.Select() )
			return false;
		m_MusicWheel.StopWheel();
		m_MusicWheel.OpenUpWheel( false );

		if( m_MusicWheel.GetSelectedSong() != nullptr )
		{
			if( GAMESTATE->IsCourseMode() )
				GAMESTATE->m_PlayMode.Set( PLAY_MODE_REGULAR );
		}
		else
		{
			Course *pCourse = m_MusicWheel.GetSelectedCourse();
			if( pCourse == nullptr )
			{
				m_soundBad.Play( false );
				return false;
			}
			GAMESTATE->m_PlayMode.Set( pCourse->GetPlayMode() );
			FOREACH_HumanPlayer( p )
			{
				if( pCourse->m_iLives != -1 )
				{
					PO_GROUP_ASSIGN( GAMESTATE->m_pPlayerState[p]->m_PlayerOptions, ModsLevel_Stage, m_LifeType, LifeType_Battery );
					PO_GROUP_ASSIGN( GAMESTATE->m_pPlayerState[p]->m_PlayerOptions, ModsLevel_Stage, m_BatteryLives, pCourse->m_iLives );
				}
				if( pCourse->GetCourseType() == COURSE_TYPE_SURVIVAL )
					PO_GROUP_ASSIGN( GAMESTATE->m_pPlayerState[p]->m_PlayerOptions, ModsLevel_Stage, m_LifeType, LifeType_Time );
			}
		}
		MESSAGEMAN->Broadcast( "SongChosen" );
		FOREACH_PlayerNumber( p )
		{
			m_bAllowOptionsList[p] = false;
			m_OptionsList[p].SetEnabled( false );
		}
		break;
	}

	case SelectionState_SelectingSteps:
	{
		PlayerNumber pn = input.pn;
		bool bInitiatedByMenuTimer = pn == PLAYER_INVALID;
		bool bAllOtherHumanPlayersDone = true;
		FOREACH_HumanPlayer( p )
		{
			if( p != pn )
				bAllOtherHumanPlayersDone &= m_bSongChosen[p];
		}
		bool bAllPlayersDoneSelectingSteps = bInitiatedByMenuTimer || bAllOtherHumanPlayersDone;
		bool bGoToNext = false;
		bool bRoutineChecked = false;

		if( !GAMESTATE->IsCourseMode() && GAMESTATE->GetNumSidesJoined() == 2 )
		{
			bool bSelectedRoutineSteps[NUM_PLAYERS] = { false, false };
			bool bAnySelectedRoutine = false;
			bool bSelectedSameSteps = GAMESTATE->m_pCurSteps[PLAYER_1] == GAMESTATE->m_pCurSteps[PLAYER_2];
			FOREACH_HumanPlayer( p )
			{
				const Steps *pSteps = GAMESTATE->m_pCurSteps[p];
				const StepsTypeInfo &sti = GAMEMAN->GetStepsTypeInfo( pSteps->m_StepsType );
				bSelectedRoutineSteps[p] = sti.m_StepsTypeCategory == StepsTypeCategory_Routine;
				bAnySelectedRoutine |= bSelectedRoutineSteps[p];
			}
			if( bAnySelectedRoutine )
			{
				bRoutineChecked = true;
				if( bInitiatedByMenuTimer )
				{
					if( !bSelectedSameSteps )
					{
						ASSERT( m_vpSteps.size() != 0 );
						FOREACH_PlayerNumber( p )
						{
							if( bSelectedRoutineSteps[p] )
								GAMESTATE->m_pCurSteps[p].Set( m_vpSteps[0] );
						}
						bGoToNext = true;
					}
					else
					{
						bRoutineChecked = false;
					}
				}
				else if( !bSelectedSameSteps && m_bStepsChosen[pn] )
				{
					m_soundBad.Play( false );
					return false;
				}
			}
		}

		if( !bGoToNext )
		{
			if( !bRoutineChecked && bInitiatedByMenuTimer )
			{
				if( !bAllPlayersDoneSelectingSteps )
				{
					m_bSongChosen[pn] = true;
					m_soundSelect.Play( false );
					Message msg( "StepsChosen" );
					msg.SetParam( "Player", pn );
					MESSAGEMAN->Broadcast( msg );
					return true;
				}
			}
			else if( m_bStepsChosen[pn] )
			{
				// Second press confirms; everyone else must have picked too.
				bool bAllOthersChosen = true;
				FOREACH_HumanPlayer( p )
				{
					if( p != pn )
						bAllOthersChosen &= m_bStepsChosen[p];
				}
				if( !bAllOthersChosen )
				{
					m_soundBad.Play( false );
					return false;
				}
			}
			else
			{
				m_bStepsChosen[pn] = true;
				m_soundSelect.Play( false );
				Message msg( "StepsChosen" );
				msg.SetParam( "Player", pn );
				MESSAGEMAN->Broadcast( msg );
				return true;
			}
		}
		break;
	}

	default:
		FAIL_M( ssprintf("Invalid SelectionState: %i", m_SelectionState) );
	}

	m_SelectionState = GetNextSelectionState();
	if( m_MenuTimer->GetSeconds() < 1.0f )
		m_SelectionState = SelectionState_Finalized;

	if( m_SelectionState != SelectionState_Finalized )
	{
		Message msg( "Start" + SelectionStateToString(m_SelectionState) );
		MESSAGEMAN->Broadcast( msg );
		m_MusicWheel.HandleMessageOver( msg );
		SCREENMAN->PlayStartSound();
		return true;
	}

	m_soundOut.PlayCopy( false );
	FOREACH_PlayerNumber( p )
	{
		m_OptionsList[p].SetVisible( false );
		m_ModList[p].SetVisible( false );
	}
	FOREACH_HumanPlayer( p )
	{
		m_bSongChosen[p] = true;
		m_bStepsChosen[p] = true;
		GAMESTATE->m_bPlayerChartIndex[p] = m_iSelection[p];
	}
	m_MenuTimer->Stop();

	// Now that Steps have been chosen, set a Style that can play them.
	const Style *pStyle = nullptr;
	if( GAMESTATE->IsCourseMode() )
		pStyle = GAMESTATE->m_pCurCourse->GetCourseStyle( GAMESTATE->m_pCurGame, GAMESTATE->GetNumSidesJoined() );
	if( pStyle == nullptr )
	{
		StepsType stCurrent;
		PlayerNumber pn = GAMESTATE->GetMasterPlayerNumber();
		if( GAMESTATE->IsCourseMode() )
		{
			ASSERT( GAMESTATE->m_pCurTrail[pn] != nullptr );
			stCurrent = GAMESTATE->m_pCurTrail[pn]->m_StepsType;
		}
		else
		{
			ASSERT( GAMESTATE->m_pCurSteps[pn] != nullptr );
			stCurrent = GAMESTATE->m_pCurSteps[pn]->m_StepsType;
		}
		pStyle = GAMEMAN->GetFirstCompatibleStyle( GAMESTATE->m_pCurGame, GAMESTATE->GetNumSidesJoined(), stCurrent );
	}
	GAMESTATE->SetCurrentStyle( pStyle );

	if( input.pn == PLAYER_INVALID )
		m_Out.StartTransitioning( SM_GoToNextScreen );
	else
		StartTransitioningScreen( SM_GoToNextScreen );

	PickRandomChannelSong();
	return true;
}

// Recreated from STEP_P1.exe (0x420382): the Random channel's entries are
// placeholders named after a level range; pick a real song and chart in it.
void ScreenSelectMusic::PickRandomChannelSong()
{
	if( GAMESTATE->m_sCurrentGroupName != "SO_RANDOM" )
		return;

	const RString &sTitle = GAMESTATE->m_pCurSong->m_sMainTitle;
	int iMin, iMax;
	if( sTitle == "S01_S04" || sTitle == "D01_D04" )		{ iMin = 1; iMax = 4; }
	else if( sTitle == "S05_S07" || sTitle == "D05_D07" )	{ iMin = 5; iMax = 7; }
	else if( sTitle == "S08_S10" || sTitle == "D08_D10" )	{ iMin = 8; iMax = 10; }
	else if( sTitle == "S11_S13" || sTitle == "D11_D13" )	{ iMin = 11; iMax = 13; }
	else if( sTitle == "S14_S16" || sTitle == "D14_D16" )	{ iMin = 14; iMax = 16; }
	else if( sTitle == "S17_S19" || sTitle == "D17_D19" )	{ iMin = 17; iMax = 19; }
	else if( sTitle == "S20_OVER" )							{ iMin = 20; iMax = 24; }
	else if( sTitle == "D20_D22" )							{ iMin = 20; iMax = 22; }
	else if( sTitle == "D23_OVER" )							{ iMin = 23; iMax = 24; }
	else return;

	int iStyle = -1;
	FOREACH_HumanPlayer( p )
	{
		const Steps *pSteps = GAMESTATE->m_pCurSteps[p];
		if( pSteps == nullptr )
			continue;
		if( pSteps->m_StepsType == StepsType_pump_single )
			iStyle = 0;
		else if( pSteps->m_StepsType == StepsType_pump_double )
			iStyle = 1;
	}
	if( iStyle == -1 )
		return;

	const vector<LevelSongList*> &vLevels = SONGMAN->GetLevelSections();
	vector<LevelSongList*> vCandidates;
	for( int i = iMin; i <= iMax; ++i )
	{
		LevelSongList *pList = vLevels[i-1];
		if( !(*pList)[iStyle].empty() )
			vCandidates.push_back( pList );
	}
	if( vCandidates.empty() )
		return;

	LevelSongList *pLevel = vCandidates[ rand() % vCandidates.size() ];
	vector<Song*> vSongs = (*pLevel)[iStyle];
	Song *pSong = vSongs[ rand() % vSongs.size() ];
	StepsType st = iStyle == 0 ? StepsType_pump_single : StepsType_pump_double;

	Steps *pChosen = nullptr;
	vector<Steps*> vOver;
	const vector<Steps*> &vSteps = pSong->GetAllSteps();
	for( unsigned i = 0; i < vSteps.size(); ++i )
	{
		Steps *pSteps = vSteps[i];
		if( pSteps->m_StepsType != st )
			continue;
		if( pLevel->m_iLevel == 24 && pSteps->GetMeter() >= 24 )
		{
			vOver.push_back( pSteps );
			continue;
		}
		if( pSteps->GetMeter() == pLevel->m_iLevel )
		{
			pChosen = pSteps;
			break;
		}
	}
	if( pLevel->m_iLevel == 24 )
	{
		if( vOver.empty() )
			return;
		pChosen = vOver[ rand() % vOver.size() ];
	}

	if( pSong != nullptr && pChosen != nullptr )
	{
		GAMESTATE->m_pCurSong.SetWithoutBroadcast( pSong );
		FOREACH_HumanPlayer( p )
			GAMESTATE->m_pCurSteps[p].SetWithoutBroadcast( pChosen );
	}
}

bool ScreenSelectMusic::MenuBack( const InputEventPlus & /* input */ )
{
	FOREACH_HumanPlayer( p )
		GAMESTATE->SaveCurrentSettingsToProfile( p );
	Cancel( SM_GoToPrevScreen );
	return true;
}

void ScreenSelectMusic::HandleMessage( const Message &msg )
{
	if( m_bShouldAllowLateJoin && msg.GetName() == MessageIDToString(Message_PlayerJoined) )
	{
		PlayerNumber master = GAMESTATE->GetMasterPlayerNumber();
		FOREACH_PlayerNumber( p )
			GAMESTATE->m_pCurSteps[p].SetWithoutBroadcast( nullptr );

		if( GAMESTATE->IsBasicMode() )
		{
			m_MusicWheel.ReleaseWheelItems();
			m_MusicWheel.OpenGroupOrChangeSort( GAMESTATE->m_sCurrentGroupName );
			AfterMusicChange();
		}
		else
		{
			// A late joiner sends everyone back to the play mode menu.
			SOUND->StopMusic();
			m_MenuTimer->Stop();
			GAMESTATE->m_sCurrentGroupName = "";
			m_fLockInputSecs = 2.0f;
			m_bGoToPlayModeBranch = true;
			g_PlayModeBranchTimer.Touch();
		}

		PlayerNumber pn = PLAYER_INVALID;
		bool b = msg.GetParam( "Player", pn );
		ASSERT( b );
		m_ModList[pn].Load( pn, false );
		m_iSelection[pn] = 0;
		m_iSelection[master] = 0;
		if( GAMESTATE->IsCourseMode() )
		{
			GAMESTATE->m_pCurSteps[pn].Set( m_vpSteps.empty() ? nullptr : m_vpSteps[0] );
			MESSAGEMAN->Broadcast( "PlayableStepsChanged" );
		}
		if( GAMESTATE->IsBasicMode() )
		{
			FOREACH_PlayerNumber( p )
			{
				if( m_bStepsChosen[p] )
				{
					Message m( "StepsPreselectedCancelled" );
					m.SetParam( "Player", p );
					MESSAGEMAN->Broadcast( m );
					m_bStepsChosen[p] = false;
				}
			}
		}
	}
	else if( msg.GetName() == MessageIDToString(Message_StepTypesToShowChanged) )
	{
		GAMESTATE->m_sCurrentGroupName = m_sCurChannel;
		m_MusicWheel.OpenGroupOrChangeSort( m_sCurChannel );
		Song *pSong = m_MusicWheel.GetSelectedSong();
		if( pSong != nullptr )
		{
			m_CurCategory = pSong->m_SongCategory;
			Message m( "CurrentCategoryChanged" );
			if( m_CurCategory == SONGCATEGORY_USE_GENRE )
				m.SetParam( "Category", pSong->m_sGenre );
			else
				m.SetParam( "Category", SongCategoryToString(m_CurCategory) );
			MESSAGEMAN->Broadcast( m );
		}
		AfterMusicChange();
	}
	else if( msg.GetName() == "Code" )
	{
		RString sCode;
		PlayerNumber pn = PLAYER_INVALID;
		bool b_name = msg.GetParam( "Name", sCode );
		bool b_pn = msg.GetParam( "PlayerNumber", pn );
		ASSERT( b_name );
		ASSERT( b_pn );

		if( !GAMESTATE->IsBasicMode() )
		{
			if( sCode == "OptionList" )
				OpenOptionsList( pn );
			else if( sCode == "RushCode" )
			{
				SO_GROUP_ASSIGN( GAMESTATE->m_SongOptions, ModsLevel_Preferred, m_fMusicRate, 5.0f );
				m_soundCheat.Play( false );
			}
		}

		if( sCode == "SpeedUp" || sCode == "SpeedDown" )
		{
			float fMax = GAMESTATE->IsBasicMode() ? 4.0f : 6.0f;
			PlayerOptions &po = GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions.GetPreferred();
			float fCur = po.m_fScrollSpeed;
			float fStep = floorf(fCur) == fCur ? 1.0f : 0.5f;
			float fNew;
			if( sCode == "SpeedUp" )
			{
				fNew = fCur + fStep;
				if( fNew > fMax )
					fNew = 1.0f;
			}
			else
			{
				fNew = fCur - fStep;
				if( fNew <= 0 )
					fNew = fMax;
			}
			fNew = floorf( fNew );
			PO_GROUP_ASSIGN( GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions, ModsLevel_Preferred, m_fScrollSpeed, fNew );
			m_ModList[pn].Set( StepP1Mods::CAT_SPEED, true, (int)fNew - 1 );
			MenuStart( InputEventPlus() );
			m_soundCommandSet.Play( false );
		}
		else if( sCode == "WX_ROULETTE" )
		{
			if( m_SelectionState == SelectionState_SelectingSong )
				m_MusicWheel.StartRoulette();
			m_soundBad.Play( false );
		}

		// PIU codes map onto the OptionList (category, item) pairs.
		if( PREFSMAN->m_bEnablePIUCodes )
		{
			struct CodeMod { const char *szCode; int iCategory; int iItem; };
			static const CodeMod codes[] = {
				{ "SX_1X", StepP1Mods::CAT_SPEED, 0 }, { "SX_2X", StepP1Mods::CAT_SPEED, 1 }, { "SX_3X", StepP1Mods::CAT_SPEED, 2 },
				{ "SX_4X", StepP1Mods::CAT_SPEED, 3 }, { "SX_5X", StepP1Mods::CAT_SPEED, 4 }, { "SX_6X", StepP1Mods::CAT_SPEED, 5 },
				{ "SX_025X", StepP1Mods::CAT_SPEED, 6 }, { "SX_050X", StepP1Mods::CAT_SPEED, 7 }, { "SX_EW", StepP1Mods::CAT_SPEED, 8 },
				{ "SX_AV", StepP1Mods::CAT_SPEED, 10 }, { "SX_AC", StepP1Mods::CAT_SPEED, 11 }, { "SX_DC", StepP1Mods::CAT_SPEED, 12 },
				{ "RX_80", StepP1Mods::CAT_RATE, 0 }, { "RX_90", StepP1Mods::CAT_RATE, 1 }, { "RX_110", StepP1Mods::CAT_RATE, 2 },
				{ "RX_120", StepP1Mods::CAT_RATE, 3 }, { "RX_130", StepP1Mods::CAT_RATE, 4 }, { "RX_140", StepP1Mods::CAT_RATE, 5 },
				{ "RX_150", StepP1Mods::CAT_RATE, 6 },
				{ "JX_EJ", StepP1Mods::CAT_JUDGE, 0 }, { "JX_HJ", StepP1Mods::CAT_JUDGE, 1 }, { "JX_VJ", StepP1Mods::CAT_JUDGE, 2 },
				{ "JX_XJ", StepP1Mods::CAT_JUDGE, 3 }, { "JX_UJ", StepP1Mods::CAT_JUDGE, 4 }, { "JX_JR", StepP1Mods::CAT_JUDGE, 5 },
				{ "JX_JN", StepP1Mods::CAT_JUDGE, 6 }, { "JX_FP", StepP1Mods::CAT_JUDGE, 7 },
				{ "AX_M", StepP1Mods::CAT_ALTERNATE, 0 }, { "AX_RS", StepP1Mods::CAT_ALTERNATE, 1 },
				{ "PX_XM", StepP1Mods::CAT_PATH, 0 }, { "PX_NM", StepP1Mods::CAT_PATH, 1 }, { "PX_UA", StepP1Mods::CAT_PATH, 2 },
				{ "PX_DR", StepP1Mods::CAT_PATH, 3 }, { "PX_SI", StepP1Mods::CAT_PATH, 4 }, { "PX_RI", StepP1Mods::CAT_PATH, 5 },
				{ "PX_SN", StepP1Mods::CAT_PATH, 6 },
				{ "DX_AP", StepP1Mods::CAT_DISPLAY, 1 }, { "DX_FD", StepP1Mods::CAT_DISPLAY, 3 }, { "DX_FL", StepP1Mods::CAT_DISPLAY, 4 },
				{ "DX_RS", StepP1Mods::CAT_DISPLAY, 5 }, { "DX_AT", StepP1Mods::CAT_DISPLAY, 6 }, { "DX_RL", StepP1Mods::CAT_DISPLAY, 7 },
				{ "DX_BGO", StepP1Mods::CAT_DISPLAY, 8 },
			};
			bool bOn = true, bSpecial = false;
			if( BeginsWith(sCode, "HX_") )
			{
				// Noteskin by name; skins outside the OptionList table are set directly.
				RString sNoteSkin = sCode.substr( 3 );
				if( NOTESKIN->DoesNoteSkinExist(sNoteSkin) )
				{
					const vector<StepP1Mods::NoteSkinEntry> &v = StepP1Mods::GetNoteSkins();
					int iFound = -1;
					for( unsigned i = 0; i < v.size(); ++i )
						if( v[i].sName == sNoteSkin )
							iFound = i;
					if( iFound != -1 )
					{
						StepP1Mods::ApplyToPlayer( pn, StepP1Mods::CAT_NOTESKIN, iFound, bOn, bSpecial );
					}
					else
					{
						ModsGroup<PlayerOptions> &po = GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions;
						PO_GROUP_ASSIGN( po, ModsLevel_Preferred, m_iNoteSkinIndex, -1 );
						PO_GROUP_ASSIGN( po, ModsLevel_Preferred, m_sNoteSkin, sNoteSkin );
						PO_GROUP_ASSIGN( po, ModsLevel_Preferred, m_bRandomSkin, false );
						PO_GROUP_ASSIGN( po, ModsLevel_Preferred, m_bAutoNoteskin, false );
						PO_GROUP_ASSIGN( po, ModsLevel_Preferred, m_bRouletteNoteskin, false );
					}
					m_ModList[pn].Set( StepP1Mods::CAT_DISPLAY, true, 0 );
					m_ModList[pn].Set( StepP1Mods::CAT_NOTESKIN, true, 0 );
					m_soundCheat.Play( false );
				}
			}
			else if( sCode == "DX_V_NS" )
			{
				// Toggles hidden, or turns off whichever of hidden/sudden is on.
				const PlayerOptions &po = GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions.GetPreferred();
				int iItem = 0;
				if( !StepP1Mods::IsActive(po, StepP1Mods::CAT_DISPLAY, 0) && StepP1Mods::IsActive(po, StepP1Mods::CAT_DISPLAY, 1) )
					iItem = 1;
				StepP1Mods::ApplyToPlayer( pn, StepP1Mods::CAT_DISPLAY, iItem, bOn, bSpecial );
				m_ModList[pn].Set( StepP1Mods::CAT_DISPLAY, true, iItem );
				m_soundCommandSet.Play( false );
			}
			else if( sCode == "RESET" )
			{
				m_OptionsList[pn].Reset();
			}
			else
			{
				for( unsigned i = 0; i < ARRAYLEN(codes); ++i )
				{
					if( sCode != codes[i].szCode )
						continue;
					StepP1Mods::ApplyToPlayer( pn, codes[i].iCategory, codes[i].iItem, bOn, bSpecial );
					m_ModList[pn].Set( codes[i].iCategory, true, codes[i].iItem );
					m_soundCommandSet.Play( false );
					break;
				}
			}
		}
	}

	ScreenWithMenuElements::HandleMessage( msg );
}

void ScreenSelectMusic::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_MenuTimer )
	{
		if( m_SelectionState != SelectionState_Finalized )
		{
			if( m_SelectionState == SelectionState_SelectingSteps )
				TweenOffScreen();
			else
				this->PlayCommand( "TimerOut" + SelectionStateToString(m_SelectionState) );
			m_SelectionState = SelectionState_SelectingSteps;
			MenuStart( InputEventPlus() );
		}
		return;
	}
	else if( SM == SM_GoToPrevScreen )
	{
		ClearMessageQueue();
		SOUND->StopMusic();
	}
	else if( SM == SM_BeginFadingOut )
	{
		this->PostScreenMessage( SM_GoToNextScreen, this->GetTweenTimeLeft() );
	}
	else if( SM == SM_GoToNextScreen )
	{
		SOUND->StopMusic();
	}
	else if( SM == SM_SongChanged )
	{
		AfterMusicChange();
	}
	else if( SM == SM_SortOrderChanging )
	{
		this->PlayCommand( "SortChange" );
	}
	else if( SM == SM_GainFocus )
	{
		CodeDetector::RefreshCacheItems( CODES );
	}
	else if( SM == SM_LoseFocus )
	{
		CodeDetector::RefreshCacheItems();
	}

	ScreenWithMenuElements::HandleScreenMessage( SM );
}

// lua start
#include "LuaBinding.h"

/** @brief Allow Lua to have access to the ScreenSelectMusic. */
class LunaScreenSelectMusic : public Luna<ScreenSelectMusic>
{
public:
	static int GetWheelCurrentIndex( T* p, lua_State *L )	{ lua_pushnumber( L, p->GetWheelCurrentIndex() ); return 1; }
	static int GetWheelNumItems( T* p, lua_State *L )		{ lua_pushnumber( L, (unsigned)p->GetWheelNumItems() ); return 1; }
	static int GetSongGroupNamesAvailables( T* p, lua_State *L )
	{
		LuaHelpers::CreateTableFromArray( p->GetChannels(), L );
		return 1;
	}
	static int IsPlayerReady( T* p, lua_State *L )
	{
		PlayerNumber pn = Enum::Check<PlayerNumber>( L, 1 );
		lua_pushboolean( L, p->IsPlayerReady(pn) );
		return 1;
	}
	static int GetCurrentGroup( T* p, lua_State *L )		{ lua_pushstring( L, p->GetCurrentGroup() ); return 1; }
	static int GetPlayableSteps( T* p, lua_State *L )
	{
		const vector<Steps*> &v = p->GetPlayableSteps();
		lua_newtable( L );
		for( unsigned i = 0; i < v.size(); ++i )
		{
			v[i]->PushSelf( L );
			lua_rawseti( L, -2, i+1 );
		}
		return 1;
	}
	static int GetPlayableTrails( T* p, lua_State *L )
	{
		const vector<Trail*> &v = p->GetPlayableTrails();
		lua_newtable( L );
		for( unsigned i = 0; i < v.size(); ++i )
		{
			v[i]->PushSelf( L );
			lua_rawseti( L, -2, i+1 );
		}
		return 1;
	}
	static int GetSelection( T* p, lua_State *L )
	{
		PlayerNumber pn = Enum::Check<PlayerNumber>( L, 1 );
		lua_pushnumber( L, p->GetSelection(pn) );
		return 1;
	}
	static int GetCurrentGroupIndex( T* p, lua_State *L )	{ lua_pushnumber( L, p->GetCurrentGroupIndex() + 1 ); return 1; }

	LunaScreenSelectMusic()
	{
		ADD_METHOD( GetWheelCurrentIndex );
		ADD_METHOD( GetWheelNumItems );
		ADD_METHOD( GetSongGroupNamesAvailables );
		ADD_METHOD( IsPlayerReady );
		ADD_METHOD( GetCurrentGroup );
		ADD_METHOD( GetPlayableSteps );
		ADD_METHOD( GetPlayableTrails );
		ADD_METHOD( GetSelection );
		ADD_METHOD( GetCurrentGroupIndex );
	}
};

LUA_REGISTER_DERIVED_CLASS( ScreenSelectMusic, ScreenWithMenuElements )
// lua end

/*
 * (c) 2001-2004 Chris Danford
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
