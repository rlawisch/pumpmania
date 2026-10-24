#include "global.h"
#include "ModIconsList.h"
#include "StepP1Mods.h"
#include "GameState.h"
#include "PlayerState.h"
#include "SongOptions.h"
#include "ThemeManager.h"
#include "RageFileManager.h"

// Recreated from STEP_P1.exe (ModIconRev at 0x5421e4-0x542b26, ModIconsList at 0x542541-0x542edc).

using namespace StepP1Mods;

ModIconRev::ModIconRev()
{
	m_pn = PLAYER_INVALID;
	m_iCurrent = 0;
	m_iCategory = 12;
	m_bShowPreview = false;
	m_bShowText = false;

	m_sprIcon.Load( THEME->GetPathG("", "OptionList/icons 11x11") );
	m_sprIcon.StopAnimating();
	m_sprIcon.SetBaseZoom( 0.67f );
	m_sprGlow1.Load( THEME->GetPathG("", "OptionList/glow_1") );
	m_sprGlow1.SetDiffuseAlpha( 0 );
	m_sprGlow1.SetBaseZoom( 0.67f );
	m_sprGlow2.Load( THEME->GetPathG("", "OptionList/glow_2") );
	m_sprGlow2.SetDiffuseAlpha( 0 );
	m_sprGlow2.SetBaseZoom( 0.67f );

	m_textValue.LoadFromFont( THEME->GetPathF("", "_arial black 20px") );
	m_textValue.SetZoom( 0.8f );
	m_textValue.SetY( 4 );

	this->AddChild( &m_sprIcon );
	this->AddChild( &m_sprPreview );
	this->AddChild( &m_textValue );
	this->AddChild( &m_sprGlow1 );
	this->AddChild( &m_sprGlow2 );
}

// Recreated from 0x542d59.
void ModIconRev::Init( int iCategory, PlayerNumber pn, bool bAnimate )
{
	if( iCategory >= 11 )
		return;
	m_iCategory = iCategory;
	m_pn = pn;
	Refresh( bAnimate, 0 );
}

// Recreated from 0x542b84.
void ModIconRev::Refresh( bool bAnimate, int iItem )
{
	const PlayerOptions &po = GAMESTATE->m_pPlayerState[m_pn]->m_PlayerOptions.GetPreferred();
	GetActiveIcons( po, m_iCategory, m_vIcons );
	GetAllIcons( m_iCategory, m_vAllIcons );
	if( m_iCategory == CAT_JUDGE )
	{
		// The rate shares the judge icon.
		vector<int> v;
		GetActiveIcons( po, CAT_RATE, v );
		m_vIcons.insert( m_vIcons.end(), v.begin(), v.end() );
		GetAllIcons( CAT_RATE, v );
		m_vAllIcons.insert( m_vAllIcons.end(), v.begin(), v.end() );
	}

	m_iCurrent = 0;
	if( iItem >= 0 && iItem < (int)m_vAllIcons.size() )
	{
		for( unsigned i = 0; i < m_vIcons.size(); ++i )
			if( m_vIcons[i] == m_vAllIcons[iItem] )
				m_iCurrent = i;
	}
	Display();
	if( bAnimate )
		Pulse();
}

// Recreated from 0x542509.
void ModIconRev::NextIcon()
{
	if( m_vIcons.size() < 2 )
		return;
	if( ++m_iCurrent == (int)m_vIcons.size() )
		m_iCurrent = 0;
	Display();
}

// Recreated from 0x5421e4.
void ModIconRev::Display()
{
	m_bShowText = false;
	m_textValue.SetVisible( false );
	m_sprPreview.SetVisible( false );
	m_sprIcon.SetVisible( true );
	if( m_vIcons.empty() )
	{
		SetVisible( false );
		return;
	}
	SetVisible( true );
	m_sprIcon.SetState( m_vIcons[m_iCurrent] );

	const PlayerOptions &po = GAMESTATE->m_pPlayerState[m_pn]->m_PlayerOptions.GetPreferred();
	if( m_iCategory == CAT_NOTESKIN )
	{
		// Noteskins outside icons 11x11 show their NOTE_LOGO.png.
		m_bShowPreview = false;
		const vector<NoteSkinEntry> &v = GetNoteSkins();
		int i = po.m_iNoteSkinIndex;
		if( i >= 0 && i < (int)v.size() && !v[i].bHasIcon )
		{
			RString sLogo = "/NoteSkins/pump/" + v[i].sName + "/NOTE_LOGO.png";
			if( DoesFileExist(sLogo) )
			{
				m_sprPreview.Load( sLogo );
				m_sprPreview.ZoomToWidth( 40 );
				m_sprPreview.ZoomToHeight( 30 );
				m_sprPreview.SetVisible( true );
				m_sprIcon.SetVisible( false );
				m_bShowPreview = true;
			}
		}
	}
	if( m_iCategory == CAT_STEPSTYPES && m_vIcons.size() == 3 )
		SetVisible( false );
	if( m_iCategory == CAT_AV )
	{
		m_bShowText = true;
		m_textValue.SetText( ssprintf("%d", (int)po.m_fMaxScrollBPM) );
		m_textValue.SetVisible( true );
	}
}

// Recreated from 0x54292c.
void ModIconRev::Pulse()
{
	m_sprIcon.StopTweening();
	m_sprIcon.SetZoom( 1.4f );
	m_sprIcon.BeginTweening( 0.1f );
	m_sprIcon.SetZoom( 0.9f );
	m_sprIcon.BeginTweening( 0.6f );
	m_sprIcon.SetZoom( 1 );

	m_sprGlow1.StopTweening();
	m_sprGlow1.SetDiffuseAlpha( 0 );
	m_sprGlow1.Sleep( 0.1f );
	m_sprGlow1.SetDiffuseAlpha( 1 );
	m_sprGlow1.Sleep( 0.1f );
	m_sprGlow1.BeginTweening( 0.5f );
	m_sprGlow1.SetDiffuseAlpha( 0 );

	m_sprGlow2.StopTweening();
	m_sprGlow2.SetDiffuseAlpha( 0 );
	m_sprGlow2.Sleep( 0.1f );
	m_sprGlow2.SetZoom( 0.9f );
	m_sprGlow2.SetDiffuseAlpha( 1 );
	m_sprGlow2.BeginTweening( 0.6f );
	m_sprGlow2.SetDiffuseAlpha( 0 );
	m_sprGlow2.SetZoom( 1.1f );
}

ModIconsList::ModIconsList()
{
	m_pn = PLAYER_INVALID;
	for( int i = 0; i < 10; ++i )
		this->AddChild( &m_Icons[i] );
	// Hidden until the player joins and Load() fills it.
	SetVisible( false );
}

// Recreated from 0x542d88: a column of icons, or two rows when bGrid.
void ModIconsList::Load( PlayerNumber pn, bool bGrid )
{
	m_pn = pn;
	m_Timer.Touch();
	SetVisible( true );
	int iY = 0;
	int iX = 0;
	for( int i = 0; i < 10; ++i )
	{
		m_Icons[i].Init( i, pn, bGrid );
		if( !bGrid )
		{
			// The rate (7) shows inside the judge icon, so it takes no place.
			if( i != CAT_RATE )
				m_Icons[i].SetY( float(iY - (i > 0 ? 37 : 0) - (i >= 8 ? 37 : 0)) );
		}
		else
		{
			if( iY > 0x102 )
				return;
			m_Icons[i].SetZoom( 0.97f );
			int iSide = (pn == PLAYER_1 ? 1 : -1) * -47;
			if( i < 3 )
			{
				m_Icons[i].SetY( -18 );
				m_Icons[i].SetX( float(iX + iSide) );
			}
			else
			{
				m_Icons[i].SetY( 18 );
				m_Icons[i].SetX( float(iX + iSide - 138) );
			}
		}
		iY += 37;
		iX += 46;
		if( iY > 0x171 )
			return;
	}
}

// Recreated from 0x542d1c.
void ModIconsList::Set( int iCategory, bool bAnimate, int iItem )
{
	if( m_pn == PLAYER_INVALID )
		return;
	m_Timer.Touch();
	if( iCategory == CAT_RATE )
	{
		iItem += 8;
		iCategory = CAT_JUDGE;
	}
	if( iCategory < 0 || iCategory >= 10 )
		return;
	m_Icons[iCategory].Refresh( bAnimate, iItem );
}

// Recreated from 0x542541: every 3 seconds each category shows its next icon.
void ModIconsList::Update( float fDeltaTime )
{
	if( m_pn == PLAYER_INVALID )
		return;
	if( m_Timer.Ago() > 3.0f )
	{
		for( int i = 0; i < 10; ++i )
			m_Icons[i].NextIcon();
		m_Timer.Touch();
	}
	ActorFrame::Update( fDeltaTime );
}
