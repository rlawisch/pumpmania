/* OptionsList - The per-player options menu of ScreenSelectMusic, recreated from STEP_P1.exe. */

#ifndef OPTIONS_LIST_H
#define OPTIONS_LIST_H

#include "ActorFrame.h"
#include "Sprite.h"
#include "RageSound.h"
#include "PlayerNumber.h"

class InputEventPlus;
class ModIconsList;

/** @brief One option shown in the list: its icon and, for noteskins, a preview. */
class ModIconSimple : public ActorFrame
{
public:
	ModIconSimple();

	Sprite	m_sprIcon;		// OptionList/icons 11x11
	Sprite	m_sprPreview;
	bool	m_bHasPreview;
	int		m_iItem;
};

class OptionsList : public ActorFrame
{
public:
	OptionsList();

	void SetPlayer( PlayerNumber pn );
	void Link( OptionsList *pLink ) { m_pLinked = pLink; }
	void SetEnabled( bool b ) { m_bEnabled = b; }
	void Reset();
	void SetModList( ModIconsList *p ) { m_pModList = p; }

	void Open();
	void Close();
	bool Input( const InputEventPlus &input );
	bool IsOpened() const { return m_bOpened; }

	virtual void HandleMessage( const Message &msg );
	virtual void DrawPrimitives();

private:
	struct Menu
	{
		int iIcon;		// frame in OptionList/MenuIcons 1x9
		int iCategory;	// StepP1Mods::Category
		int iText;		// frame in OptionList/Texts 8x16
	};
	const Menu &CurrentMenu() const { return m_vMenus[m_iMenu]; }
	int MenuIndex( int iOffset ) const;
	int ItemIndex( int iOffset ) const;

	void ScrollMenus( int iDir );
	void Refresh( const RString &sCommand );
	void Animate( const RString &sCommand );
	void ShowSlotItem( ModIconSimple &slot, int iItem );
	void UpdateSlotDiffuse( ModIconSimple &slot );
	void SlideSlot( ModIconSimple &slot, float fFromX, float fFromZoomX, float fToX, float fToZoomX, float fAlign );
	void SweepWhiteSmall( Sprite &spr, float fStartX );
	void Blink( Sprite &spr );

	PlayerNumber	m_pn;
	int				m_iMenu;
	int				m_iItem;
	bool			m_bLoaded;
	bool			m_bOpened;
	bool			m_bInMenus;	// choosing a menu, not an item
	bool			m_bArrowsBigPulse;
	bool			m_bArrowsSmPulse;
	bool			m_bEnabled;
	bool			m_bWhitePulse;
	bool			m_bTextPulse;
	int				m_iTextState;
	OptionsList		*m_pLinked;
	ModIconsList	*m_pModList;
	vector<Menu>	m_vMenus;

	Sprite			m_sprBg;
	Sprite			m_sprBlack1;
	Sprite			m_sprBlack2;
	Sprite			m_sprArrowsBig;
	Sprite			m_sprArrowsSm;
	Sprite			m_sprBg2;
	Sprite			m_sprNextArrow;
	Sprite			m_sprPrevArrow;
	Sprite			m_sprBackArrow1;
	Sprite			m_sprBackArrow2;
	Sprite			m_sprWhiteSmall1;
	Sprite			m_sprWhiteSmall2;
	Sprite			m_sprWhiteBig;
	ActorFrame		m_frameMenus;
	Sprite			m_sprMenuIcon[4];	// left, center, right, leaving
	Sprite			m_sprTexts;
	ActorFrame		m_frameItems;
	ModIconSimple	m_Slots[4];		// left, center, right, leaving

	RageSound		m_soundMove;
	RageSound		m_soundInOut;
	RageSound		m_soundChange;
	RageSound		m_soundSet;
	RageSound		m_soundBad;
	RageSound		m_soundBattle[5];
};

#endif

/*
 * (c) 2006 Glenn Maynard
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
