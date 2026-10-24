/* ModIconsList - Icons of the active mods next to each player on ScreenSelectMusic, recreated from STEP_P1.exe. */

#ifndef MOD_ICONS_LIST_H
#define MOD_ICONS_LIST_H

#include "ActorFrame.h"
#include "Sprite.h"
#include "BitmapText.h"
#include "PlayerNumber.h"
#include "RageTimer.h"

/** @brief The active mods of one category; cycles through them when there is more than one. */
class ModIconRev : public ActorFrame
{
public:
	ModIconRev();

	void Init( int iCategory, PlayerNumber pn, bool bAnimate );
	// Rebuild the icon list; iItem picks which icon to show first.
	void Refresh( bool bAnimate, int iItem );
	void NextIcon();

private:
	void Display();
	void Pulse();

	PlayerNumber	m_pn;
	int				m_iCurrent;
	int				m_iCategory;
	vector<int>		m_vIcons;
	vector<int>		m_vAllIcons;
	Sprite			m_sprIcon;
	Sprite			m_sprGlow2;
	Sprite			m_sprGlow1;
	Sprite			m_sprPreview;
	bool			m_bShowPreview;
	BitmapText		m_textValue;
	bool			m_bShowText;
};

class ModIconsList : public ActorFrame
{
public:
	ModIconsList();

	void Load( PlayerNumber pn, bool bGrid );
	// Refresh one category after a mod changed; iItem is the item that changed.
	void Set( int iCategory, bool bAnimate, int iItem );

	virtual void Update( float fDeltaTime );

private:
	PlayerNumber	m_pn;
	RageTimer		m_Timer;
	ModIconRev		m_Icons[10];
};

#endif
