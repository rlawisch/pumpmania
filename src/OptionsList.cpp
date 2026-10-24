#include "global.h"
#include "OptionsList.h"
#include "StepP1Mods.h"
#include "GameState.h"
#include "PlayerState.h"
#include "ThemeManager.h"
#include "InputEventPlus.h"
#include "MessageManager.h"
#include "NoteSkinManager.h"
#include "RageFileManager.h"
#include "ScreenDimensions.h"
#include "ModIconsList.h"

// Recreated from STEP_P1.exe (OptionsList at 0x549xxx-0x54fxxx, ModIconSimple at 0x54a314).

using namespace StepP1Mods;

static const float BASE_ZOOM = 0.66f;

static void LoadOptionListGraphic( Sprite &spr, const RString &sFile )
{
	spr.Load( THEME->GetPathG("", "OptionList/" + sFile) );
}

ModIconSimple::ModIconSimple()
{
	m_sprIcon.Load( THEME->GetPathG("", "OptionList/icons 11x11.png") );
	m_sprIcon.StopAnimating();
	m_bHasPreview = false;
	m_iItem = 0;
	this->AddChild( &m_sprIcon );
	this->AddChild( &m_sprPreview );
}

OptionsList::OptionsList()
{
	m_pn = PLAYER_INVALID;
	m_iMenu = 0;
	m_iItem = 0;
	m_bLoaded = false;
	m_bOpened = false;
	m_bInMenus = true;
	m_bEnabled = true;
	m_bArrowsBigPulse = false;
	m_bArrowsSmPulse = false;
	m_bWhitePulse = false;
	m_bTextPulse = false;
	m_iTextState = -1;
	m_pLinked = nullptr;
	m_pModList = nullptr;

	// { menu icon, category, description text }
	static const Menu menus[] = {
		{ 0, CAT_SPEED, 16 }, { 8, CAT_AV, 120 }, { 1, CAT_DISPLAY, 91 }, { 2, CAT_NOTESKIN, 57 },
		{ 3, CAT_PATH, 99 }, { 4, CAT_ALTERNATE, 68 }, { 5, CAT_JUDGE, 108 }, { 6, CAT_RATE, 6 },
	};
	for( unsigned i = 0; i < ARRAYLEN(menus); ++i )
		m_vMenus.push_back( menus[i] );
	if( GAMESTATE->m_PlayMode == PLAY_MODE_BATTLE )
	{
		Menu battle = { 8, CAT_BATTLE, 0 };
		m_vMenus.push_back( battle );
	}
	Menu reset = { 7, CAT_RESET, 7 };
	m_vMenus.push_back( reset );

	m_soundMove.Load( THEME->GetPathS("", "CW/S_CMD_MOVE"), true );
	m_soundInOut.Load( THEME->GetPathS("", "CW/S_CMD_INOUT"), true );
	m_soundChange.Load( THEME->GetPathS("", "CW/S_CMD_CHG"), true );
	m_soundSet.Load( THEME->GetPathS("", "CW/S_CMD_SET"), true );
	m_soundBad.Load( THEME->GetPathS("", "Sounds/BAD"), true );
	if( GAMESTATE->m_PlayMode == PLAY_MODE_BATTLE )
	{
		static const char *battleSounds[] = { "20_SCORE_BATTLE", "21_LOW_SCORE_BATTLE", "22_COMBO_BATTLE", "23_LOW_COMBO_BATTLE", "24_MORE_GOOD_BATTLE" };
		for( int i = 0; i < 5; ++i )
			m_soundBattle[i].Load( THEME->GetPathS("", RString("Sounds/") + battleSounds[i]), true );
	}

	for( int i = 0; i < 4; ++i )
		LoadOptionListGraphic( m_sprMenuIcon[i], "MenuIcons 1x9.png" );
	LoadOptionListGraphic( m_sprTexts, "Texts 8x16.png" );
	LoadOptionListGraphic( m_sprBg, "bg.png" );
	LoadOptionListGraphic( m_sprBlack1, "black.png" );
	LoadOptionListGraphic( m_sprBlack2, "black.png" );
	LoadOptionListGraphic( m_sprNextArrow, "NextArrow.png" );
	LoadOptionListGraphic( m_sprPrevArrow, "PrevArrow.png" );
	LoadOptionListGraphic( m_sprBackArrow1, "BackArrow1.png" );
	LoadOptionListGraphic( m_sprBackArrow2, "BackArrow2.png" );
	LoadOptionListGraphic( m_sprWhiteSmall1, "WhiteSmall.png" );
	LoadOptionListGraphic( m_sprWhiteSmall2, "WhiteSmall.png" );
	LoadOptionListGraphic( m_sprWhiteBig, "WhiteBig.png" );
	LoadOptionListGraphic( m_sprArrowsBig, "arrows_big.png" );
	LoadOptionListGraphic( m_sprArrowsSm, "arrows_sm.png" );
	LoadOptionListGraphic( m_sprBg2, "bg.png" );

	Sprite *pBaseZoomed[] = { &m_sprBg, &m_sprBlack1, &m_sprBlack2, &m_sprNextArrow, &m_sprPrevArrow, &m_sprBackArrow1,
		&m_sprBackArrow2, &m_sprWhiteSmall1, &m_sprWhiteSmall2, &m_sprWhiteBig, &m_sprArrowsBig, &m_sprArrowsSm, &m_sprBg2 };
	for( unsigned i = 0; i < ARRAYLEN(pBaseZoomed); ++i )
		pBaseZoomed[i]->SetBaseZoom( BASE_ZOOM );
	m_sprBlack2.SetY( -26 );
	m_sprArrowsSm.SetY( -26 );

	// Menu icons: left, center and right of the current menu, plus one sliding out.
	m_sprMenuIcon[0].SetBaseZoom( BASE_ZOOM );
	m_sprMenuIcon[0].StopAnimating();
	m_sprMenuIcon[0].SetX( -72 );
	m_sprMenuIcon[0].SetHorizAlign( 0 );
	m_sprMenuIcon[0].SetState( m_vMenus[MenuIndex(-1)].iIcon );
	m_sprMenuIcon[0].SetZoomX( 0.55f );
	m_sprMenuIcon[0].SetDiffuseAlpha( 0.5f );

	m_sprMenuIcon[1].SetBaseZoom( BASE_ZOOM );
	m_sprMenuIcon[1].StopAnimating();
	m_sprMenuIcon[1].SetHorizAlign( 0.5f );
	m_sprMenuIcon[1].SetState( m_vMenus[0].iIcon );
	m_sprMenuIcon[1].SetDiffuseAlpha( 1 );

	m_sprMenuIcon[2].SetBaseZoom( BASE_ZOOM );
	m_sprMenuIcon[2].StopAnimating();
	m_sprMenuIcon[2].SetX( 72 );
	m_sprMenuIcon[2].SetHorizAlign( 1 );
	m_sprMenuIcon[2].SetState( m_vMenus[MenuIndex(+1)].iIcon );
	m_sprMenuIcon[2].SetZoomX( 0.55f );
	m_sprMenuIcon[2].SetDiffuseAlpha( 0.5f );

	m_sprMenuIcon[3].SetBaseZoom( BASE_ZOOM );
	m_sprMenuIcon[3].StopAnimating();
	m_sprMenuIcon[3].SetDiffuseAlpha( 0 );

	// Item slots.
	m_Slots[3].m_sprIcon.SetHorizAlign( 0.5f );
	m_Slots[3].SetBaseZoom( 0.67f );
	m_Slots[3].SetZoom( 0 );
	m_Slots[0].SetX( -50 );
	m_Slots[0].m_sprIcon.SetHorizAlign( 0 );
	m_Slots[0].SetBaseZoom( 0.67f );
	m_Slots[0].SetZoom( 0 );
	m_Slots[1].m_sprIcon.SetHorizAlign( 1 );
	m_Slots[1].SetBaseZoom( 0.67f );
	m_Slots[1].SetX( 20.7f );
	m_Slots[1].SetZoom( 0 );
	m_Slots[2].SetX( 50 );
	m_Slots[2].m_sprIcon.SetHorizAlign( 1 );
	m_Slots[2].SetBaseZoom( 0.67f );
	m_Slots[2].SetZoom( 0 );

	this->AddChild( &m_sprBg );
	m_sprBlack1.SetY( -26 );
	m_sprBlack1.SetDiffuseAlpha( 0 );
	this->AddChild( &m_sprBlack1 );
	this->AddChild( &m_sprBlack2 );

	m_sprNextArrow.SetXY( 54, 86 );
	m_sprNextArrow.SetDiffuseAlpha( 0 );
	this->AddChild( &m_sprNextArrow );
	m_sprPrevArrow.SetXY( -54, 86 );
	m_sprPrevArrow.SetDiffuseAlpha( 0 );
	this->AddChild( &m_sprPrevArrow );
	m_sprBackArrow1.SetXY( -54, -86 );
	m_sprBackArrow1.SetDiffuseAlpha( 0 );
	this->AddChild( &m_sprBackArrow1 );
	m_sprBackArrow2.SetXY( 54, -86 );
	m_sprBackArrow2.SetDiffuseAlpha( 0 );
	this->AddChild( &m_sprBackArrow2 );

	m_sprWhiteSmall1.SetDiffuseAlpha( 0 );
	m_sprWhiteSmall2.SetDiffuseAlpha( 0 );
	m_sprWhiteBig.SetDiffuseAlpha( 0 );

	m_frameMenus.SetY( -26 );
	m_frameMenus.AddChild( &m_sprWhiteSmall1 );
	for( int i = 0; i < 4; ++i )
		m_frameMenus.AddChild( &m_sprMenuIcon[i] );
	this->AddChild( &m_frameMenus );

	m_frameItems.AddChild( &m_sprWhiteSmall2 );
	for( int i = 0; i < 4; ++i )
		m_frameItems.AddChild( &m_Slots[i] );
	m_frameItems.SetY( -46 );
	this->AddChild( &m_frameItems );

	m_sprArrowsBig.SetZoom( 0 );
	m_sprArrowsBig.SetDiffuseAlpha( 0 );
	this->AddChild( &m_sprArrowsBig );
	m_sprArrowsSm.SetY( -46 );
	m_sprArrowsSm.SetZoom( 0 );
	m_sprArrowsSm.SetDiffuseAlpha( 0 );
	this->AddChild( &m_sprArrowsSm );

	m_sprBg2.SetDiffuseAlpha( 0 );
	this->AddChild( &m_sprBg2 );

	m_sprWhiteBig.SetY( 42 );
	this->AddChild( &m_sprWhiteBig );

	m_sprTexts.StopAnimating();
	m_sprTexts.SetState( 0 );
	m_sprTexts.SetBaseZoom( BASE_ZOOM );
	m_sprTexts.SetY( 40 );
	this->AddChild( &m_sprTexts );
}

int OptionsList::MenuIndex( int iOffset ) const
{
	int iNum = m_vMenus.size();
	return ((m_iMenu + iOffset) % iNum + iNum) % iNum;
}

int OptionsList::ItemIndex( int iOffset ) const
{
	int iNum = GetNumItems( CurrentMenu().iCategory );
	if( iNum <= 0 )
		return 0;
	return ((m_iItem + iOffset) % iNum + iNum) % iNum;
}

// Recreated from 0x54a3d7.
void OptionsList::SetPlayer( PlayerNumber pn )
{
	m_iMenu = 0;
	m_iItem = 0;
	m_pn = pn;
	m_bLoaded = true;
	SetX( pn == PLAYER_1 ? -100.0f : SCREEN_WIDTH + 100.0f );
	SetY( SCREEN_HEIGHT * 0.5f - 40 );
}

// Recreated from 0x54cce3.
void OptionsList::Open()
{
	StopTweening();
	m_bInMenus = true;
	m_bOpened = true;
	Animate( "Open" );
	SetVisible( true );
	if( m_pn == PLAYER_1 )
	{
		SetX( -100 );
		BeginTweening( 0.1f );
		SetX( 145 );
		BeginTweening( 0.1f );
		SetX( 135 );
	}
	else
	{
		SetX( SCREEN_WIDTH + 100 );
		BeginTweening( 0.1f );
		SetX( SCREEN_WIDTH - 145 );
		BeginTweening( 0.1f );
		SetX( SCREEN_WIDTH - 135 );
	}
}

// Recreated from 0x54a454.
void OptionsList::Close()
{
	m_bOpened = false;
	m_soundInOut.Play( true );
	if( m_pn == PLAYER_1 )
	{
		SetX( 135 );
		Sleep( 0.1f );
		BeginTweening( 0.1f );
		SetX( 145 );
		BeginTweening( 0.1f );
		SetX( -100 );
	}
	else
	{
		SetX( SCREEN_WIDTH - 135 );
		BeginTweening( 0.1f );
		SetX( SCREEN_WIDTH - 145 );
		BeginTweening( 0.1f );
		SetX( SCREEN_WIDTH + 100 );
	}
	m_bWhitePulse = false;
}

// Recreated from 0x54a5c8: the menu timer running out slides the list away.
void OptionsList::HandleMessage( const Message &msg )
{
	if( msg.GetName() == "TimerOutSelectingSong" || msg.GetName() == "TimerOutSelectingGroup" )
	{
		if( m_pn == PLAYER_1 )
		{
			SetX( 135 );
			Sleep( 0.1f );
			BeginTweening( 0.1f );
			SetX( 145 );
			BeginTweening( 0.1f );
			SetX( -100 );
		}
		else
		{
			SetX( SCREEN_WIDTH - 135 );
			BeginTweening( 0.1f );
			SetX( SCREEN_WIDTH - 145 );
			BeginTweening( 0.1f );
			SetX( SCREEN_WIDTH + 100 );
		}
	}
	ActorFrame::HandleMessage( msg );
}

// Recreated from 0x54c503.
void OptionsList::ScrollMenus( int iDir )
{
	Sprite &leaving = m_sprMenuIcon[3];
	Sprite &left = m_sprMenuIcon[0];
	Sprite &center = m_sprMenuIcon[1];
	Sprite &right = m_sprMenuIcon[2];

	leaving.StopTweening();
	left.StopTweening();
	center.StopTweening();
	right.StopTweening();
	if( iDir == 1 )
	{
		leaving.SetState( m_vMenus[MenuIndex(-2)].iIcon );
		leaving.SetX( -72 );
		leaving.SetHorizAlign( 0 );
		leaving.SetZoomX( 0.55f );
		leaving.SetDiffuseAlpha( 0.5f );
		leaving.BeginTweening( 0.25f );
		leaving.SetX( -84 );
		leaving.SetDiffuseAlpha( 0 );

		left.SetState( m_vMenus[MenuIndex(-1)].iIcon );
		left.SetX( -34 );
		left.SetHorizAlign( 0 );
		left.SetZoomX( 1 );
		left.SetDiffuseAlpha( 1 );
		left.BeginTweening( 0.25f );
		left.SetX( -72 );
		left.SetZoomX( 0.55f );
		left.SetDiffuseAlpha( 0.5f );

		center.SetState( m_vMenus[MenuIndex(0)].iIcon );
		center.SetX( 72 );
		center.SetHorizAlign( 1 );
		center.SetZoomX( 0.55f );
		center.SetDiffuseAlpha( 0.5f );
		center.BeginTweening( 0.25f );
		center.SetX( 34 );
		center.SetZoomX( 1 );
		center.SetDiffuseAlpha( 1 );

		right.SetState( m_vMenus[MenuIndex(+1)].iIcon );
		right.SetX( 84 );
		right.SetHorizAlign( 1 );
		right.SetZoomX( 0.55f );
		right.SetDiffuseAlpha( 0 );
	}
	else
	{
		leaving.SetState( m_vMenus[MenuIndex(+2)].iIcon );
		leaving.SetX( 72 );
		leaving.SetHorizAlign( 1 );
		leaving.SetZoomX( 0.55f );
		leaving.SetDiffuseAlpha( 0.5f );
		leaving.BeginTweening( 0.25f );
		leaving.SetX( 84 );
		leaving.SetDiffuseAlpha( 0 );

		left.SetState( m_vMenus[MenuIndex(-1)].iIcon );
		left.SetX( -84 );
		left.SetHorizAlign( 0 );
		left.SetZoomX( 0.55f );
		left.SetDiffuseAlpha( 0 );
		left.BeginTweening( 0.25f );
		left.SetX( -72 );
		left.SetDiffuseAlpha( 0.5f );

		center.SetState( m_vMenus[MenuIndex(0)].iIcon );
		center.SetX( -72 );
		center.SetHorizAlign( 0 );
		center.SetZoomX( 0.55f );
		center.SetDiffuseAlpha( 0.5f );
		center.BeginTweening( 0.25f );
		center.SetX( -34 );
		center.SetZoomX( 1 );
		center.SetDiffuseAlpha( 1 );

		right.SetState( m_vMenus[MenuIndex(+1)].iIcon );
		right.SetX( 34 );
		right.SetHorizAlign( 1 );
		right.SetZoomX( 1 );
		right.SetDiffuseAlpha( 1 );
	}
	right.BeginTweening( 0.25f );
	right.SetX( 72 );
	right.SetZoomX( 0.55f );
	right.SetDiffuseAlpha( 0.5f );
}

void OptionsList::ShowSlotItem( ModIconSimple &slot, int iItem )
{
	int iCategory = CurrentMenu().iCategory;
	slot.FinishTweening();
	slot.m_iItem = iItem;
	slot.m_sprIcon.SetState( GetIcon(iCategory, iItem) );
	slot.m_sprIcon.SetVisible( true );
	slot.m_bHasPreview = false;
	slot.m_sprPreview.SetVisible( false );
	if( iCategory != CAT_NOTESKIN )
		return;

	// Noteskins without an icon in icons 11x11 show their NOTE_LOGO.png instead.
	const vector<NoteSkinEntry> &v = GetNoteSkins();
	if( iItem < 0 || iItem >= (int)v.size() || v[iItem].bHasIcon )
		return;
	RString sLogo = "/NoteSkins/pump/" + v[iItem].sName + "/NOTE_LOGO.png";
	if( !DoesFileExist(sLogo) )
		return;
	slot.m_sprPreview.Load( sLogo );
	slot.m_sprPreview.SetVisible( true );
	slot.m_sprIcon.SetVisible( false );
}

void OptionsList::UpdateSlotDiffuse( ModIconSimple &slot )
{
	int iCategory = CurrentMenu().iCategory;
	const PlayerOptions &po = GAMESTATE->m_pPlayerState[m_pn]->m_PlayerOptions.GetPreferred();
	bool bLit = IsActive( po, iCategory, slot.m_iItem ) || iCategory == CAT_AV;
	float f = bLit ? 1.0f : 0.3f;
	slot.SetDiffuse( RageColor(f, f, f, 1) );
}

// Recreated from 0x54cdfd: fill the item slots, then animate.
void OptionsList::Refresh( const RString &sCommand )
{
	if( sCommand == "ModSelected" )
	{
		for( int i = 0; i < 4; ++i )
			UpdateSlotDiffuse( m_Slots[i] );
		if( CurrentMenu().iCategory == CAT_BATTLE )
		{
			for( int i = 0; i < 5; ++i )
				m_soundBattle[i].Stop();
			if( m_iItem >= 0 && m_iItem < 5 )
				m_soundBattle[m_iItem].Play( true );
		}
	}
	else if( sCommand == "GoBackSelectingMenu" )
	{
		for( int i = 0; i < 4; ++i )
			m_Slots[i].FinishTweening();
		if( CurrentMenu().iCategory == CAT_NOTESKIN )
		{
			for( int i = 0; i < 4; ++i )
				m_Slots[i].m_bHasPreview = false;
		}
		m_sprTexts.SetState( CurrentMenu().iText );
	}
	else if( sCommand != "Reset" && sCommand != "Refresh" )
	{
		int iDir = sCommand == "NextMod" ? 1 : -1;
		ShowSlotItem( m_Slots[3], ItemIndex(iDir == 1 ? -2 : +2) );
		ShowSlotItem( m_Slots[0], ItemIndex(-1) );
		ShowSlotItem( m_Slots[1], ItemIndex(0) );
		ShowSlotItem( m_Slots[2], ItemIndex(+1) );

		m_iTextState = GetText( CurrentMenu().iCategory, m_iItem );
		m_sprTexts.FinishTweening();
		m_sprTexts.SetState( m_iTextState );
		m_sprTexts.BeginTweening( 0.24f );
		m_bTextPulse = true;
		for( int i = 0; i < 4; ++i )
			UpdateSlotDiffuse( m_Slots[i] );
	}
	else
	{
		for( int i = 0; i < 4; ++i )
			UpdateSlotDiffuse( m_Slots[i] );
	}
	Animate( sCommand );
}

void OptionsList::Blink( Sprite &spr )
{
	spr.StopTweening();
	spr.SetDiffuseAlpha( 1 );
	spr.BeginTweening( 0.2f );
	spr.SetDiffuseAlpha( 0 );
}

// The highlight bar that turns over when moving between menus or items.
void OptionsList::SweepWhiteSmall( Sprite &spr, float fStartX )
{
	spr.StopTweening();
	spr.SetRotationZ( 0 );
	spr.SetZoom( 1 );
	spr.SetHorizAlign( fStartX > 0 ? 1.0f : 0.0f );
	spr.SetZoomX( 0 );
	spr.SetX( fStartX );
	spr.SetDiffuseAlpha( 0.5f );
	spr.BeginTweening( 0.125f );
	spr.SetZoomX( 1 );
	spr.SetDiffuseAlpha( 1 );
	spr.Sleep( 0 );
	spr.SetRotationZ( 180 );
	spr.SetX( -fStartX );
	spr.BeginTweening( 0.125f );
	spr.SetZoomX( 0 );
	spr.SetDiffuseAlpha( 0.5f );
	spr.Sleep( 0 );
	spr.SetRotationZ( 0 );
	spr.Sleep( 0.1f );
	m_bWhitePulse = true;
}

void OptionsList::SlideSlot( ModIconSimple &slot, float fFromX, float fFromZoomX, float fToX, float fToZoomX, float fAlign )
{
	slot.SetZoomY( 1 );
	slot.SetX( fFromX );
	slot.m_sprIcon.SetHorizAlign( fAlign );
	if( slot.m_bHasPreview )
	{
		slot.m_sprPreview.SetHorizAlign( fAlign );
		float fHalf = slot.m_sprPreview.GetZoomedWidth() * 0.5f;
		slot.m_sprPreview.SetX( fAlign < 0.5f ? 31 - fHalf : fHalf - 31 );
	}
	slot.SetZoomX( fFromZoomX );
	slot.BeginTweening( 0.25f );
	slot.SetX( fToX );
	slot.SetZoomX( fToZoomX );
}

// Recreated from 0x54a743.
void OptionsList::Animate( const RString &sCommand )
{
	if( sCommand == "Open" )
	{
		m_soundInOut.Stop();
		m_soundInOut.Play( true );
		m_sprArrowsSm.StopTweening();
		m_bArrowsSmPulse = false;
		m_sprTexts.SetState( CurrentMenu().iText );
		m_sprArrowsBig.StopTweening();
		m_sprArrowsBig.SetDiffuseAlpha( 0 );
		m_sprArrowsBig.SetZoom( 0 );
		m_sprArrowsBig.Sleep( 0.2f );
		m_sprArrowsBig.BeginTweening( 0.2f );
		m_sprArrowsBig.SetZoom( 1 );
		m_sprArrowsBig.SetDiffuseAlpha( 1 );
		m_bArrowsBigPulse = true;
		m_sprBg2.StopTweening();
		m_sprBg2.SetDiffuseAlpha( 0 );
		m_sprBg2.Sleep( 0.2f );
		m_sprBg2.BeginTweening( 0.05f );
		m_sprBg2.SetDiffuseAlpha( 1 );
		m_sprBg2.BeginTweening( 0.25f );
		m_sprBg2.SetDiffuseAlpha( 0 );
		m_sprWhiteSmall1.StopTweening();
		m_sprWhiteSmall1.SetDiffuseAlpha( 0 );
		m_sprWhiteSmall1.Sleep( 0.3f );
		m_sprWhiteSmall1.SetDiffuseAlpha( 0 );
		m_sprWhiteBig.StopTweening();
		m_sprWhiteBig.SetDiffuseAlpha( 0 );
		m_sprWhiteSmall2.StopTweening();
		m_sprWhiteSmall2.SetDiffuseAlpha( 0 );
	}
	else if( sCommand == "NextMod" || sCommand == "PrevMod" )
	{
		bool bNext = sCommand == "NextMod";
		m_soundMove.Stop();
		m_soundMove.Play( true );
		if( bNext )
		{
			SlideSlot( m_Slots[3], -70, 1, -72, 0, 0 );
			SlideSlot( m_Slots[0], -20.77f, 1, -70, 1, 0 );
			SlideSlot( m_Slots[1], 70, 1, 20.77f, 1, 1 );
			SlideSlot( m_Slots[2], 72, 0, 70, 1, 1 );
		}
		else
		{
			SlideSlot( m_Slots[3], 70, 1, 72, 0, 1 );
			SlideSlot( m_Slots[0], -72, 0, -70, 1, 0 );
			SlideSlot( m_Slots[1], -70, 1, -20.77f, 1, 0 );
			SlideSlot( m_Slots[2], 20.77f, 1, 70, 1, 1 );
		}
		m_sprArrowsSm.FinishTweening();
		m_sprArrowsSm.SetZoom( 1 );
		m_sprArrowsSm.BeginTweening( 0.1f );
		m_sprArrowsSm.SetZoom( 1.3f );
		m_sprArrowsSm.BeginTweening( 0.15f );
		m_sprArrowsSm.SetZoom( 1 );
		m_bArrowsSmPulse = true;
		Blink( bNext ? m_sprNextArrow : m_sprPrevArrow );
		if( !bNext )
			Blink( m_sprNextArrow );

		m_sprWhiteSmall1.StopTweening();
		m_sprWhiteSmall1.SetHorizAlign( 0.5f );
		m_sprWhiteSmall1.SetX( 0 );
		m_sprWhiteSmall1.SetRotationZ( 0 );
		m_sprWhiteSmall1.SetZoom( 1 );
		m_sprWhiteSmall1.SetDiffuseAlpha( 0 );
		m_sprWhiteSmall1.BeginTweening( 0.125f );
		m_sprWhiteSmall1.SetDiffuseAlpha( 1 );
		m_sprWhiteSmall1.BeginTweening( 0.125f );
		m_sprWhiteSmall1.SetDiffuseAlpha( 0 );
		m_sprWhiteSmall1.Sleep( 0.1f );

		m_sprWhiteBig.StopTweening();
		m_sprWhiteBig.SetZoom( 1 );
		m_sprWhiteBig.SetDiffuseAlpha( 0 );
		m_sprWhiteBig.BeginTweening( 0.125f );
		m_sprWhiteBig.SetDiffuseAlpha( 1 );
		m_sprWhiteBig.BeginTweening( 0.125f );
		m_sprWhiteBig.SetDiffuseAlpha( 0 );
		m_sprWhiteBig.Sleep( 0.1f );

		SweepWhiteSmall( m_sprWhiteSmall2, bNext ? 70.0f : -70.0f );
	}
	else if( sCommand == "MenuSelected" )
	{
		m_soundChange.Stop();
		m_soundChange.Play( true );
		for( int i = 3; i >= 0; --i )
			m_sprMenuIcon[i].FinishTweening();
		m_frameMenus.FinishTweening();
		m_frameMenus.SetY( -26 );
		m_frameMenus.BeginTweening( 0.1f );
		m_frameMenus.SetY( -6 );

		const float fSlotX[3] = { -50.77f, 0, 50.77f };
		for( int i = 0; i < 3; ++i )
		{
			ModIconSimple &slot = m_Slots[i];
			slot.m_sprIcon.SetHorizAlign( 0.5f );
			if( slot.m_bHasPreview )
			{
				slot.m_sprPreview.SetHorizAlign( 0.5f );
				slot.m_sprPreview.SetX( 0 );
			}
			slot.SetZoom( 0 );
			slot.SetX( fSlotX[i] );
			slot.Sleep( 0.1f );
			slot.BeginTweening( 0.1f );
			slot.SetZoom( 1 );
		}

		m_sprArrowsBig.StopTweening();
		m_sprArrowsBig.SetZoom( 1 );
		m_sprArrowsBig.SetDiffuseAlpha( 0 );
		m_bArrowsBigPulse = false;
		m_sprArrowsSm.StopTweening();
		m_sprArrowsSm.SetDiffuseAlpha( 0 );
		m_sprArrowsSm.Sleep( 0.1f );
		m_sprArrowsSm.SetZoom( 1.15f );
		m_sprArrowsSm.BeginTweening( 0.1f );
		m_sprArrowsSm.SetZoom( 1 );
		m_sprArrowsSm.SetDiffuseAlpha( 1 );
		m_sprArrowsSm.Sleep( 0.1f );
		m_bArrowsSmPulse = true;

		m_sprBg2.FinishTweening();
		m_sprBg2.SetDiffuseAlpha( 1 );
		m_sprBg2.BeginTweening( 0.2f );
		m_sprBg2.SetDiffuseAlpha( 0 );

		m_sprBlack1.StopTweening();
		m_sprBlack1.SetDiffuseAlpha( 0 );
		m_sprBlack1.SetY( -26 );
		m_sprBlack1.BeginTweening( 0.1f );
		m_sprBlack1.SetY( -46 );
		m_sprBlack1.SetDiffuseAlpha( 1 );

		m_sprBlack2.StopTweening();
		m_sprBlack2.SetY( -26 );
		m_sprBlack2.BeginTweening( 0.1f );
		m_sprBlack2.SetY( -6 );

		Sprite *pFlash[] = { &m_sprWhiteSmall1, &m_sprWhiteBig, &m_sprWhiteSmall2 };
		for( unsigned i = 0; i < ARRAYLEN(pFlash); ++i )
		{
			Sprite &spr = *pFlash[i];
			spr.StopTweening();
			spr.SetHorizAlign( 0.5f );
			spr.SetX( 0 );
			spr.SetRotationZ( 0 );
			spr.SetZoom( 1 );
			spr.Sleep( 0.1f );
			spr.SetDiffuseAlpha( 0 );
			spr.BeginTweening( 0.1f );
			spr.SetDiffuseAlpha( 1 );
			spr.SetZoom( 1.2f );
			spr.Sleep( 0 );
			spr.SetDiffuseAlpha( 0 );
		}
	}
	else if( sCommand == "GoBackSelectingMenu" )
	{
		m_soundChange.Stop();
		m_soundChange.Play( true );
		m_Slots[3].SetZoom( 0 );
		const float fSlotX[3] = { -50.77f, 0, 50.77f };
		for( int i = 0; i < 3; ++i )
		{
			ModIconSimple &slot = m_Slots[i];
			slot.m_sprIcon.SetHorizAlign( 0.5f );
			slot.SetZoom( 1 );
			slot.SetX( fSlotX[i] );
			slot.BeginTweening( 0.1f );
			slot.SetZoom( 0 );
		}

		m_sprArrowsBig.StopTweening();
		m_sprArrowsBig.SetDiffuseAlpha( 0 );
		m_sprArrowsBig.SetZoom( 1.5f );
		m_sprArrowsBig.Sleep( 0.1f );
		m_sprArrowsBig.BeginTweening( 0.1f );
		m_sprArrowsBig.SetZoom( 1 );
		m_sprArrowsBig.SetDiffuseAlpha( 1 );
		m_bArrowsBigPulse = true;

		m_sprArrowsSm.FinishTweening();
		m_sprArrowsSm.SetZoom( 1.15f );
		m_sprArrowsSm.BeginTweening( 0.05f );
		m_sprArrowsSm.SetZoom( 1 );
		m_sprArrowsSm.BeginTweening( 0.05f );
		m_sprArrowsSm.SetZoom( 1.2f );
		m_sprArrowsSm.SetDiffuseAlpha( 0 );
		m_bArrowsSmPulse = false;

		m_sprBlack1.StopTweening();
		m_sprBlack1.SetDiffuseAlpha( 1 );
		m_sprBlack1.SetY( -46 );
		m_sprBlack1.Sleep( 0.1f );
		m_sprBlack1.BeginTweening( 0.1f );
		m_sprBlack1.SetDiffuseAlpha( 0 );
		m_sprBlack1.SetY( -26 );

		m_sprBlack2.StopTweening();
		m_sprBlack2.SetY( -6 );
		m_sprBlack2.Sleep( 0.1f );
		m_sprBlack2.BeginTweening( 0.1f );
		m_sprBlack2.SetY( -26 );

		m_frameMenus.FinishTweening();
		m_frameMenus.SetY( -6 );
		m_frameMenus.Sleep( 0.1f );
		m_frameMenus.BeginTweening( 0.1f );
		m_frameMenus.SetY( -26 );

		m_sprBg2.FinishTweening();
		m_sprBg2.SetDiffuseAlpha( 0 );
		m_sprBg2.Sleep( 0.2f );
		m_sprBg2.SetDiffuseAlpha( 1 );
		m_sprBg2.BeginTweening( 0.1f );
		m_sprBg2.SetDiffuseAlpha( 0 );

		Sprite *pFlash[] = { &m_sprWhiteSmall1, &m_sprWhiteBig };
		for( unsigned i = 0; i < ARRAYLEN(pFlash); ++i )
		{
			Sprite &spr = *pFlash[i];
			spr.StopTweening();
			spr.SetHorizAlign( 0.5f );
			spr.SetX( 0 );
			spr.SetDiffuseAlpha( 0 );
			spr.SetZoom( 1.2f );
			spr.Sleep( 0.2f );
			spr.SetDiffuseAlpha( 1 );
			spr.BeginTweening( 0.1f );
			spr.SetDiffuseAlpha( 0 );
			spr.SetZoom( 1 );
			spr.Sleep( 0 );
		}
		m_sprWhiteSmall2.StopTweening();
		m_sprWhiteSmall2.SetHorizAlign( 0.5f );
		m_sprWhiteSmall2.SetX( 0 );
		m_sprWhiteSmall2.SetRotationZ( 0 );
		m_sprWhiteSmall2.SetZoom( 1 );
	}
	else if( sCommand == "Reset" )
	{
		m_sprTexts.SetState( CurrentMenu().iText );
		m_soundSet.Stop();
		m_soundSet.Play( true );
		m_sprBg2.StopTweening();
		m_sprBg2.SetDiffuse( RageColor(1, 0, 0, 1) );
		m_sprBg2.SetDiffuseAlpha( 0 );
		m_sprBg2.Sleep( 0.05f );
		m_sprBg2.SetDiffuseAlpha( 1 );
		m_sprBg2.BeginTweening( 0.3f );
		m_sprBg2.SetDiffuseAlpha( 0 );
		m_sprBg2.Sleep( 0 );
		m_sprBg2.SetDiffuse( RageColor(1, 1, 1, 0) );

		m_sprArrowsBig.StopTweening();
		m_sprArrowsBig.SetZoom( 1 );
		m_sprArrowsBig.SetDiffuseAlpha( 1 );
		m_sprArrowsBig.BeginTweening( 0.1f );
		m_sprArrowsBig.SetZoom( 1.5f );
		m_sprArrowsBig.SetDiffuseAlpha( 0 );
		m_sprArrowsBig.BeginTweening( 0.15f );
		m_sprArrowsBig.SetZoom( 1 );
		m_sprArrowsBig.SetDiffuseAlpha( 1 );
		m_bArrowsBigPulse = true;

		for( int i = 3; i >= 0; --i )
			m_sprMenuIcon[i].FinishTweening();
		Sprite &center = m_sprMenuIcon[1];
		for( int i = 0; i < 3; ++i )
		{
			center.SetDiffuseAlpha( 1 );
			center.Sleep( 0.05f );
			center.SetDiffuseAlpha( 0 );
			center.Sleep( 0.05f );
		}
		center.SetDiffuseAlpha( 1 );
	}
	else if( sCommand == "GoBack" )
	{
		Blink( m_sprBackArrow1 );
		Blink( m_sprBackArrow2 );
		m_sprWhiteSmall1.StopTweening();
		m_sprWhiteSmall1.SetHorizAlign( 0.5f );
		m_sprWhiteSmall1.SetX( 0 );
	}
	else if( sCommand == "NextMenu" || sCommand == "PrevMenu" )
	{
		m_soundMove.Stop();
		m_soundMove.Play( true );
		m_sprArrowsBig.FinishTweening();
		m_sprArrowsBig.SetZoom( 1 );
		m_sprArrowsBig.SetDiffuseAlpha( 1 );
		m_sprArrowsBig.BeginTweening( 0.075f );
		m_sprArrowsBig.SetZoom( 1.1f );
		m_sprArrowsBig.BeginTweening( 0.075f );
		m_sprArrowsBig.SetZoom( 1 );
		m_bArrowsBigPulse = true;
		m_sprTexts.SetState( CurrentMenu().iText );

		m_sprWhiteBig.StopTweening();
		m_sprWhiteBig.SetZoom( 1 );
		m_sprWhiteBig.SetDiffuseAlpha( 0 );
		m_sprWhiteBig.BeginTweening( 0.125f );
		m_sprWhiteBig.SetDiffuseAlpha( 1 );
		m_sprWhiteBig.BeginTweening( 0.125f );
		m_sprWhiteBig.SetDiffuseAlpha( 0 );
		m_sprWhiteBig.Sleep( 0.1f );

		m_sprWhiteSmall2.StopTweening();
		m_sprWhiteSmall2.SetHorizAlign( 0.5f );
		m_sprWhiteSmall2.SetX( 0 );
		m_sprWhiteSmall2.SetRotationZ( 0 );
		m_sprWhiteSmall2.SetZoom( 1 );
		m_sprWhiteSmall2.SetDiffuseAlpha( 0 );

		SweepWhiteSmall( m_sprWhiteSmall1, sCommand == "NextMenu" ? 70.0f : -70.0f );
		Blink( sCommand == "NextMenu" ? m_sprNextArrow : m_sprPrevArrow );
	}
}

// Recreated from 0x54df34.
void OptionsList::Reset()
{
	bool bStepsTypesChanged = false;
	StepP1Mods::Reset( m_pn, bStepsTypesChanged );
	if( m_pModList != nullptr )
	{
		for( int i = 0; i < 10; ++i )
			m_pModList->Set( i, true, 0 );
	}
	if( m_pLinked != nullptr && m_pLinked->m_bLoaded )
	{
		m_pLinked->Refresh( "Refresh" );
		if( m_pLinked->m_pModList != nullptr )
		{
			m_pLinked->m_pModList->Set( CAT_DISPLAY, false, 0 );
			m_pLinked->m_pModList->Set( CAT_RATE, false, 0 );
			m_pLinked->m_pModList->Set( CAT_STEPSTYPES, false, 0 );
			if( GAMESTATE->m_PlayMode == PLAY_MODE_BATTLE )
				m_pLinked->m_pModList->Set( CAT_BATTLE, false, 0 );
		}
	}
	if( bStepsTypesChanged )
		MESSAGEMAN->Broadcast( Message_StepTypesToShowChanged );
	Animate( "Reset" );
}

// Recreated from 0x54e1b8.
bool OptionsList::Input( const InputEventPlus &input )
{
	if( input.pn != m_pn || input.type != IET_FIRST_PRESS )
		return false;

	switch( input.MenuI )
	{
	case GAME_BUTTON_MENUUP:
	case GAME_BUTTON_MENUDOWN:
	case GAME_BUTTON_SELECT:
		if( !m_bInMenus )
		{
			m_bInMenus = true;
			Refresh( "GoBackSelectingMenu" );
		}
		else
		{
			Close();
		}
		Animate( "GoBack" );
		return true;

	case GAME_BUTTON_MENULEFT:
		if( m_bInMenus )
		{
			if( --m_iMenu < 0 )
				m_iMenu = m_vMenus.size() - 1;
			ScrollMenus( -1 );
			Animate( "PrevMenu" );
			return true;
		}
		if( --m_iItem < 0 )
			m_iItem = max( GetNumItems(CurrentMenu().iCategory) - 1, 0 );
		Refresh( "PrevMod" );
		return true;

	case GAME_BUTTON_MENURIGHT:
		if( m_bInMenus )
		{
			if( ++m_iMenu >= (int)m_vMenus.size() )
				m_iMenu = 0;
			ScrollMenus( +1 );
			Animate( "NextMenu" );
			return true;
		}
		if( ++m_iItem >= GetNumItems(CurrentMenu().iCategory) )
			m_iItem = 0;
		Refresh( "NextMod" );
		return true;

	case GAME_BUTTON_START:
	{
		if( m_bInMenus )
		{
			if( CurrentMenu().iCategory == CAT_RESET )
			{
				Reset();
				return true;
			}
			m_iItem = 0;
			m_bInMenus = false;
			Refresh( "MenuSelected" );
			return true;
		}

		int iCategory = CurrentMenu().iCategory;
		if( !m_bEnabled && iCategory == CAT_STEPSTYPES )
		{
			m_soundBad.Stop();
			m_soundBad.Play( true );
			Message msg( "MenuSttsIsDisabled" );
			msg.SetParam( "pn", m_pn );
			MESSAGEMAN->Broadcast( msg );
			return true;
		}

		bool bOn = true;
		bool bSpecial = false;
		bool bGlobal = IsGlobal( iCategory, m_iItem );
		ApplyToPlayer( m_pn, iCategory, m_iItem, bOn, bSpecial );
		Refresh( "ModSelected" );
		if( bSpecial )
		{
			m_soundBad.Stop();
			m_soundBad.Play( true );
			if( iCategory == CAT_NOTESKIN )
			{
				Message msg( "HiddenNoteskinIsLocked" );
				msg.SetParam( "pn", m_pn );
				MESSAGEMAN->Broadcast( msg );
			}
			return true;
		}
		if( m_pModList != nullptr )
		{
			m_pModList->Set( iCategory, bOn, m_iItem );
			// Speed/AV and display/noteskin affect each other's icons.
			switch( iCategory )
			{
			case CAT_NOTESKIN:	m_pModList->Set( CAT_DISPLAY, false, 0 ); break;
			case CAT_DISPLAY:	m_pModList->Set( CAT_NOTESKIN, false, 0 ); break;
			case CAT_SPEED:		m_pModList->Set( CAT_AV, false, 0 ); break;
			case CAT_AV:		m_pModList->Set( CAT_SPEED, false, 0 ); break;
			}
		}
		// Song-wide options show up on the other player's list too.
		if( bGlobal && m_pLinked != nullptr && m_pLinked->m_bLoaded )
		{
			m_pLinked->Refresh( "Refresh" );
			if( m_pLinked->m_pModList != nullptr )
				m_pLinked->m_pModList->Set( iCategory, bOn, m_iItem );
		}
		m_soundSet.Stop();
		m_soundSet.Play( true );
		return true;
	}

	default:
		return false;
	}
}

// Recreated from 0x54c9e0: looping pulses once each tween ends.
void OptionsList::DrawPrimitives()
{
	if( m_sprArrowsBig.GetTweenTimeLeft() <= 0 && m_bArrowsBigPulse )
	{
		m_sprArrowsBig.SetDiffuseAlpha( 1 );
		m_sprArrowsBig.BeginTweening( 0.5f );
		m_sprArrowsBig.SetDiffuseAlpha( 0.4f );
		m_sprArrowsBig.BeginTweening( 0.5f );
		m_sprArrowsBig.SetDiffuseAlpha( 1 );
	}
	if( m_sprArrowsSm.GetTweenTimeLeft() <= 0 && m_bArrowsSmPulse )
	{
		m_sprArrowsSm.SetZoom( 1 );
		m_sprArrowsSm.SetDiffuseAlpha( 1 );
		m_sprArrowsSm.BeginTweening( 0.2f );
		m_sprArrowsSm.SetZoom( 1.1f );
		m_sprArrowsSm.SetDiffuseAlpha( 0.8f );
		m_sprArrowsSm.BeginTweening( 0.2f );
		m_sprArrowsSm.SetZoom( 1 );
		m_sprArrowsSm.SetDiffuseAlpha( 1 );
	}
	if( m_sprTexts.GetTweenTimeLeft() <= 0 && m_bTextPulse )
	{
		m_bTextPulse = false;
		m_sprTexts.SetState( m_iTextState );
	}
	if( m_sprWhiteSmall1.GetTweenTimeLeft() <= 0 && m_bWhitePulse )
	{
		Sprite *pPulse[] = { &m_sprWhiteSmall1, &m_sprWhiteBig };
		for( unsigned i = 0; i < ARRAYLEN(pPulse); ++i )
		{
			Sprite &spr = *pPulse[i];
			spr.StopTweening();
			spr.SetHorizAlign( 0.5f );
			spr.SetX( 0 );
			spr.SetZoom( 1 );
			spr.SetDiffuseAlpha( 0 );
			spr.BeginTweening( 1 );
			spr.SetDiffuseAlpha( 0.2f );
			spr.BeginTweening( 1 );
			spr.SetDiffuseAlpha( 0 );
		}
		if( !m_bInMenus )
		{
			m_sprWhiteSmall2.StopTweening();
			m_sprWhiteSmall2.SetHorizAlign( 0.5f );
			m_sprWhiteSmall2.SetRotationZ( 0 );
			m_sprWhiteSmall2.SetX( 0 );
			m_sprWhiteSmall2.SetZoom( 1 );
			m_sprWhiteSmall2.SetDiffuseAlpha( 0 );
			m_sprWhiteSmall2.BeginTweening( 1 );
			m_sprWhiteSmall2.SetDiffuseAlpha( 0.2f );
			m_sprWhiteSmall2.BeginTweening( 1 );
			m_sprWhiteSmall2.SetDiffuseAlpha( 0 );
		}
	}
	ActorFrame::DrawPrimitives();
}

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
