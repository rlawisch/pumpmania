#ifndef PLAYER_H
#define PLAYER_H

#include "ActorFrame.h"
#include "HoldJudgment.h"
#include "NoteDataWithScoring.h"
#include "RageSound.h"
#include "AttackDisplay.h"
#include "NoteData.h"
#include "ScreenMessage.h"
#include "ThemeMetric.h"
#include "InputEventPlus.h"
#include "TimingData.h"

class ScoreDisplay;
class LifeMeter;
class CombinedLifeMeter;
class ScoreKeeper;
class Inventory;
class RageTimer;
class NoteField;
class PlayerStageStats;
// class JudgedRows; // xMAx

// todo: replace these with a Message and MESSAGEMAN? -aj
/*
AutoScreenMessage( SM_100Combo );
AutoScreenMessage( SM_200Combo );
AutoScreenMessage( SM_300Combo );
AutoScreenMessage( SM_400Combo );
AutoScreenMessage( SM_500Combo );
AutoScreenMessage( SM_600Combo );
AutoScreenMessage( SM_700Combo );
AutoScreenMessage( SM_800Combo );
AutoScreenMessage( SM_900Combo );
AutoScreenMessage( SM_1000Combo );
AutoScreenMessage( SM_ComboStopped );
AutoScreenMessage( SM_ComboContinuing );
*/ // xMAx - Removed
AutoScreenMessage( SM_Player1HitMine );
AutoScreenMessage( SM_Player2HitMine );

/** @brief Accepts input, knocks down TapNotes that were stepped on, and keeps score for the player. */
class Player: public ActorFrame
{
public:
	// The passed in NoteData isn't touched until Load() is called.
	Player( NoteData &nd, bool bVisibleParts = true );
	~Player();

	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();
	// PushPlayerMatrix and PopPlayerMatrix are separate functions because
	// they need to be used twice so that the notefield board can rendered
	// underneath the combo and judgment.  They're not embedded in
	// PlayerMatrixPusher so that some nutjob can later decide to expose them
	// to lua. -Kyz
	void PushPlayerMatrix(float x, float skew, float center_y);
	void PopPlayerMatrix();

	// This exists so that the board can be drawn underneath combo/judge. -Kyz
	void DrawNoteFieldBoard();

	// Here's a fun construct for people that haven't seen it before:
	// This object does some task when it's created, then cleans up when it's
	// destroyed.  That way, you stick it inside a block, and can't forget the
	// cleanup. -Kyz
	struct PlayerNoteFieldPositioner
	{
		PlayerNoteFieldPositioner(Player* p, float x, float tilt, float skew, float mini, float center_y, bool reverse);
		~PlayerNoteFieldPositioner();
		Player* player;
		float original_y;
		float y_offset;
	};

	struct TrackRowTapNote
	{
		int iTrack;
		int iRow;
		TapNote *pTN;

    // xMAx ---------------------------------------------------------------------------------------
		bool operator==( const TrackRowTapNote &other ) const
		{
			#define COMPARE(x)	if(x!=other.x) return false
			COMPARE(iTrack);
			COMPARE(iRow);
			COMPARE(pTN);
			#undef COMPARE
			return true;
		}
    // --------------------------------------------------------------------------------------------
	};
	// void UpdateHoldNotes( int iSongRow, float fDeltaTime, vector<TrackRowTapNote> &vTN ); xMAx
	void UpdateHoldNote( int iSongRow, float fDeltaTime, TrackRowTapNote &trtn ); // xMAx

	void Init(
		const RString &sType,
		PlayerState* pPlayerState,
		PlayerStageStats* pPlayerStageStats,
		LifeMeter* pLM,
		CombinedLifeMeter* pCombinedLM,
		ScoreDisplay* pScoreDisplay,
		ScoreDisplay* pSecondaryScoreDisplay,
		Inventory* pInventory,
		ScoreKeeper* pPrimaryScoreKeeper,
		ScoreKeeper* pSecondaryScoreKeeper );
	void Load();
	void CrossedRows( int iLastRowCrossed, const RageTimer &now );
	void CrossedHoldsRows( int iLastRowCrossed, const RageTimer &now, float fDeltaTime ); // xMAx
	bool IsOniDead() const;

	/**
	 * @brief Retrieve the Player's TimingData.
	 *
	 * This is primarily for a lua hook.
	 * @return the TimingData in question. */
	TimingData GetPlayerTimingData() const
	{
		return *(this->m_Timing);
	}

  /* StepP1 Revival - Thequila
	void ScoreAllActiveHoldsLetGo();
	void DoTapScoreNone();
  */

	void Step( int col, int row, const RageTimer &tm, bool bRelease ); // xMAx - removed bHeld

	void FadeToFail();
	void CacheAllUsedNoteSkins();
	// TapNoteScore GetLastTapNoteScore() const { return m_LastTapNoteScore; } // xMAx
	void ApplyWaitingTransforms();
	void SetPaused( bool bPaused ) { m_bPaused = bPaused; }

  // xMAx - Removed static
	float GetMaxStepDistanceSeconds();
	float GetWindowSeconds( TimingWindow tw );

	const NoteData &GetNoteData() const { return m_NoteData; }
	bool HasVisibleParts() const { return false; } // xMAx

	void SetActorWithJudgmentPosition( Actor *pActor ) { m_pActorWithJudgmentPosition = pActor; }
	void SetActorWithComboPosition( Actor *pActor ) { m_pActorWithComboPosition = pActor; }

	void SetSendJudgmentAndComboMessages( bool b ) { m_bSendJudgmentAndComboMessages = b; }

  /* StepP1 Revival - Thequila
	// OITG bug:  Actor::SetZoom only sets X and Y.  When mini is applied to
	// the notefield with SetZoom, it does not affect the range of bumpy.
	// m_oitg_zoom_mode provides compatibility with that bug.  Only used in
	// defective mode. -Kyz
	bool m_oitg_zoom_mode;
  */

	// Lua
	// virtual void PushSelf( lua_State *L ); // xMAx

	PlayerState * GetPlayerState() { return this->m_pPlayerState; }
	void ChangeLife(float delta);
	void SetLife(float value);
	bool m_inside_lua_set_life;

	// xMAx ------------------------------------------------------
	/*
	JUDGE  PERFECT  DELAY  INTERVAL
	EJ     7        5      5
	NJ     5        5      5
	HJ     3        5      5
	VJ     2        5      4
	XJ     1        4      2
	UJ     0        3      2

	number/120.f*1000
	*/
	struct JudgeData
	{
		int iPerfect;
		int iDelay;
		int iDelta;
		JudgeData &operator=( const JudgeData &judgeData );
	};

	float HOLD_TIMING;
	float PERF_U, PERF_D;
	float GREAT_U, GREAT_D;
	float GOOD_U, GOOD_D;
	float BAD_U, BAD_D;

	bool m_bCountNotesSeparately;

	/** xMAx: use this when using Record mode in the steps editor */
	/** Else, the game will send Combo/Judging messages when the "Player" were updated in the Update() function in the Editor */
	/** That will happen only after one Player load (which happens when play in the editor) */
	inline void Unload() { m_bLoaded = false; };

	//TapNoteScore Player::EvaluateRowAsGroup(int iRow);
	// ------------------------------------------------------------

protected:
	void UpdateTapNotesMissedOlderThan( float fMissIfOlderThanThisBeat );
	// void UpdateJudgedRows(); // xMAx
	// void FlashGhostRow( int iRow );
	void FlashGhostRow( int iRow, int iPlayerNoteSkin ); // xMAx - added iPlayerNoteSkin
	void HandleTapRowScore( unsigned row, TapNoteScore tns ); // xMAx - added tns
	// void HandleHoldScore( const TapNote &tn ); // xMAx
	void HandleHoldCheckpoint( int iRow, int iNumHoldsHeldThisRow, int iNumHoldsMissedThisRow, const vector<int> &viColsWithHold, bool bHoldsAreBeingPressed ); // xMAx - added bHoldsAreBeingPressed
	// void DrawTapJudgments(); // xMAx
	// void DrawHoldJudgments(); // xMAx
	// void SendComboMessages( unsigned int iOldCombo, unsigned int iOldMissCombo ); // xMAx
	void PlayKeysound( const TapNote &tn, TapNoteScore score );

	// void SetMineJudgment( TapNoteScore tns , int iTrack ); // xMAx
	// void SetJudgment( int iRow, int iFirstTrack, const TapNote &tn ) { SetJudgment( iRow, iFirstTrack, tn, tn.result.tns, tn.result.fTapNoteOffset ); } // xMAx
	// void SetJudgment( int iRow, int iFirstTrack, const TapNote &tn, TapNoteScore tns, float fTapNoteOffset );	// -1 if no track as in TNS_Miss // xMAx
	void SetJudgment( TapNoteScore tns );	// xMAx
	// void SetHoldJudgment( TapNote &tn, int iTrack ); // xMAx
	void SetCombo( unsigned int iCombo, unsigned int iMisses );
	void IncrementComboOrMissCombo( bool bComboOrMissCombo );
	void IncrementCombo() { IncrementComboOrMissCombo(true); };
	void IncrementMissCombo() { IncrementComboOrMissCombo(false); };

	void ChangeLife( TapNoteScore tns );
	// void ChangeLife( HoldNoteScore hns, TapNoteScore tns ); // xMAx
	// void ChangeLifeRecord(); // xMAx

	int GetClosestNoteDirectional( int col, int iStartRow, int iMaxRowsAhead, bool bAllowGraded, bool bForward, bool bAllowHoldHead ) const; // xMAx
	int GetClosestNote( int col, int iNoteRow, int iMaxRowsAhead, int iMaxRowsBehind, bool bAllowGraded, bool bAllowHoldHead = true ) const; // xMAx
	// int GetClosestNonEmptyRowDirectional( int iStartRow, int iMaxRowsAhead, bool bAllowGraded, bool bForward ) const; // xMAx
	// int GetClosestNonEmptyRow( int iNoteRow, int iMaxRowsAhead, int iMaxRowsBehind, bool bAllowGraded ) const; // xMAx

	RString ApplyRandomAttack();

	inline void HideNote( int col, int row )
	{
		NoteData::iterator iter = m_NoteData.FindTapNote( col, row );
		if( iter != m_NoteData.end(col) )
			iter->second.result.bHidden = true;
	}

	bool			m_bLoaded;

	/** @brief The player's present state. */
	PlayerState		*m_pPlayerState;
	/** @brief The player's present stage stats. */
	PlayerStageStats	*m_pPlayerStageStats;
	TimingData      *m_Timing;
	// float			m_fNoteFieldHeight; // xMAx

	bool			m_bPaused;
	bool			m_bDelay;

	NoteData		&m_NoteData;
	NoteField		*m_pNoteField;

	// vector<HoldJudgment*>	m_vpHoldJudgment; // xMAx

	// AutoActor		m_sprJudgment; // xMAx
	// AutoActor		m_sprCombo; // xMAx
	Actor			*m_pActorWithJudgmentPosition;
	Actor			*m_pActorWithComboPosition;

	// AttackDisplay		*m_pAttackDisplay; // xMAx

	// TapNoteScore		m_LastTapNoteScore; // xMAx
	LifeMeter		*m_pLifeMeter;
	CombinedLifeMeter	*m_pCombinedLifeMeter;
	ScoreDisplay		*m_pScoreDisplay;
	ScoreDisplay		*m_pSecondaryScoreDisplay;
	ScoreKeeper		*m_pPrimaryScoreKeeper;
	ScoreKeeper		*m_pSecondaryScoreKeeper;
	Inventory		*m_pInventory;

	int			m_iFirstUncrossedRow;	// used by hold checkpoints logic
	NoteData::all_tracks_iterator *m_pIterNeedsTapJudging;
	NoteData::all_tracks_iterator *m_pIterNeedsHoldJudging;
	NoteData::all_tracks_iterator *m_pIterUncrossedRows;
	// NoteData::all_tracks_iterator *m_pIterUnjudgedRows; // xMAx
	// NoteData::all_tracks_iterator *m_pIterUnjudgedMineRows; // xMAx
	// unsigned int	m_iLastSeenCombo; // xMAx
	// bool	m_bSeenComboYet; // StepP1 Revival - Thequila
	// JudgedRows		*m_pJudgedRows; // xMAx

	RageSound		m_soundMine;
	// RageSound		m_soundAttackLaunch; // xMAx
	// RageSound		m_soundAttackEnding; // xMAx

	float			m_fActiveRandomAttackStart;

	// vector<bool>	m_vbFretIsDown; // xMAx

	vector<RageSound>	m_vKeysounds;

  // xMAx
	// ThemeMetric<float>	GRAY_ARROWS_Y_STANDARD;
	// ThemeMetric<float>	GRAY_ARROWS_Y_REVERSE;
	// ThemeMetric2D<float>	ATTACK_DISPLAY_X;
	// ThemeMetric<float>	ATTACK_DISPLAY_Y;
	// ThemeMetric<float>	ATTACK_DISPLAY_Y_REVERSE;
	// ThemeMetric<float>	HOLD_JUDGMENT_Y_STANDARD;
	// ThemeMetric<float>	HOLD_JUDGMENT_Y_REVERSE;
	// ThemeMetric<int>	BRIGHT_GHOST_COMBO_THRESHOLD;
	// ThemeMetric<bool>	TAP_JUDGMENTS_UNDER_FIELD;
	// ThemeMetric<bool>	HOLD_JUDGMENTS_UNDER_FIELD;
	// ThemeMetric<bool>	COMBO_UNDER_FIELD;

	ThemeMetric<int>	DRAW_DISTANCE_AFTER_TARGET_PIXELS;
	ThemeMetric<int>	DRAW_DISTANCE_BEFORE_TARGET_PIXELS;

	// StepP1 Revival - bSilver
	int iCombo = 0;
	int iMisses = 0;

#define NUM_REVERSE 2
#define NUM_CENTERED 2
	TweenState		m_tsJudgment[NUM_REVERSE][NUM_CENTERED];
	TweenState		m_tsCombo[NUM_REVERSE][NUM_CENTERED];

	bool m_bSendJudgmentAndComboMessages;
	bool m_bTickHolds;
	// This exists so that the board can be drawn underneath combo/judge. -Kyz
	bool m_drawing_notefield_board;

private:
	TapNoteScore JudgeRow(int row);
};

class PlayerPlus
{
	Player *m_pPlayer;
	NoteData m_NoteData;
public:
	PlayerPlus() { m_pPlayer = new Player(m_NoteData); }
	~PlayerPlus() { delete m_pPlayer; }
	void Load( const NoteData &nd ) { m_NoteData = nd; m_pPlayer->Load(); }
	Player *operator->() { return m_pPlayer; }
	const Player *operator->() const { return m_pPlayer; }
	operator Player*() { return m_pPlayer; }
	operator const Player*() const { return m_pPlayer; }
};

#endif

/*
 * (c) 2001-2006 Chris Danford, Steve Checkoway
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
