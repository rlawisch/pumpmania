#include "global.h"
#include "WheelBase.h"
#include "RageUtil.h"
#include "GameManager.h"
#include "PrefsManager.h"
#include "ScreenManager.h"	// for sending SM_PlayMusicSample
#include "RageLog.h"
#include "GameConstantsAndTypes.h"
#include "GameState.h"
#include "RageMath.h"
#include "ThemeManager.h"
#include "RageTextureManager.h"
#include "ActorUtil.h"
// Foreach.h removed in SM5.1
#include "Style.h"
#include "ThemeMetric.h"
#include "ScreenDimensions.h"

const int MAX_WHEEL_SOUND_SPEED = 15;
AutoScreenMessage( SM_SongChanged ); // TODO: Replace this with a Message and MESSAGEMAN

static const char *WheelStateNames[] = {
	"Selecting",
	"FlyingOffBeforeNextSort",
	"FlyingOnAfterNextSort",
	"RouletteSpinning",
	"RouletteSlowingDown",
	"RandomSpinning",
	"Locked",
};
XToString( WheelState );
StringToX( WheelState );
LuaXType( WheelState );

WheelBase::~WheelBase()
{
	for( auto i = m_WheelBaseItems.begin(); i != m_WheelBaseItems.end(); ++i )
		SAFE_DELETE( *i );
	m_WheelBaseItems.clear();
	
	m_CurWheelItemData.clear();
	
	wheel_positions.clear();
	wheel_init_positions.clear();
	m_LastSelection = NULL;
}

Actor::TweenState MadeWheelTweenState( float x_pos, float y_pos, float z_pos, float y_rotation, float zoom )
{
	Actor::TweenState m_TS;

	m_TS.Init();
	m_TS.pos.x = x_pos;
	m_TS.pos.y = y_pos;
	m_TS.pos.z = z_pos;
	m_TS.rotation.y = y_rotation;
	m_TS.scale.x = zoom;
	m_TS.scale.y = zoom;

	return m_TS;
}
/*
Actor::TweenState wheel_positions[9] = { 
	MadeWheelTweenState(-360.0f,-85.0f,0.6f), // 0
	MadeWheelTweenState(-285.0f,-85.0f,0.6f), 
	MadeWheelTweenState(-210.0f,-65.0f,0.6f), 
	MadeWheelTweenState(-123.0f,-45.0f,0.6f), 
	MadeWheelTweenState(0.0f,0.0f,0.8f), // 4
	MadeWheelTweenState(123.0f,45.0f,0.6f), 
	MadeWheelTweenState(210.0f,65.0f,0.6f),
	MadeWheelTweenState(285.0f,85.0f,0.6f), 
	MadeWheelTweenState(360.0f,85.0f,0.6f) // 8
};
*/
void WheelBase::Load( RString sType ) 
{
	LOG->Trace( "WheelBase::Load('%s')", sType.c_str() );
	ASSERT( this->GetNumChildren() == 0 ); // only load once

	m_bEmpty = false;
	m_LastSelection = NULL;
	m_iSelection = 0;
	m_fTimeLeftInState = 0;
	m_fPositionOffsetFromSelection = 0;
	m_iSwitchesLeftInSpinDown = 0;
	m_Moving = 0;
	m_iTimesBeforeStartSpin = 1;	//xMAx

	SWITCH_SECONDS.Load(sType,"SwitchSeconds");
	LOCKED_INITIAL_VELOCITY.Load(sType,"LockedInitialVelocity");
	//SCROLL_BAR_HEIGHT.Load(sType,"ScrollBarHeight");
	m_exprItemTransformFunction.SetFromReference( THEME->GetMetricR(sType,"ItemTransformFunction") );
	NUM_WHEEL_ITEMS_TO_DRAW.Load(sType,"NumWheelItems");
	WHEEL_ITEM_LOCKED_COLOR.Load(sType,"WheelItemLockedColor");

	m_soundChangeMusic.Load(THEME->GetPathS(sType,"change"), true);
	m_soundLocked.Load(THEME->GetPathS(sType,"locked"), true);

	WheelItemBase *pTempl = MakeItem();
	ActorUtil::LoadAllCommands( *pTempl, m_sName );
	pTempl->PlayCommand( "Init" );
	for( int i=0; i<NUM_WHEEL_ITEMS; i++ )
	{
		WheelItemBase *pItem = pTempl->Copy();
		DEBUG_ASSERT( pItem );
		m_WheelBaseItems.push_back( pItem );
	}
	SAFE_DELETE( pTempl );

	// float x_pos, float y_pos, float z_pos, float y_rotation, float zoom )
	//FIESTA EX STYLE
/*
	float fXFactor = 1.0f;
	wheel_positions.push_back( MadeWheelTweenState(floor(-635.0f*fXFactor),9,-180,17,1.0f) ); // 0
	wheel_positions.push_back( MadeWheelTweenState(floor(-480.0f*fXFactor),6,-130,18,1.0f) ); 
	wheel_positions.push_back( MadeWheelTweenState(floor(-325.0f*fXFactor),3,-80,19.0f,1.0f) ); 
	wheel_positions.push_back( MadeWheelTweenState(floor(-170.0f*fXFactor),0,-30,20.0f,1.0f) ); 
	wheel_positions.push_back( MadeWheelTweenState(0.0f,0,0,0.0f,1) ); // 4
	wheel_positions.push_back( MadeWheelTweenState(floor(170.0f*fXFactor),0,-30,-20,1.0f) ); 
	wheel_positions.push_back( MadeWheelTweenState(floor(325.0f*fXFactor),3,-80,-19,1.0f) );
	wheel_positions.push_back( MadeWheelTweenState(floor(480.0f*fXFactor),6,-130,-18,1.0f) ); 
	wheel_positions.push_back( MadeWheelTweenState(floor(635.0f*fXFactor),9,-180,-17,1) ); // 8
*/	

	//FIESTA2 STYLE
	/*
	float fXFactor = 1.0f;
	if( (float)PREFSMAN->m_fDisplayAspectRatio >= 1.5f )
		fXFactor = 1.3f;
	
	wheel_positions.push_back( MadeWheelTweenState(floor(-360.0f*fXFactor),0,0,-85.0f,0.6f) ); // 0
	wheel_positions.push_back( MadeWheelTweenState(floor(-285.0f*fXFactor),0,0,-85.0f,0.6f) ); 
	wheel_positions.push_back( MadeWheelTweenState(floor(-210.0f*fXFactor),0,0,-65.0f,0.6f) ); 
	wheel_positions.push_back( MadeWheelTweenState(floor(-123.0f*fXFactor),0,0,-45.0f,0.6f) ); 
	wheel_positions.push_back( MadeWheelTweenState(0.0f,0,0,0.0f,0.8f) ); // 4
	wheel_positions.push_back( MadeWheelTweenState(floor(123.0f*fXFactor),0,0,45.0f,0.6f) ); 
	wheel_positions.push_back( MadeWheelTweenState(floor(210.0f*fXFactor),0,0,65.0f,0.6f) );
	wheel_positions.push_back( MadeWheelTweenState(floor(285.0f*fXFactor),0,0,85.0f,0.6f) ); 
	wheel_positions.push_back( MadeWheelTweenState(floor(360.0f*fXFactor),0,0,85.0f,0.6f) ); // 8
	*/

	//PRIME STYLE
	float fXFactor = 0.55f;
	if( (float)PREFSMAN->m_fDisplayAspectRatio >= 1.5f )
		fXFactor = 2/3.0f;
	
	wheel_positions.push_back( MadeWheelTweenState(floor(-680.0f*fXFactor),0,-4,-120.0f,0.82f) ); // 0
	wheel_positions.push_back( MadeWheelTweenState(floor(-530.0f*fXFactor),0,-3,-100.0f,0.82f) ); 
	wheel_positions.push_back( MadeWheelTweenState(floor(-390.0f*fXFactor),0,-2,-100.0f,0.82f) ); 
	wheel_positions.push_back( MadeWheelTweenState(floor(-280.0f*fXFactor),0,-1,-95.0f,0.82f) ); 
	wheel_positions.push_back( MadeWheelTweenState(0.0f,-25,0.0f,0.0f,1.0f) ); // 4
	wheel_positions.push_back( MadeWheelTweenState(floor(280.0f*fXFactor),0,-1,95.0f,0.82f) ); 
	wheel_positions.push_back( MadeWheelTweenState(floor(390.0f*fXFactor),0,-2,100.0f,0.82f) );
	wheel_positions.push_back( MadeWheelTweenState(floor(530.0f*fXFactor),0,-3,100.0f,0.82f) ); 
	wheel_positions.push_back( MadeWheelTweenState(floor(680.0f*fXFactor),0,-4,120.0f,0.82f) ); // 8
	
	wheel_init_positions.push_back( MadeWheelTweenState(floor(-680.0f*fXFactor),0,-4,-120.0f,0.82f) ); // 0
	wheel_init_positions.push_back( MadeWheelTweenState(floor(-857.0f*fXFactor),-10,-3,-100.0f,0.3f) ); 
	wheel_init_positions.push_back( MadeWheelTweenState(floor(-857.0f*fXFactor),-10,-2,-100.0f,0.3f) ); 
	wheel_init_positions.push_back( MadeWheelTweenState(floor(-762.0f*fXFactor),-10,-1,-95.0f,0.3f) ); 
	wheel_init_positions.push_back( MadeWheelTweenState(0.0f,-25,0.0f,0.0f,0.0f) ); // 4
	wheel_init_positions.push_back( MadeWheelTweenState(floor(762.0f*fXFactor),-10,-1,95.0f,0.3f) ); 
	wheel_init_positions.push_back( MadeWheelTweenState(floor(857.0f*fXFactor),-10,-2,100.0f,0.3f) );
	wheel_init_positions.push_back( MadeWheelTweenState(floor(857.0f*fXFactor),-10,-3,100.0f,0.3f) ); 
	wheel_init_positions.push_back( MadeWheelTweenState(floor(680.0f*fXFactor),-10,-4,120.0f,0.82f) ); // 8
	
	
	// draw outside->inside
	for( int i=0; i<NUM_WHEEL_ITEMS/2; i++ )
		this->AddChild( m_WheelBaseItems[i] );
	for( int i=NUM_WHEEL_ITEMS-1; i>=NUM_WHEEL_ITEMS/2; i-- )
		this->AddChild( m_WheelBaseItems[i] );

/*
	m_sprHighlight.Load( THEME->GetPathG(sType,"highlight") );
	m_sprHighlight->SetName( "Highlight" );
	this->AddChild( m_sprHighlight );
	ActorUtil::LoadAllCommands( *m_sprHighlight, m_sName );

	m_ScrollBar.SetName( "ScrollBar" );
	m_ScrollBar.SetBarHeight( SCROLL_BAR_HEIGHT ); 
	this->AddChild( &m_ScrollBar );
	ActorUtil::LoadAllCommands( m_ScrollBar, m_sName );
*/
	SetPositions();
}

void WheelBase::BeginScreen()
{
	m_WheelState = STATE_SELECTING;
}

//PRIME
void WheelBase::OpenUpWheel( bool bOpenCenter )
{
	//return;

	RebuildWheelItems(0);
	for( int i=0; i<NUM_WHEEL_ITEMS; i++ )
	{
		WheelItemBase *pDisplay = m_WheelBaseItems[i];
		
		if ( i >= 8 || i < 0 )
			return;
		
		pDisplay->FinishTweening();
		pDisplay->DestTweenState() = wheel_positions[i];
		
		if( i==4 && !bOpenCenter )
			continue;
		
		switch( i )
		{
			case 1: pDisplay->Sleep(0); break;
			case 2: pDisplay->Sleep(.1f); break;
			case 3: pDisplay->Sleep(.2f); break;
			case 4: pDisplay->Sleep(.25f); break;
			case 5: pDisplay->Sleep(.2f); break;
			case 6: pDisplay->Sleep(.1f); break;
			case 7: pDisplay->Sleep(0); break;
			default: break;
		}

		if( i == 4 )
		{
			pDisplay->BeginTweening(0.1f, TWEEN_DECELERATE);
			pDisplay->SetY(-45.0f);
			pDisplay->SetDiffuseAlpha(0.0f);
		}
		else
		{
			pDisplay->BeginTweening(0.2f, TWEEN_DECELERATE);
			pDisplay->DestTweenState() = wheel_init_positions[i];
		}
	}
}

//PRIME
void WheelBase::CloseUpWheel( bool bCloseCenter )
{
	//return;

	RebuildWheelItems(0);
	for( int i=0; i<NUM_WHEEL_ITEMS; i++ )
	{
		WheelItemBase *pDisplay = m_WheelBaseItems[i];
		
		if ( i >= 8 || i < 0 )
			return;
		
		pDisplay->FinishTweening();
		
		if( i==4 && !bCloseCenter )
		{
			pDisplay->DestTweenState() = wheel_positions[i];
			continue;
		}
			
		pDisplay->DestTweenState() = wheel_init_positions[i];
		
		switch( i )
		{
			case 1: pDisplay->Sleep(.3f); break;
			case 2: pDisplay->Sleep(.2f); break;
			case 3: pDisplay->Sleep(.1f); break;
			case 4: pDisplay->Sleep(0); break;
			case 5: pDisplay->Sleep(.1f); break;
			case 6: pDisplay->Sleep(.2f); break;
			case 7: pDisplay->Sleep(.3f); break;
			default: break;
		}

		pDisplay->BeginTweening(0.2f, TWEEN_DECELERATE);
		pDisplay->DestTweenState() = wheel_positions[i];
	}
}

void WheelBase::SetItemPosition( Actor &item, float fPosOffsetsFromMiddle )
{
	/* Don't supply and item index or num items. The number of items can be so
	 * large that transforms that depend on such large numbers are likely to break. */
	int iItemIndex = 0; // dummy
	int iNumItems = 1; // dummy

	Actor::TweenState ts = m_exprItemTransformFunction.GetTransformCached( fPosOffsetsFromMiddle, iItemIndex, iNumItems );

	// Round to achieve pixel alignment. Any benefit to moving this to Lua? -Chris
	ts.pos.x = roundf( ts.pos.x );
	ts.pos.y = roundf( ts.pos.y );
	ts.pos.z = roundf( ts.pos.z );

	item.DestTweenState() = ts;
}
/*
void WheelBase::UpdateScrollbar()
{
	int iTotalNumItems = m_CurWheelItemData.size();
	float fItemAt = m_iSelection - m_fPositionOffsetFromSelection;

	{
		float fSize = float(NUM_WHEEL_ITEMS) / iTotalNumItems;
		float fCenter = fItemAt / iTotalNumItems;
		fSize *= 0.5f;

		m_ScrollBar.SetPercentage( fCenter, fSize );
	}
}
*/
bool WheelBase::IsSettled() const
{
	if( m_Moving )
		return false;
	if( m_WheelState != STATE_SELECTING && m_WheelState != STATE_LOCKED )
		return false;
	if( m_fPositionOffsetFromSelection != 0 )
		return false;

	return true;
}
/*
void WheelBase::SetPositions()
{
	for( int i=0; i<NUM_WHEEL_ITEMS; i++ )
	{
		WheelItemBase *pDisplay = m_WheelBaseItems[i];
		const float fOffsetFromSelection = i - NUM_WHEEL_ITEMS/2 + m_fPositionOffsetFromSelection;
		if( fabsf(fOffsetFromSelection) > NUM_WHEEL_ITEMS_TO_DRAW/2 )
			pDisplay->SetVisible( false );
		else
			pDisplay->SetVisible( true );
			
		SetItemPosition( *pDisplay, fOffsetFromSelection );
	}
}
*/

//bool bIsWide = float(PREFSMAN->m_fDisplayAspectRatio) > 1.5f;
//typedef Actor::TweenState TweenState;
/*Actor::TweenState MadeWheelTweenState( float x_pos, float y_rotation, float zoom )
{
	Actor::TweenState m_TS;

	m_TS.Init();
//
	if( bIsWideScreen )
		m_TS.pos.x = x_pos * 1.3f;
	//else
		m_TS.pos.x = x_pos;
		
	m_TS.rotation.y = y_rotation;
	m_TS.scale.x = zoom;
	m_TS.scale.y = zoom;

	return m_TS;
}*/

//bool bIsWideScreen = PREFSMAN->m_fDisplayAspectRatio >= 1.5f;
//vector<Actor::TweenState> wheel_positions;

/*
Actor::TweenState wheel_positions[9] = { 
	MadeWheelTweenState(-360.0f,-85.0f,0.6f), // 0
	MadeWheelTweenState(-285.0f,-85.0f,0.6f), 
	MadeWheelTweenState(-210.0f,-65.0f,0.6f), 
	MadeWheelTweenState(-123.0f,-45.0f,0.6f), 
	MadeWheelTweenState(0.0f,0.0f,0.8f), // 4
	MadeWheelTweenState(123.0f,45.0f,0.6f), 
	MadeWheelTweenState(210.0f,65.0f,0.6f),
	MadeWheelTweenState(285.0f,85.0f,0.6f), 
	MadeWheelTweenState(360.0f,85.0f,0.6f) // 8
};
*/

void WheelBase::SetPositions(int dir)
{
	for( int i=0; i<NUM_WHEEL_ITEMS; i++ )
	{
		WheelItemBase *pDisplay = m_WheelBaseItems[i];

		/*
		const float fOffsetFromSelection = i - NUM_WHEEL_ITEMS/2;
		
		//
		if( fabsf(fOffsetFromSelection+dir) > NUM_WHEEL_ITEMS_TO_DRAW/2 )
			pDisplay->SetVisible( false );
		else
			pDisplay->SetVisible( true );
		//
		
		//SetItemPosition( *pDisplay, fOffsetFromSelection );
		int iItemIndex = 0; // dummy
		int iNumItems = 1; // dummy

		Actor::TweenState ts_i = m_exprItemTransformFunction.GetTransformCached( fOffsetFromSelection+dir, iItemIndex, iNumItems );
		Actor::TweenState ts = m_exprItemTransformFunction.GetTransformCached( fOffsetFromSelection, iItemIndex, iNumItems );

		// Round to achieve pixel alignment. Any benefit to moving this to Lua? -Chris
		ts_i.pos.x = roundf( ts_i.pos.x );
		ts_i.pos.y = roundf( ts_i.pos.y );
		ts_i.pos.z = roundf( ts_i.pos.z );
		
		ts.pos.x = roundf( ts.pos.x );
		ts.pos.y = roundf( ts.pos.y );
		ts.pos.z = roundf( ts.pos.z );
	
		pDisplay->StopTweening();
		pDisplay->DestTweenState() = ts_i;
		
		if( dir == 1 && i==(NUM_WHEEL_ITEMS-1) )	// derecha
		{
			pDisplay->Sleep(.1f);
			pDisplay->BeginTweening(0.3f, TWEEN_LINEAR);
		}
		else if ( dir == -1 && i==2 )
		{
			pDisplay->Sleep(.1f);
			pDisplay->BeginTweening(0.3f, TWEEN_LINEAR);
		}
		else
			pDisplay->BeginTweening(0.4f, TWEEN_LINEAR);
		
		pDisplay->DestTweenState() = ts;
		*/
		
		
		if ( i >= 8 || i < 0 )
			return;
			
		pDisplay->StopTweening();
		
		if ( dir == 1 )
		{
			pDisplay->DestTweenState() = wheel_positions[i+1];
			
			if ( i==(NUM_WHEEL_ITEMS-1) )
			{
				pDisplay->Sleep(.1f);
				pDisplay->BeginTweening(0.2f, TWEEN_LINEAR);
			}
			else
				pDisplay->BeginTweening(0.3f, TWEEN_LINEAR);
				
			pDisplay->DestTweenState() = wheel_positions[i];
		}
		else if ( dir == -1 )
		{
			pDisplay->DestTweenState() = wheel_positions[i];
			
			if ( i==0 )
			{
				pDisplay->Sleep(.1f);
				pDisplay->BeginTweening(0.2f, TWEEN_LINEAR);
			}
			else
				pDisplay->BeginTweening(0.3f, TWEEN_LINEAR);
				
			pDisplay->DestTweenState() = wheel_positions[i+1];
		}	
		else
			pDisplay->DestTweenState() = wheel_positions[i];
		
	}
}

void WheelBase::StopWheel( void )
{
	m_Moving = 0;
	m_fPositionOffsetFromSelection = 0;
	
	for( int i=0; i<NUM_WHEEL_ITEMS; i++ )
	{
		WheelItemBase *pDisplay = m_WheelBaseItems[i];
		pDisplay->FinishTweening();
	}
}

void WheelBase::HandleMessageOver( const Message &msg )	//test - xMAx (ver ScreenSelectMusic.cpp: 2083 )
{
	//if( msg == "StartSelectingSteps" || msg == "GoBackSelectingGroup" )
	if( msg == "GoBackSelectingGroup" )
	{
		StopWheel();	//para setear los banners en sus lugares - xMAx		
		
		for( int i=0; i<NUM_WHEEL_ITEMS; i++ )
		{
			WheelItemBase *pDisplay = m_WheelBaseItems[i];
			Message msg( msg.GetName() + "Animation" );
			msg.SetParam( "NumItem", i );
			
			pDisplay->HandleMessage( msg );
		}
	}
	else if ( msg == "CurrentCategoryChanged" && IsMoving() )
	{
		m_iTimesBeforeStartSpin = 0;
		m_SpinSpeed = float(3);
	}
}

void WheelBase::Update( float fDeltaTime )
{
	ActorFrame::Update( fDeltaTime );

	// If tweens aren't controlling the position of the wheel, set positions.
/*
	if( !GetTweenTimeLeft() )	
		SetPositions();
*/
/*
	for( int i=0; i<NUM_WHEEL_ITEMS; i++ )
	{
		WheelItemBase *pDisplay = m_WheelBaseItems[i];
		if( m_WheelState == STATE_LOCKED  &&  i != NUM_WHEEL_ITEMS/2 )
			pDisplay->m_colorLocked = WHEEL_ITEM_LOCKED_COLOR.GetValue();
		else
			pDisplay->m_colorLocked = RageColor(0,0,0,0);
	}
*/ //xMAx - no es necesario
	// Moved to CommonUpdateProcedure, seems to work fine. Revert if it happens
	// to break something.
	//UpdateScrollbar();

	if( m_Moving )
	{
		m_TimeBeforeMovingBegins -= fDeltaTime;
		m_TimeBeforeMovingBegins = max(m_TimeBeforeMovingBegins, 0);
	}

	// update wheel state
	m_fTimeLeftInState -= fDeltaTime;
	if( m_fTimeLeftInState <= 0 )	// time to go to a new state
		UpdateSwitch();

	if( m_WheelState == STATE_LOCKED )
	{
		/* Do this in at most .1 sec chunks, so we don't get weird if we stop
		 * for some reason (and so it behaves the same when being single stepped). */
		float fTime = fDeltaTime;
		while( fTime > 0 )
		{
			float t = min( fTime, 0.1f );
			fTime -= t;

			m_fPositionOffsetFromSelection = clamp( m_fPositionOffsetFromSelection, -0.3f, +0.3f );

			float fSpringForce = - m_fPositionOffsetFromSelection * LOCKED_INITIAL_VELOCITY;
			m_fLockedWheelVelocity += fSpringForce;

			float fDrag = -m_fLockedWheelVelocity * t*4;
			m_fLockedWheelVelocity += fDrag;

			m_fPositionOffsetFromSelection  += m_fLockedWheelVelocity*t;

			if( fabsf(m_fPositionOffsetFromSelection) < 0.01f  &&  fabsf(m_fLockedWheelVelocity) < 0.01f )
			{
				m_fPositionOffsetFromSelection = 0;
				m_fLockedWheelVelocity = 0;
			}
		}
	}

	
		
	if( IsMoving() )
	{
		// xMAx --- SpinSpeed ------
		switch( m_iTimesBeforeStartSpin )
		{	
			case -1:
				m_SpinSpeed = float((3)*1.06f);
				break;
			case -2:
				m_SpinSpeed = float((3)*1.25f);
				break;
			case -3:
				m_SpinSpeed = float((3)*1.56f);
				break;
			case -4:
				m_SpinSpeed = float((3)*2.0f);
				break;
			case -5:
				m_SpinSpeed = float((3)*2.56f);
				break;
			case -6:
				m_SpinSpeed = float((3)*3.25f);
				break;
			case -7:
				m_SpinSpeed = float((3)*4.06f);
				break;
			case -8:
				m_SpinSpeed = float((3)*6.0f);
				break;
			default: 
				break;
		}	
			
		// We're automatically moving. Move linearly, and don't clamp to the selection.
		//float fSpinSpeed = m_SpinSpeed*m_Moving;
		//temp = m_fPositionOffsetFromSelection;
		
		//m_fPositionOffsetFromSelection -= fSpinSpeed*fDeltaTime;
		m_fPositionOffsetFromSelection -= float(3)*m_Moving*fDeltaTime;
		temp -= m_SpinSpeed*m_Moving*fDeltaTime;

		/* Make sure that we don't go further than 1 away, in case the speed is
		 * very high or we miss a lot of frames. */
		m_fPositionOffsetFromSelection = clamp(m_fPositionOffsetFromSelection, -1.0f, 1.0f);
		temp = clamp(temp, -1.0f, 1.0f);

		// If it passed the selection, move again.
		if((m_Moving == -1 && temp >= 0) ||
		   (m_Moving == 1 && temp <= 0))
		{
			
			ChangeMusic( m_Moving );
			temp = m_Moving;

			// xMAx - Se detiene la resta si la cuenta es menor que -7
			if( m_iTimesBeforeStartSpin > -9 )	
				m_iTimesBeforeStartSpin -= 1;
			/*
			if( 3 < MAX_WHEEL_SOUND_SPEED )
				m_soundChangeMusic.Play(true);
			*/
		}
/*
		if( 3 >= MAX_WHEEL_SOUND_SPEED &&
			m_MovingSoundTimer.PeekDeltaTime() >= 1.0f / MAX_WHEEL_SOUND_SPEED )
		{
			m_MovingSoundTimer.GetDeltaTime();
			m_soundChangeMusic.Play(true);
		}
*/
	}
	else
	{		
		// "rotate" wheel toward selected song
		float fSpinSpeed = 0.2f + fabsf( m_fPositionOffsetFromSelection ) / SWITCH_SECONDS;
		
		if( m_fPositionOffsetFromSelection > 0 )
		{
			m_fPositionOffsetFromSelection -= fSpinSpeed*fDeltaTime;
			if( m_fPositionOffsetFromSelection < 0 )
				m_fPositionOffsetFromSelection = 0;
		}
		else if( m_fPositionOffsetFromSelection < 0 )
		{
			m_fPositionOffsetFromSelection += fSpinSpeed*fDeltaTime;
			if( m_fPositionOffsetFromSelection > 0 )
				m_fPositionOffsetFromSelection = 0;
		}
		
		temp = m_fPositionOffsetFromSelection;
	}
}

void WheelBase::UpdateSwitch()
{
	switch( m_WheelState )
	{
	case STATE_SELECTING:
		m_fTimeLeftInState = 0;
		break;
	case STATE_LOCKED:
		break;
	default:
		FAIL_M(ssprintf("Invalid wheel state: %i", m_WheelState));
	}
}

bool WheelBase::Select()	// return true if this selection can end the screen
{
	LOG->Trace( "WheelBase::Select()" );

	m_Moving = 0;

	if( m_bEmpty )
		return false;

	switch( m_CurWheelItemData[m_iSelection]->m_Type )
	{
	case WheelItemDataType_Generic:
		m_LastSelection = m_CurWheelItemData[m_iSelection];
		break;
	case WheelItemDataType_Section:
		{
			RString sThisItemSectionName = m_CurWheelItemData[m_iSelection]->m_sText;
			if( m_sExpandedSectionName == sThisItemSectionName ) // already expanded
			{
				SetOpenSection( "" ); // collapse it
				m_soundCollapse.Play(true);
			}
			else // already collapsed
			{
				SetOpenSection( sThisItemSectionName ); // expand it
				m_soundExpand.Play(true);
			}
		}
		break;
	default:
		break;
	}

	return true;
}

WheelItemBaseData* WheelBase::GetItem( unsigned int iIndex )
{
	if( !m_bEmpty && iIndex < m_CurWheelItemData.size() )
		return m_CurWheelItemData[iIndex];

	return NULL;
}

int WheelBase::IsMoving() const
{
	return m_Moving && m_TimeBeforeMovingBegins == 0;
}

void WheelBase::TweenOnScreenForSort()
{
	m_fPositionOffsetFromSelection = 0;

	/* Before we send SortOn, position items back to their destinations, so commands
	 * can use this as a reference point. */
	SetPositions();

	m_WheelState = STATE_FLYING_ON_AFTER_NEXT_SORT;

	this->PlayCommand( "SortOn" );

	m_fTimeLeftInState = GetTweenTimeLeft();
}

void WheelBase::TweenOffScreenForSort()
{
	m_WheelState = STATE_FLYING_OFF_BEFORE_NEXT_SORT;

	this->PlayCommand( "SortOff" );

	//m_fTimeLeftInState = GetTweenTimeLeft();
	m_fTimeLeftInState = 0;
}

void WheelBase::ChangeMusicUnlessLocked( int n )
{
	if( m_WheelState == STATE_LOCKED )
	{
		if(n)
		{
			int iSign = n/abs(n);
			m_fLockedWheelVelocity = iSign*LOCKED_INITIAL_VELOCITY;
			m_soundLocked.Play(true);
		}
		return;
	}

	ChangeMusic( n );
}

void WheelBase::Move(int n)
{
	if( n == m_Moving )
		return;

	if( m_WheelState == STATE_LOCKED )
	{
		if(n)
		{
			int iSign = n/abs(n);
			m_fLockedWheelVelocity = iSign*LOCKED_INITIAL_VELOCITY;
			m_soundLocked.Play(true);
		}
		return;
	}

	if (!MoveSpecific(n))
		return;


	m_iTimesBeforeStartSpin = 1;
	m_TimeBeforeMovingBegins = 0.6f;
	m_SpinSpeed = float(3);
	m_Moving = n;

	
	if( m_Moving )
		ChangeMusic(m_Moving);
}

bool WheelBase::MoveSpecific( int n )
{
	/* If we're not selecting, discard this.  We won't ignore it; we'll
	 * get called again every time the key is repeated. */
	/* Still process Move(0) so we sometimes continue moving immediate 
	 * after the sort change finished and before the repeat event causes a 
	 * Move(0). -Chris */
	switch( m_WheelState )
	{
	case STATE_SELECTING:
		break;
	case STATE_FLYING_OFF_BEFORE_NEXT_SORT:
	case STATE_FLYING_ON_AFTER_NEXT_SORT:
		if( n!= 0 )
			return false;
		break;
	default:
		return false;	// don't continue
	}

	if( m_Moving != 0 && n == 0 && m_TimeBeforeMovingBegins == 0 )
	{
		/* We were moving, and now we're stopping.  If we're really close to
		 * the selection, move to the next one, so we have a chance to spin down
		 * smoothly. */
		if(fabsf(m_fPositionOffsetFromSelection) < 0.25f )
			ChangeMusic(m_Moving);

		/* Make sure the user always gets an SM_SongChanged when
		 * Moving() is 0, so the final banner, etc. always gets set. */
		SCREENMAN->PostMessageToTopScreen( SM_SongChanged, 0 );
	}

	return true;
}

void WheelBase::ChangeMusic( int iDist )
{
	m_iSelection += iDist;
	wrap( m_iSelection, m_CurWheelItemData.size() );

	RebuildWheelItems( iDist );

	m_fPositionOffsetFromSelection += iDist;

//	SCREENMAN->PostMessageToTopScreen( SM_SongChanged, 0 );

	/* If we're moving automatically, don't play this; it'll be called in Update. */
	if(!IsMoving())
		m_soundChangeMusic.Play(true);
}

void WheelBase::RebuildWheelItems( int iDist )
{
	const vector<WheelItemBaseData *> &data = m_CurWheelItemData;
	vector<WheelItemBase *> &items = m_WheelBaseItems;

	// rewind to first index that will be displayed;
	int iFirstVisibleIndex = m_iSelection;
	if( m_iSelection > int(data.size()-1) )
		m_iSelection = 0;

	// find the first wheel item shown
	iFirstVisibleIndex -= NUM_WHEEL_ITEMS/2;

	// xMAx - para corregir el moviento a la izquierda
	if( iDist < 0 )
		iFirstVisibleIndex += 1;
	
	ASSERT(data.size() != 0);
	wrap( iFirstVisibleIndex, data.size() );

	// iIndex is now the index of the lowest WheelItem to draw

	int iFirst = 0;
	int iLast = NUM_WHEEL_ITEMS-1;

	if( iDist != INT_MAX )
	{
		// Shift items and refresh only those that have changed.
		CircularShift( items, iDist );
		if( iDist > 0 )
			iFirst = NUM_WHEEL_ITEMS-iDist;
		else if( iDist < 0 )
			iLast = -iDist-1;
	}

	for( int i=iFirst; i <= iLast; i++ )
	{
		int iIndex = iFirstVisibleIndex + i;
		wrap( iIndex, data.size() );

		const WheelItemBaseData *pData = data[iIndex];
		WheelItemBase *pDisplay = items[i];

		pDisplay->SetExpanded( pData->m_Type == WheelItemDataType_Section && pData->m_sText == m_sExpandedSectionName );
	}

	for( int i=0; i<(int)items.size(); i++ )
	{
		int iIndex = iFirstVisibleIndex + i;
		wrap( iIndex, data.size() );
		const WheelItemBaseData *pData = data[iIndex];
		WheelItemBase *pDisplay = items[i];
		pDisplay->LoadFromWheelItemData( pData, iIndex, m_iSelection==iIndex, i );
	}
}

WheelItemBaseData* WheelBase::LastSelected()
{
	if( m_bEmpty )
		return NULL;
	else
		return m_LastSelection;
}

int WheelBase::FirstVisibleIndex()
{
	// rewind to first index that will be displayed;
	int iFirstVisibleIndex = m_iSelection;
	if( m_iSelection >= int(m_CurWheelItemData.size()) )
		m_iSelection = 0;
	
	// find the first wheel item shown
	iFirstVisibleIndex -= NUM_WHEEL_ITEMS/2;

	wrap( iFirstVisibleIndex, m_CurWheelItemData.size() );
	return iFirstVisibleIndex;
}

// lua start
#include "LuaBinding.h"

/** @brief Allow Lua to have access to the WheelBase. */ 
class LunaWheelBase: public Luna<WheelBase>
{
public:
	/*
	static int Move( T* p, lua_State *L ){ p->Move( IArg(1) ); return 0; }
	static int GetWheelItem( T* p, lua_State *L )
	{
		int iItem = IArg(1);

		WheelItemBase *pItem = p->GetWheelItem( iItem );
		if( pItem == NULL )
			luaL_error( L, "%i out of bounds", iItem );
		pItem->PushSelf( L );

		return 1;
	}
	static int IsSettled( T* p, lua_State *L ){ lua_pushboolean( L, p->IsSettled() ); return 1; }
	static int SetOpenSection( T* p, lua_State *L ){ p->SetOpenSection( SArg(1) ); return 0; }
	DEFINE_METHOD( GetSelectedType,		GetSelectedType() )
	DEFINE_METHOD( GetWheelState,		GetWheelState() )
	// deprecated; use GetWheelState instead:
	static int IsLocked( T* p, lua_State *L ){ lua_pushboolean( L, p->WheelIsLocked() ); return 1; }
	*/ // xMAx - removed
	static int GetCurrentIndex( T* p, lua_State *L ){ lua_pushnumber( L, p->GetCurrentIndex() ); return 1; }
	static int GetNumItems( T* p, lua_State *L ){ lua_pushnumber( L, p->GetNumItems() ); return 1; }
	// evil shit
	//static int Move( T* p, lua_State *L ){ p->Move( IArg(1) ); return 0; }
	//static int ChangeMusic( T* p, lua_State *L ){ p->ChangeMusicUnlessLocked( IArg(1) ); return 0; }
	
	LunaWheelBase()
	{
		/*ADD_METHOD( Move );
		ADD_METHOD( GetWheelItem );
		ADD_METHOD( IsSettled );
		ADD_METHOD( IsLocked );
		ADD_METHOD( GetSelectedType );
		ADD_METHOD( SetOpenSection );*/ // xMAx - removed
		ADD_METHOD( GetCurrentIndex );
		ADD_METHOD( GetNumItems );
		// evil shit
		//ADD_METHOD( Move );
		//ADD_METHOD( ChangeMusic );
	}
};

LUA_REGISTER_DERIVED_CLASS( WheelBase, ActorFrame )
// lua end

/*
 * (c) 2001-2004 Chris Danford, Chris Gomez, Glenn Maynard, Josh Allen
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
