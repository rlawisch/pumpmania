/* ScreenSelectAvatar - Pick the avatar of a local profile; the theme draws everything (STEP_P1). */

#ifndef SCREEN_SELECT_AVATAR_H
#define SCREEN_SELECT_AVATAR_H

#include "ScreenWithMenuElements.h"

class ScreenSelectAvatar : public ScreenWithMenuElements
{
public:
	bool FinishSelectingAvatar();

	virtual void PushSelf( lua_State *L );
};

#endif
