/* ScreenSelectPlayMode - Choose between Arcade and Battle (STEP_P1). */

#ifndef SCREEN_SELECT_PLAY_MODE_H
#define SCREEN_SELECT_PLAY_MODE_H

#include "ScreenWithMenuElements.h"

class ScreenSelectPlayMode : public ScreenWithMenuElements
{
public:
	virtual void Init();
	virtual bool Input( const InputEventPlus &input );
	virtual void TweenOnScreen();

private:
	enum Focus { FOCUS_ARCADE = 1, FOCUS_BATTLE = 2 };
	bool	m_bCentered;	// first Center press; the second one confirms
	bool	m_bDone;
	Focus	m_Focus;
};

#endif
