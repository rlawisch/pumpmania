/* ScreenSelectMusic - Choose a Song and Steps. */

#ifndef SCREEN_SELECT_MUSIC_H
#define SCREEN_SELECT_MUSIC_H

#include "ScreenWithMenuElements.h"
#include "GameConstantsAndTypes.h"
#include "MusicWheel.h"
#include "Banner.h"
#include "ThemeMetric.h"
#include "RageTexturePreloader.h"
#include "GameInput.h"
#include "OptionsList.h"
#include "ModIconsList.h"
#include "Song.h"

// Recreated from STEP_P1.exe: the screen goes channel (group) -> song -> steps -> finalized.
enum SelectionState
{
	SelectionState_SelectingGroup,
	SelectionState_SelectingSong,
	SelectionState_SelectingSteps,
	SelectionState_Finalized,
	NUM_SelectionState,
};
const RString& SelectionStateToString( SelectionState ss );

class ScreenSelectMusic : public ScreenWithMenuElements
{
public:
	ScreenSelectMusic();
	virtual ~ScreenSelectMusic();
	virtual void Init();
	virtual void BeginScreen();

	virtual void Update( float fDeltaTime );
	virtual bool Input( const InputEventPlus &input );
	virtual void HandleMessage( const Message &msg );
	virtual void HandleScreenMessage( const ScreenMessage SM );
	virtual bool AllowLateJoin() const { return true; }

	virtual bool MenuStart( const InputEventPlus &input );
	virtual bool MenuBack( const InputEventPlus &input );

	MusicWheel *GetMusicWheel() { return &m_MusicWheel; }

	virtual RString NextChannel();
	virtual RString PrevChannel();

	virtual void PushSelf( lua_State *L );

	// Lua
	int GetWheelCurrentIndex() { return m_MusicWheel.GetCurrentIndex(); }
	int GetWheelNumItems() { return m_MusicWheel.GetNumItems(); }
	const vector<RString> &GetChannels() const { return m_vChannels; }
	bool IsPlayerReady( PlayerNumber pn ) const { return m_bStepsChosen[pn]; }
	RString GetCurrentGroup() const { return m_vChannels[m_iCurChannel]; }
	const vector<Steps*> &GetPlayableSteps() const { return m_vpSteps; }
	const vector<Trail*> &GetPlayableTrails() const { return m_vpTrails; }
	int GetSelection( PlayerNumber pn ) const { return m_iSelection[pn]; }
	int GetCurrentGroupIndex() const { return m_iCurChannel; }

protected:
	void ChangeSteps( PlayerNumber pn, int dir );
	void AfterStepsOrTrailChange( const vector<PlayerNumber> &vpns );
	void AfterMusicChange();
	void UpdateCategory();
	void UpdatePreview( bool bForce );
	void OpenOptionsList( PlayerNumber pn );
	bool LeaveBasicMode( const InputEventPlus &input );
	void PickRandomChannelSong();
	void ChangeChannel( int iDir );
	SelectionState GetNextSelectionState() const;
	SelectionState GetPrevSelectionState( bool &bValid ) const;

	ThemeMetric<RString>	CODES;

	RString			m_sCurChannel;
	vector<RString>	m_vChannels;
	int				m_iCurChannel;
	bool			m_bStepsChosen[NUM_PLAYERS];	// first Start press on a chart
	Banner			m_Preview;
	vector<Steps*>	m_vpSteps;
	vector<Trail*>	m_vpTrails;
	int				m_iSelection[NUM_PLAYERS];
	MusicWheel		m_MusicWheel;
	OptionsList		m_OptionsList[NUM_PLAYERS];
	ModIconsList	m_ModList[NUM_PLAYERS];
	bool			m_bAllowOptionsList[NUM_PLAYERS];

	SelectionState	m_SelectionState;
	RString			m_sSampleMusicToPlay;
	float			m_fSampleStartSeconds;
	float			m_fSampleLengthSeconds;
	int				m_iSampleVolume;
	bool			m_bSongChosen[NUM_PLAYERS];
	bool			m_bQuestMode;	// SO_QUEST / 04-SKILLUP ZONE channel
	bool			m_bBattleMode;
	bool			m_bGoToPlayModeBranch;

	RageSound		m_soundOut;			// Sounds/OUT
	RageSound		m_soundBack;		// Sounds/S_BACK
	RageSound		m_soundSongMove;	// Sounds/MOVE
	RageSound		m_soundUnlock;		// Sounds/N_TO_M
	RageSound		m_soundJoin;		// Sounds/JOIN
	RageSound		m_soundCategory[NUM_SongCategory];	// Categories/<name>
	RageSound		m_soundCommandSet;	// CW/S_CMD_SET
	RageSound		m_soundCheat;		// Sounds/CHEAT
	RageSound		m_soundGroupMove;	// Sounds/MOVE
	RageSound		m_soundSelect;		// Sounds/SELECT
	RageSound		m_soundStepsMove;	// Sounds/D_MOVE
	RageSound		m_soundBad;			// Sounds/BAD

	SongCategory	m_CurCategory;
	RString			m_sPreviewPath;
	RString			m_sCategoryName;
	int				m_iGroupScrollDir;
	int				m_iGroupScrollSpeed;
	bool			m_bGroupFastScroll;
	bool			m_bGroupHeld;
	bool			m_bPreviewPending;
	bool			m_bBackButtonsHeld;
	bool			m_bBackSoundPlayed;

	GameButton		m_GameButtonPreviousSong;
	GameButton		m_GameButtonNextSong;
	GameButton		m_GameButtonPrevState1;
	GameButton		m_GameButtonPrevState2;
	RageTexturePreloader	m_TexturePreload;
};

#endif

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
