#include "global.h"
#include "ScreenSelectAvatar.h"
#include "ScreenManager.h"

// Recreated from STEP_P1.exe (ScreenSelectAvatar at 0x647173-0x647408).

REGISTER_SCREEN_CLASS( ScreenSelectAvatar );

bool ScreenSelectAvatar::FinishSelectingAvatar()
{
	m_sNextScreen = "ScreenTitleMenu";
	StartTransitioningScreen( SM_GoToNextScreen );
	return true;
}

// lua start
#include "LuaBinding.h"

class LunaScreenSelectAvatar : public Luna<ScreenSelectAvatar>
{
public:
	static int FinishSelectingAvatar( T* p, lua_State *L ) { lua_pushboolean( L, p->FinishSelectingAvatar() ); return 1; }

	LunaScreenSelectAvatar()
	{
		ADD_METHOD( FinishSelectingAvatar );
	}
};

LUA_REGISTER_DERIVED_CLASS( ScreenSelectAvatar, ScreenWithMenuElements )
// lua end
