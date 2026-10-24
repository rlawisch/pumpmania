/* ScreenSelectPlayModeBranch - Profile selection shown again after the play mode menu (STEP_P1). */

#ifndef SCREEN_SELECT_PLAY_MODE_BRANCH_H
#define SCREEN_SELECT_PLAY_MODE_BRANCH_H

#include "ScreenSelectProfile.h"

class ScreenSelectPlayModeBranch : public ScreenSelectProfile
{
public:
	virtual void Init();
	virtual bool Input( const InputEventPlus &input );
	virtual void HandleScreenMessage( const ScreenMessage SM );

	bool Finish();

protected:
	bool m_bAlreadyJoined[NUM_PLAYERS];	// player already chose a profile on ScreenSelectProfile
	bool m_bWaitingForFirstStart;
};

#endif
