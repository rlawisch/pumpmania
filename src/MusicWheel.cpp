#include "global.h"
#include "MusicWheel.h"
#include "RageUtil.h"
#include "SongManager.h"
#include "GameManager.h"
#include "PrefsManager.h"
#include "ScreenManager.h"
#include "RageLog.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "Song.h"
#include "Course.h"
#include "Steps.h"
#include "UnlockManager.h"
#include "GameCommand.h"
#include "ActorUtil.h"
#include "SongUtil.h"
#include "CourseUtil.h"
// Foreach.h removed in SM5.1
#include "Style.h"
#include "PlayerState.h"
#include "CommonMetrics.h"
#include "MessageManager.h"
#include "LocalizedString.h"

static Preference<bool> g_bMoveRandomToEnd			( "MoveRandomToEnd", 		false );
static Preference<bool> g_bPrecacheWheelItems		( "PreCacheWheelItems", 	true );	//xMAx
//static Preference<bool> g_bPrecacheAllSorts		( "PreCacheAllWheelSorts", 	true );	//xMAx
//static Preference<bool> g_bPrecacheAllSongsGroups	( "PreCacheAllSongsGroups", true );	//xMAx

#define NUM_WHEEL_ITEMS					((int)ceil(NUM_WHEEL_ITEMS_TO_DRAW+2))
#define WHEEL_TEXT(s)					THEME->GetString( "MusicWheel", ssprintf("%sText",s.c_str()) );
#define CUSTOM_ITEM_WHEEL_TEXT(s)		THEME->GetString( "MusicWheel", ssprintf("CustomItem%sText",s.c_str()) );

static RString SECTION_COLORS_NAME( size_t i )			{ return ssprintf("SectionColor%d",int(i+1)); }
static RString CHOICE_NAME( RString s )					{ return ssprintf("Choice%s",s.c_str()); }
static RString CUSTOM_WHEEL_ITEM_NAME( RString s )		{ return ssprintf("CustomWheelItem%s",s.c_str()); }
static RString CUSTOM_WHEEL_ITEM_COLOR( RString s )		{ return ssprintf("%sColor",s.c_str()); }

static LocalizedString EMPTY_STRING	( "MusicWheel", "Empty" );

AutoScreenMessage( SM_SongChanged ); // TODO: Replace this with a Message and MESSAGEMAN
AutoScreenMessage( SM_SortOrderChanging );
AutoScreenMessage( SM_SortOrderChanged );

static SortOrder ForceAppropriateSort( PlayMode pm, SortOrder so )
{
	switch( pm )
	{
		// in course modes, force a particular sort
		case PLAY_MODE_ONI:	return SORT_ONI_COURSES;
		case PLAY_MODE_NONSTOP:	return SORT_NONSTOP_COURSES;
		case PLAY_MODE_ENDLESS:	return SORT_ENDLESS_COURSES;
		default: break;
	}

	// If we're not in a course mode, don't start in a course sort.
	switch( so )
	{
		case SORT_ONI_COURSES:
		case SORT_NONSTOP_COURSES:
		case SORT_ENDLESS_COURSES:
			so = SortOrder_Invalid;
		default:
			return so;
	}
}

MusicWheelItem *MusicWheel::MakeItem()
{
	return new MusicWheelItem;
}

void MusicWheel::Load( RString sType ) 
{
	ROULETTE_SWITCH_SECONDS				.Load(sType,"RouletteSwitchSeconds");
	ROULETTE_SLOW_DOWN_SWITCHES			.Load(sType,"RouletteSlowDownSwitches");
	NUM_SECTION_COLORS					.Load(sType,"NumSectionColors");
	SONG_REAL_EXTRA_COLOR				.Load(sType,"SongRealExtraColor");
	SORT_MENU_COLOR						.Load(sType,"SortMenuColor");
	SHOW_ROULETTE						.Load(sType,"ShowRoulette");
	SHOW_RANDOM							.Load(sType,"ShowRandom");
	SHOW_PORTAL							.Load(sType,"ShowPortal");
	RANDOM_PICKS_LOCKED_SONGS			.Load(sType,"RandomPicksLockedSongs");
	MOST_PLAYED_SONGS_TO_SHOW			.Load(sType,"MostPlayedSongsToShow");
	RECENT_SONGS_TO_SHOW				.Load(sType,"RecentSongsToShow");
	MODE_MENU_CHOICE_NAMES				.Load(sType,"ModeMenuChoiceNames");
	SORT_ORDERS							.Load(sType,"SortOrders");
	SHOW_EASY_FLAG						.Load(sType,"UseEasyMarkerFlag");
	USE_SECTIONS_WITH_PREFERRED_GROUP	.Load(sType,"UseSectionsWithPreferredGroup");
	HIDE_INACTIVE_SECTIONS				.Load(sType,"OnlyShowActiveSection");
	REMIND_WHEEL_POSITIONS				.Load(sType,"RemindWheelPositions");
	vector<RString> vsModeChoiceNames;
	split( MODE_MENU_CHOICE_NAMES, ",", vsModeChoiceNames );
	CHOICE								.Load(sType,CHOICE_NAME,vsModeChoiceNames);
	SECTION_COLORS						.Load(sType,SECTION_COLORS_NAME,NUM_SECTION_COLORS);
	CUSTOM_WHEEL_ITEM_NAMES				.Load(sType,"CustomWheelItemNames");
	vector<RString> vsCustomItemNames;
	split( CUSTOM_WHEEL_ITEM_NAMES, ",", vsCustomItemNames );
	CUSTOM_CHOICES						.Load(sType,CUSTOM_WHEEL_ITEM_NAME,vsCustomItemNames);
	CUSTOM_CHOICE_COLORS				.Load(sType,CUSTOM_WHEEL_ITEM_COLOR,vsCustomItemNames);
	ROULETTE_COLOR						.Load(sType,"RouletteColor");
	RANDOM_COLOR						.Load(sType,"RandomColor");
	PORTAL_COLOR						.Load(sType,"PortalColor");
	EMPTY_COLOR							.Load(sType,"EmptyColor");

	WheelBase::Load( sType );

	SONGMAN->UpdateRankingCourses();

	m_soundChangeSort	.Load(	THEME->GetPathS(sType,"sort") );
	m_soundExpand		.Load(	THEME->GetPathS(sType,"expand"), true );
	m_soundCollapse		.Load(	THEME->GetPathS(sType,"collapse"), true );

	// Update for SORT_MOST_PLAYED.
	//SONGMAN->UpdatePopular();
	// xMAx - nope

	/* Sort SONGMAN's songs by CompareSongPointersByTitle, so we can do other sorts (with
	 * stable_sort) from its output, and title will be the secondary sort, without having
	 * to re-sort by title each time. */
	//SONGMAN->SortSongs();
	/* xMAx - No es necesario. El ordenamiento se efectua cuando las canciones se cargan al inicio del juego */

	
	FOREACH_ENUM( SortOrder, so )
	{
		m_WheelItemDatasStatus[so]=INVALID;
	}
	
	// xMAx - added for an empty music wheel after filtered steps type
	dummyMWID = new MusicWheelItemData(WheelItemDataType_Custom, NULL, "DUMMYME", NULL, RageColor(1,1,1,1), 0);
}

MusicWheel::~MusicWheel()
{
	// Release caches items
	ReleaseWheelItems();

	// Delete dummy item
	if( dummyMWID != NULL )
	{
		SAFE_DELETE( dummyMWID );
	}
}

void MusicWheel::ReleaseWheelItems()
{
	FOREACH_ENUM( SortOrder, so ) 
	{
		vector<MusicWheelItemData*> &vMWID = m__WheelItemDatas[so];

		if( vMWID.empty() )
			continue;

		vector<MusicWheelItemData*>::iterator i = vMWID.begin();
		vector<MusicWheelItemData*>::iterator iEnd = vMWID.end();
		for( ; i != iEnd; ++i ) 
			delete *i;

		LOG->Trace("MusicWheel::Deleting sort order data of: %s", (SortOrderToString(so)).c_str() );
		vMWID.clear();
	}

	for( map<RString, vector<MusicWheelItemData *>>::iterator it = m__WheelItemDatasForGroups.begin(); it != m__WheelItemDatasForGroups.end(); ++it )
	{
		vector<MusicWheelItemData*>::iterator i = (it->second).begin();
		vector<MusicWheelItemData*>::iterator iEnd = (it->second).end();
		for( ; i != iEnd; ++i ) 
			delete *i;

		LOG->Trace("MusicWheel::Deleting group data of: %s", (it->first).c_str() );
		(it->second).clear();
	}
	m__WheelItemDatasForGroups.clear();
}

void MusicWheel::BeginScreen()
{
	if( g_bPrecacheWheelItems )
	{
		RageTimer 			timer;
		vector<RString> 	vsAllAvailableChannels;
		
		// xMAx - Obtiene una lista de los canales (grupos de canciones) disponibles según la cantidad de vidas y jugadores
		SONGMAN->GetSongGroupNamesAvailables( vsAllAvailableChannels );
		
		// xMAx - Busca en la lista de canales disponibles cuáles son canales automáticos (sorts) y cuales son canales normales (grupo de canciones)
		for( unsigned i = 0; i < vsAllAvailableChannels.size(); i++ )
		{
			if( vsAllAvailableChannels[i].find("SO_") != std::string::npos || vsAllAvailableChannels[i]== "AllCourses"  )
			{
				SortOrder so = StringToSortOrder( vsAllAvailableChannels[i] );
				if( so != SortOrder_Invalid )
				{				
					readyWheelItemsData(so);
					LOG->Trace( "MusicWheel::Pre-cached sort order: %s", vsAllAvailableChannels[i].c_str() );
				}
			}
			else
			{
				readyWheelChanelItemsData( vsAllAvailableChannels[i] );
				LOG->Trace( "MusicWheel::Pre-cached song group: %s", vsAllAvailableChannels[i].c_str() );
			}
		}

		LOG->Trace( "MusicWheel::MusicWheel precache items took: %s", ssprintf("%.3f ", timer.GetDeltaTime()).c_str() );
	}

	WheelBase::BeginScreen();
}

//
void MusicWheel::CheckCurrentSongOnWheel()
{
	if( SelectSongOrCourse() )
		RebuildWheelItems();
}
//

/* If a song or course is set in GAMESTATE and available, select it.  Otherwise, choose the
 * first available song or course.  Return true if an item was set, false if no items are
 * available. */
bool MusicWheel::SelectSongOrCourse()
{
	if( GAMESTATE->m_pPreferredSong && SelectSong( GAMESTATE->m_pPreferredSong ) )
		return true;
	if( GAMESTATE->m_pCurSong && SelectSong( GAMESTATE->m_pCurSong ) )
		return true;
	if( GAMESTATE->m_pPreferredCourse && SelectCourse( GAMESTATE->m_pPreferredCourse ) )
		return true;
	if( GAMESTATE->m_pCurCourse && SelectCourse( GAMESTATE->m_pCurCourse ) )
		return true;

	// Select the first selectable song based on the sort order...
	/*
	vector<MusicWheelItemData *> &wiWheelItems = getWheelItemsData(GAMESTATE->m_SortOrder);
	for( unsigned i = 0; i < wiWheelItems.size(); i++ )
	{
		if( wiWheelItems[i]->m_pSong )
			return SelectSong( wiWheelItems[i]->m_pSong );
		else if ( wiWheelItems[i]->m_pCourse )
			return SelectCourse( wiWheelItems[i]->m_pCourse );
	}
	*/ //xMAx
	LOG->Trace( "MusicWheel::MusicWheel() - No selectable songs or courses found in WheelData" );
	return false;
}

bool MusicWheel::SelectSection( const RString & SectionName )
{
	for( unsigned int i = 0; i < m_CurWheelItemData.size(); ++i )
	{
		if( m_CurWheelItemData[i]->m_sText == SectionName )
		{
			m_iSelection = i;		// select it
			return true;
		}
	}

	return false;
}

bool MusicWheel::SelectSong( const Song *p )
{
	if( p == NULL )
		return false;

	unsigned i;
	/*
	vector<MusicWheelItemData *> &from = getWheelItemsData(GAMESTATE->m_SortOrder);
	for( i=0; i<from.size(); i++ )
	{
		if( from[i]->m_pSong == p )
		{
			// make its group the currently expanded group
			SetOpenSection( from[i]->m_sText );
			break;
		}
	}

	if( i == from.size() )
		return false;
	

	for( i=0; i<m_CurWheelItemData.size(); i++ )
	{
		if( GetCurWheelItemData(i)->m_pSong == p )
			m_iSelection = i;		// select it
	}
	return true;
	*/ //xMAx
	
	for( i=0; i<m_CurWheelItemData.size(); i++ )
	{
		if( GetCurWheelItemData(i)->m_pSong == p )
		{
			m_iSelection = i;		// select it
			return true;
		}
	}
	
	return false;
}

bool MusicWheel::SelectCourse( const Course *p )
{
	if( p == NULL )
		return false;

	unsigned i;
	vector<MusicWheelItemData *> from;
	getWheelItemsData(GAMESTATE->m_SortOrder,from);
	for( i=0; i<from.size(); i++ )
	{
		if( from[i]->m_pCourse == p )
		{
			// make its group the currently expanded group
			SetOpenSection( from[i]->m_sText );
			break;
		}
	}

	if( i == from.size() )
		return false;

	for( i=0; i<m_CurWheelItemData.size(); i++ )
	{
		if( GetCurWheelItemData(i)->m_pCourse == p )
			m_iSelection = i; // select it
	}

	return true;
}

bool MusicWheel::SelectModeMenuItem()
{
	// Select the last-chosen option.
	ASSERT( GAMESTATE->m_SortOrder == SORT_MODE_MENU );
	vector<MusicWheelItemData *> from;
	getWheelItemsData(GAMESTATE->m_SortOrder,from);
	unsigned i;
	for( i=0; i<from.size(); i++ )
	{
		const GameCommand &gc = *from[i]->m_pAction;
		if( gc.m_sName == m_sLastModeMenuItem )
			break;
	}
	if( i == from.size() )
		return false;

	// make its group the currently expanded group
	SetOpenSection( from[i]->m_sText );

	for( i=0; i<m_CurWheelItemData.size(); i++ )
	{
		if( GetCurWheelItemData(i)->m_pAction->m_sName != m_sLastModeMenuItem )
			continue;
		m_iSelection = i;		// select it
		break;
	}

	return true;
}

// bool MusicWheel::SelectCustomItem()

//void MusicWheel::GetSongList( vector<Song*> &arraySongs, SortOrder so )
void MusicWheel::GetSongList( vector<Song*> &arraySongs, SortOrder so, RString SongGroup )
{
	vector<Song*> apAllSongs;
	
	switch( so )
	{
		/*case SORT_PREFERRED:
			SONGMAN->GetPreferredSortSongs( apAllSongs );
			break;
		case SORT_POPULARITY:
			apAllSongs = SONGMAN->GetPopularSongs();
			break;*/
		case SORT_FULLSONG:	
			apAllSongs = SONGMAN->GetFullSongs();
			break;
		case SORT_UCS:	
			apAllSongs = SONGMAN->GetUCS();
			break;
		case SORT_QUEST:	
			apAllSongs = SONGMAN->GetQuestSongs();
			break;
		case SORT_REMIX:
			apAllSongs = SONGMAN->GetRemixSongs();
			break;
		case SORT_SHORTCUT:
			apAllSongs = SONGMAN->GetShortCutSongs();
			break;
		case SORT_ALLTUNES:
			apAllSongs = SONGMAN->GetAllTunes();
			break;
		case SORT_ORIGINAL:
			apAllSongs = SONGMAN->GetOriginalSongs();
			break;
		case SORT_KPOP:
			apAllSongs = SONGMAN->GetKpopSongs();
			break;	
		case SORT_WORLDMUSIC:
			apAllSongs = SONGMAN->GetWorldMusicSongs();
			break;
		case SORT_COOP:
			apAllSongs = SONGMAN->GetCoOpPlaySongs();
			break;
		case SORT_RANDOM:
			apAllSongs = SONGMAN->GetRandomSongs();
			break;
		case SORT_JMUSIC:
			apAllSongs = SONGMAN->GetJMusicSongs();
			break;
		case SORT_LEVEL_1:{	vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[0]).m_pAllStyles; }; break;
		case SORT_LEVEL_2:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[1]).m_pAllStyles; }; break;
		case SORT_LEVEL_3:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[2]).m_pAllStyles; }; break;
		case SORT_LEVEL_4:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[3]).m_pAllStyles; }; break;
		case SORT_LEVEL_5:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[4]).m_pAllStyles; }; break;
		case SORT_LEVEL_6:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[5]).m_pAllStyles; }; break;
		case SORT_LEVEL_7:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[6]).m_pAllStyles; }; break;
		case SORT_LEVEL_8:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[7]).m_pAllStyles; }; break;
		case SORT_LEVEL_9:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[8]).m_pAllStyles; }; break;
		case SORT_LEVEL_10:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[9]).m_pAllStyles; }; break;
		case SORT_LEVEL_11:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[10]).m_pAllStyles; }; break;
		case SORT_LEVEL_12:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[11]).m_pAllStyles; }; break;
		case SORT_LEVEL_13:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[12]).m_pAllStyles; }; break;
		case SORT_LEVEL_14:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[13]).m_pAllStyles; }; break;
		case SORT_LEVEL_15:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[14]).m_pAllStyles; }; break;
		case SORT_LEVEL_16:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[15]).m_pAllStyles; }; break;
		case SORT_LEVEL_17:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[16]).m_pAllStyles; }; break;
		case SORT_LEVEL_18:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[17]).m_pAllStyles; }; break;
		case SORT_LEVEL_19:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[18]).m_pAllStyles; }; break;
		case SORT_LEVEL_20:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[19]).m_pAllStyles; }; break;
		case SORT_LEVEL_21:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[20]).m_pAllStyles; }; break;
		case SORT_LEVEL_22:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[21]).m_pAllStyles; }; break;
		case SORT_LEVEL_23:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[22]).m_pAllStyles; }; break;
		case SORT_LEVEL_24_OVER:{ vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[23]).m_pAllStyles; }; break;
		//----------------------------------------------------------------------------------
		case SORT_GROUP:
			{
				// xMAxGetRandomSongs
				if( SongGroup != "" )
				{
					apAllSongs = SONGMAN->GetSongs( SongGroup );
					break;
				}
				// if we're not using sections with a preferred song group, and there
				// is a group to load, only load those songs. -aj
				//if(GAMESTATE->m_sPreferredSongGroup != GROUP_ALL && !USE_SECTIONS_WITH_PREFERRED_GROUP )
				if( GAMESTATE->sExpandedSectionName != "" )
				{
					//apAllSongs = SONGMAN->GetSongs(GAMESTATE->m_sPreferredSongGroup);
					apAllSongs = SONGMAN->GetSongs(GAMESTATE->sExpandedSectionName);
					break;
				}
			} // otherwise fall through
		default:
			apAllSongs = SONGMAN->GetAllSongs();
			break;
	}

	if( apAllSongs.empty() )
		return;
		
	// filter songs that we don't have enough stages to play
	vector<Song*> vFilteredSongs;
	SongCriteria sc;
	// sc.m_iMaxStagesForSong = GAMESTATE->GetSmallestNumStagesLeftForAnyHumanPlayer(); 	//xMAx
	sc.m_iMaxStagesForSong = GAMESTATE->GetHighestNumStagesLeftForAnyHumanPlayer();
	SongUtil::FilterSongs( sc, apAllSongs, vFilteredSongs );
	apAllSongs.clear();

	// copy only songs that have at least one Steps for the current GameMode
	for( unsigned i=0; i<vFilteredSongs.size(); i++ )
	{
		Song* pSong = vFilteredSongs[i];

		vector<Steps*>	m_vpSteps;
		SongUtil::GetPlayableSteps( pSong, m_vpSteps, so );
		if( m_vpSteps.empty() )
		{
			continue;
		}
		
		arraySongs.push_back( pSong );
	}
}

void MusicWheel::BuildWheelItemDatas( vector<MusicWheelItemData *> &arrayWheelItemDatas, SortOrder so, RString SongGroup )
{
	switch( so )
	{
		case SORT_GROUP:
		case SORT_FULLSONG:
		case SORT_UCS:
		case SORT_QUEST:
		case SORT_REMIX:
		case SORT_SHORTCUT:
		case SORT_ORIGINAL:
		case SORT_KPOP:
		case SORT_WORLDMUSIC:
		case SORT_COOP:
		case SORT_RANDOM:
		case SORT_JMUSIC:
		case SORT_LEVEL_1:
		case SORT_LEVEL_2:
		case SORT_LEVEL_3:
		case SORT_LEVEL_4:
		case SORT_LEVEL_5:
		case SORT_LEVEL_6:
		case SORT_LEVEL_7:
		case SORT_LEVEL_8:
		case SORT_LEVEL_9:
		case SORT_LEVEL_10:
		case SORT_LEVEL_11:
		case SORT_LEVEL_12:
		case SORT_LEVEL_13:
		case SORT_LEVEL_14:
		case SORT_LEVEL_15:
		case SORT_LEVEL_16:
		case SORT_LEVEL_17:
		case SORT_LEVEL_18:
		case SORT_LEVEL_19:
		case SORT_LEVEL_20:
		case SORT_LEVEL_21:
		case SORT_LEVEL_22:
		case SORT_LEVEL_23:
		case SORT_LEVEL_24_OVER:
		case SORT_ALLTUNES:
		{
			vector<Song*> arraySongs;
			
			// SORT_GROUP only can get an array of song if we have a non emoty SongGroup
			if( so == SORT_GROUP )
			{
				if( SongGroup != "" )
				{
					GetSongList( arraySongs, so, SongGroup );
				}
			}
			else
			{
				GetSongList( arraySongs, so, SongGroup );
			}	

			if( arraySongs.empty() )
				return;
			
			arrayWheelItemDatas.clear();
			arrayWheelItemDatas.reserve( arraySongs.size() );

			for( unsigned i=0; i< arraySongs.size(); i++ )
			{
				Song* pSong = arraySongs[i];
				arrayWheelItemDatas.push_back( new MusicWheelItemData(WheelItemDataType_Song, pSong, "", NULL, RageColor(1,1,1,1), 0) );
			}
		}; break;
		case SORT_ALL_COURSES:
		case SORT_NONSTOP_COURSES:
		case SORT_ONI_COURSES:
		case SORT_ENDLESS_COURSES:
		{
			//bool bOnlyPreferred = PREFSMAN->m_CourseSortOrder == COURSE_SORT_PREFERRED;
			bool bOnlyPreferred = false;

			vector<CourseType> vct;
			switch( so )
			{
			case SORT_NONSTOP_COURSES:
				vct.push_back( COURSE_TYPE_NONSTOP );
				break;
			case SORT_ONI_COURSES:
				vct.push_back( COURSE_TYPE_ONI );
				vct.push_back( COURSE_TYPE_SURVIVAL );
				break;
			case SORT_ENDLESS_COURSES:
				vct.push_back( COURSE_TYPE_ENDLESS );
				break;
			case SORT_ALL_COURSES:
				FOREACH_ENUM( CourseType, i )
					vct.push_back( i );
				break;
			default:
				FAIL_M(ssprintf("Wrong sort order: %i", so));
			}

			vector<Course*> apCourses;
			for( auto ct = vct.begin(); ct != vct.end(); ++ct )
			{
				if( bOnlyPreferred )
					SONGMAN->GetPreferredSortCourses( *ct, apCourses, PREFSMAN->m_bAutogenGroupCourses );
				else
					SONGMAN->GetCourses( *ct, apCourses, PREFSMAN->m_bAutogenGroupCourses );
			}

			LOG->Trace("xMAx::We have %d courses",apCourses.size());
			
			switch( PREFSMAN->m_CourseSortOrder )
			{
				case COURSE_SORT_SONGS:
					CourseUtil::SortCoursePointerArrayByDifficulty( apCourses );
					break;
				case COURSE_SORT_PREFERRED:
					break;
				case COURSE_SORT_METER:
					CourseUtil::SortCoursePointerArrayByAvgDifficulty( apCourses );
					break;
				case COURSE_SORT_METER_SUM:
					CourseUtil::SortCoursePointerArrayByTotalDifficulty( apCourses );
					break;
				case COURSE_SORT_RANK:
					CourseUtil::SortCoursePointerArrayByRanking( apCourses );
					break;
				default:	FAIL_M("Impossible to sort the courses! Aborting...");
			}

			// since we can't agree, make it an option
			if( PREFSMAN->m_CourseSortOrder != COURSE_SORT_SONGS && g_bMoveRandomToEnd )
				CourseUtil::MoveRandomToEnd( apCourses );

			if( so == SORT_ALL_COURSES )
				CourseUtil::SortCoursePointerArrayByType( apCourses );

			arrayWheelItemDatas.clear();	// clear out the previous wheel items 

			RString sLastSection = "";
			for( unsigned i=0; i<apCourses.size(); i++ )	// foreach course
			{
				Course* pCourse = apCourses[i];

				// if unlocks are on, make sure it is unlocked
				if ( UNLOCKMAN->CourseIsLocked(pCourse) )
					continue;

				RString sThisSection = "";
				if( so == SORT_ALL_COURSES )
				{
					switch( pCourse->GetPlayMode() )
					{
						case PLAY_MODE_ONI:	sThisSection = "Oni";		break;
						case PLAY_MODE_NONSTOP:	sThisSection = "Nonstop";	break;
						case PLAY_MODE_ENDLESS:	sThisSection = "Endless";	break;
						default: break;
					}
				}

				// check that this course has at least one song playable in the current style
				if( !pCourse->IsPlayableIn(GAMESTATE->GetCurrentStyle()->m_StepsType) )
					continue;

				arrayWheelItemDatas.push_back( new MusicWheelItemData(WheelItemDataType_Course, NULL, sThisSection, pCourse, RageColor(1,1,1,1), 0) );
			}
			
			LOG->Trace("xMAx::We have %d courses on the wheel",arrayWheelItemDatas.size());
			
		}; break;
		default: break;
	}
}

// xMAx - Use this to get items for SORTs
//vector<MusicWheelItemData *> & MusicWheel::getWheelItemsData(SortOrder so) 
void MusicWheel::getWheelItemsData(SortOrder so,vector<MusicWheelItemData *> &out) 
{
	readyWheelItemsData(so);
	
	out = m__WheelItemDatas[so];
	
	// Aplicar el filtro de items
	//FilterItems( m__WheelItemDatas[so], out, so );
}

void MusicWheel::readyWheelItemsData(SortOrder so) 
{
	if( m__WheelItemDatas[so].empty() ) 
	{
		vector<MusicWheelItemData *> aUnFilteredDatas;
		BuildWheelItemDatas( aUnFilteredDatas, so );
		m__WheelItemDatas[so] = aUnFilteredDatas;
	}
}

// xMAx - use this to get items for normal Channels (group of songs)
void MusicWheel::getWheelChannelItemsData(RString group, vector<MusicWheelItemData *> &out)
{
	// xMAx - check if the channel that we're trying to open is available (it should be since we got the name from the available channels list)
	//ASSERT_M( SONGMAN->IsGroupAvailable(group), ssprintf("MusicWheel::Group %s is not available to be selected", group.c_str()) );
	ASSERT_M( SONGMAN->DoesSongGroupExist(group), ssprintf("MusicWheel::Group %s doesn't exists", group.c_str()) );

	// xMAx - we have to check if it's available or not since we're going to include it to the wheel items map
	readyWheelChanelItemsData(group);
	
	// Return the items list
	map<RString,vector<MusicWheelItemData*>>::iterator it = m__WheelItemDatasForGroups.find( group );
	ASSERT_M( it != m__WheelItemDatasForGroups.end(), ssprintf("MusicWheel::Could't find group %s in the precached groups list", group.c_str()) );
	
	out = it->second;
	// Aplicar el filtro de items
	//FilterItems( it->second, out, SORT_GROUP );
}

void MusicWheel::readyWheelChanelItemsData(RString group)
{
	map<RString,vector<MusicWheelItemData*>>::iterator it = m__WheelItemDatasForGroups.find( group );
	
	// Si no se encuentra el item hay que crearlo y precargarlo
	if( it == m__WheelItemDatasForGroups.end() )
	{
		vector<MusicWheelItemData *> aUnFilteredDatas;
		BuildWheelItemDatas( aUnFilteredDatas, SORT_GROUP, group );
		m__WheelItemDatasForGroups[group] = aUnFilteredDatas;
	}
}

void MusicWheel::FilterItems( vector<MusicWheelItemData *> arrayWheelItemDatas, vector<MusicWheelItemData *> &out, SortOrder so )
{
	out.clear();
	
	if( !GAMESTATE->m_SongOptions.GetCurrent().m_bShowSingles 		||
		!GAMESTATE->m_SongOptions.GetCurrent().m_bShowHalfDoubles 	||
		!GAMESTATE->m_SongOptions.GetCurrent().m_bShowDoubles )
	 {
		for( unsigned i = 0; i < arrayWheelItemDatas.size(); ++i )
		{
			MusicWheelItemData &d = *arrayWheelItemDatas[i];
			Song* tempSong = d.m_pSong;
			vector<Steps*> m_vpSteps;
			SongUtil::GetFilteredPlayableSteps( tempSong, m_vpSteps, so );
			
			if( m_vpSteps.size() > 0 )
			{
				out.push_back(arrayWheelItemDatas[i]);
			}
		}
		
		if( out.empty() )
		{
			out.push_back( dummyMWID );
		}
	}
	else
	{
		for( unsigned i = 0; i < arrayWheelItemDatas.size(); ++i )
		{
			out.push_back(arrayWheelItemDatas[i]);
		}
	}
}

void MusicWheel::UpdateSwitch()
{
	switch( m_WheelState )
	{
	case STATE_FLYING_OFF_BEFORE_NEXT_SORT:
		{
			const Song* pPrevSelectedSong = GetCurWheelItemData(m_iSelection)->m_pSong;

			SCREENMAN->PostMessageToTopScreen( SM_SortOrderChanged, 0 );

			SetOpenSection( SongUtil::GetSectionNameFromSongAndSort(pPrevSelectedSong, GAMESTATE->m_SortOrder) );

			m_iSelection = 0;

			// Select the previously selected item
			switch( GAMESTATE->m_SortOrder )
			{
			default:
				// Look for the last selected song or course
				SelectSongOrCourse();
				break;
			case SORT_MODE_MENU:
				SelectModeMenuItem();
				break;
			}

			// Change difficulty for sorts by meter
			// XXX: do this with GameCommand?
			StepsType st;
			Difficulty dc;
			if( SongUtil::GetStepsTypeAndDifficultyFromSortOrder( GAMESTATE->m_SortOrder, st, dc ) )
			{
				ASSERT( dc != Difficulty_Invalid );
				FOREACH_PlayerNumber( p )
					if( GAMESTATE->IsPlayerEnabled(p) )
						GAMESTATE->m_PreferredDifficulty[p].Set( dc );
			}

			SCREENMAN->PostMessageToTopScreen( SM_SongChanged, 0 );
			RebuildWheelItems();
			TweenOnScreenForSort();
		}
		break;

	case STATE_FLYING_ON_AFTER_NEXT_SORT:
		m_WheelState = STATE_SELECTING;	// now, wait for input
		break;

	case STATE_SELECTING:
		m_fTimeLeftInState = 0;
		break;
	case STATE_ROULETTE_SPINNING:
	case STATE_RANDOM_SPINNING:
		break;
	case STATE_LOCKED:
		break;
	case STATE_ROULETTE_SLOWING_DOWN:
		if( m_iSwitchesLeftInSpinDown == 0 )
		{
			if( PREFSMAN->m_bLockWheelAfterRoulette )
			{
				m_WheelState = STATE_LOCKED;
			}
			else
			{
				m_WheelState = STATE_SELECTING;
			}
			m_fTimeLeftInState = 0;
			SCREENMAN->PlayStartSound();
			m_fLockedWheelVelocity = 0;

			// xMAx
			if( PREFSMAN->m_bLockWheelAfterRoulette )
			{
				vector<WheelItemBase *> &items = m_WheelBaseItems;
				for( unsigned i=0; i<(int)items.size(); i++ )
				{
					if( bLastRouletterMoveWasToTheRight )
					{
						if( i == (items.size()/2) )
							continue;
					}
					else
					{
						if( i == (items.size()/2 - 1) )
							continue;
					}
					
					WheelItemBase *pDisplay = items[i];
					pDisplay->SetDiffuse(RageColor(.2f,.2f,.2f,1));
				}
			}
			
			// Send this again so the screen starts sample music.
			SCREENMAN->PostMessageToTopScreen( SM_SongChanged, 0 );
			MESSAGEMAN->Broadcast("RouletteStopped");
		}
		else
		{
			--m_iSwitchesLeftInSpinDown;
			const float SwitchTimes[] = { 0.5f, 1.3f, 0.8f, 0.4f, 0.2f };
			ASSERT( m_iSwitchesLeftInSpinDown >= 0 && m_iSwitchesLeftInSpinDown <= 4 );
			m_fTimeLeftInState = SwitchTimes[m_iSwitchesLeftInSpinDown];
			m_Moving = 0;

			LOG->Trace( "m_iSwitchesLeftInSpinDown id %d, m_fTimeLeftInState is %f", m_iSwitchesLeftInSpinDown, m_fTimeLeftInState );

			if( m_iSwitchesLeftInSpinDown == 0 )
			{
				//ChangeMusic( randomf(0,1) >= 0.5f? 1:-1 );
				//xMAx
				int lastMove = randomf(0,1) >= 0.5f? 1:-1;
				bLastRouletterMoveWasToTheRight = lastMove > 0;
				ChangeMusic( lastMove );
			}
			else
				ChangeMusic( 1 );
		}
		break;
	default:
		FAIL_M(ssprintf("Invalid wheel state: %i", m_WheelState));
	}
}

void MusicWheel::ChangeMusic( int iDist )
{
	m_iSelection += iDist;
	wrap( m_iSelection, m_CurWheelItemData.size() );

	if( REMIND_WHEEL_POSITIONS && HIDE_INACTIVE_SECTIONS )
	{
		// store the group song index
		for( unsigned idx = 0 ; idx < m_viWheelPositions.size() ; idx++ )
		{
			if( m_sExpandedSectionName == SONGMAN->GetSongGroupByIndex(idx) )
			{
				m_viWheelPositions[idx] = m_iSelection;
			}
		}
	}

	RebuildWheelItems( iDist );

	//m_fPositionOffsetFromSelection += iDist;
	m_fPositionOffsetFromSelection = iDist;
	SetPositions(iDist);
	
	SCREENMAN->PostMessageToTopScreen( SM_SongChanged, 0 );
	
	// xMAx - Mensajes de Next y Previous Song
	if( iDist > 0 )	
		MESSAGEMAN->Broadcast( "NextSong" );
	else
		MESSAGEMAN->Broadcast( "PreviousSong" );

	// If we're moving automatically, don't play this; it'll be called in Update.
	//if(!IsMoving()) //xMAx - Se reproduce el sonido sin condición
	if( m_soundChangeMusic.IsPlaying() )
	{
		m_soundChangeMusic.Stop();
	}
	
	m_soundChangeMusic.Play(true);
}


void MusicWheel::OpenGroupOrChangeSort( RString sChannel )
{
	// xMAx
	SortOrder SO = StringToSortOrder( sChannel );
	if( SO != SortOrder_Invalid && ( sChannel.find("SO_") != std::string::npos || sChannel == "AllCourses" ) )
	{
		ChangeSort( SO );
		return;
	}

	if( SONGMAN->DoesSongGroupExist( sChannel ) )
	{
		GAMESTATE->m_SortOrder.Set( SORT_GROUP );
		GAMESTATE->m_PreferredSortOrder = SORT_GROUP;
		GAMESTATE->m_sCurrentGroupName = sChannel;	//esto se está realizando en SSM tambien - xMAx 
		SetOpenSection( sChannel );
	}
}

bool MusicWheel::ChangeSort( SortOrder new_so, bool allowSameSort )	// return true if change successful
{
	ASSERT_M( new_so < NUM_SortOrder, "MusicWheel::Invalid sort order" );
	
	if( IsSongSort(new_so) || new_so == SORT_ALL_COURSES )
	{
		GAMESTATE->m_PreferredSortOrder = new_so;
		GAMESTATE->m_SortOrder.Set( new_so );
	}
	else
		return false;
		
	
	// Clears current wheel items data
	m_CurWheelItemData.clear();
	
	// Gets new items data according to the selected sort order
	vector<MusicWheelItemData *> from;
	getWheelItemsData( new_so, from );
	m_CurWheelItemData.reserve( from.size() );
	for( unsigned i = 0; i < from.size(); ++i )
	{
		m_CurWheelItemData.push_back(from[i]);
	}

	// Restarts selection
	m_iSelection = 0;
	
	// Rebuilds items
	RebuildWheelItems();
	
	// Set the position of each item on the wheel (no moving)
	SetPositions();

	// Sends message that sort order changed
	SCREENMAN->PostMessageToTopScreen( SM_SortOrderChanged, 0 );
	
	return true;
}

bool MusicWheel::NextSort()		// return true if change successful
{
	// don't allow NextSort when on the mode menu
	if( GAMESTATE->m_SortOrder == SORT_MODE_MENU )
		return false;

	vector<SortOrder> aSortOrders;
	{
		Lua *L = LUA->Get();
		SORT_ORDERS.PushSelf( L );
		FOREACH_LUATABLEI( L, -1, i )
		{
			SortOrder so = Enum::Check<SortOrder>( L, -1, true );
			aSortOrders.push_back( so );
		}
		lua_pop( L, 1 );
		LUA->Release(L);
	}

	// find the index of the current sort
	int cur = 0;
	while( cur < int(aSortOrders.size()) && aSortOrders[cur] != GAMESTATE->m_SortOrder )
		++cur;

	// move to the next sort with wrapping
	++cur;
	wrap( cur, aSortOrders.size() );

	// apply new sort
	SortOrder soNew = aSortOrders[cur];
	return ChangeSort( soNew );
}

bool MusicWheel::Select()	// return true if this selection ends the screen
{
	LOG->Trace( "MusicWheel::Select()" );

	switch( m_WheelState )
	{
		case STATE_FLYING_OFF_BEFORE_NEXT_SORT:
		case STATE_ROULETTE_SLOWING_DOWN:
			return false;
		case STATE_ROULETTE_SPINNING:
			MESSAGEMAN->Broadcast("SlowingDownRoulette");
			m_WheelState = STATE_ROULETTE_SLOWING_DOWN;
			m_iSwitchesLeftInSpinDown = ROULETTE_SLOW_DOWN_SWITCHES/2+1 + RandomInt( ROULETTE_SLOW_DOWN_SWITCHES/2 );
			m_fTimeLeftInState = 0.1f;
			return false;
		case STATE_RANDOM_SPINNING:
			m_fPositionOffsetFromSelection = max(m_fPositionOffsetFromSelection, 0.3f);
			m_WheelState = STATE_LOCKED;
			SCREENMAN->PlayStartSound();
			m_fLockedWheelVelocity = 0;
			// Set m_Moving to zero to stop the sounds from playing.
			m_Moving = 0;
			SCREENMAN->PostMessageToTopScreen( SM_SongChanged, 0 );
			return true;
		default: break;
	}

	if( !WheelBase::Select() )
		return false;
/*
	switch( m_CurWheelItemData[m_iSelection]->m_Type )
	{
		case WheelItemDataType_Roulette:  
			StartRoulette();
			return false;
		case WheelItemDataType_Random:
			StartRandom();
			return false;
		case WheelItemDataType_Sort:
			GetCurWheelItemData(m_iSelection)->m_pAction->ApplyToAllPlayers();
			ChangeSort( GAMESTATE->m_PreferredSortOrder );
			m_sLastModeMenuItem = GetCurWheelItemData(m_iSelection)->m_pAction->m_sName;
			return false;
		case WheelItemDataType_Custom:
			GetCurWheelItemData(m_iSelection)->m_pAction->ApplyToAllPlayers();
			if( GetCurWheelItemData(m_iSelection)->m_pAction->m_sScreen != "" )
				return true;
			else
				return false;
		default: return true;
	}
*/
}

void MusicWheel::StartRoulette() 
{
	MESSAGEMAN->Broadcast("StartRoulette");
	m_WheelState = STATE_ROULETTE_SPINNING;
	m_Moving = 1;
	m_TimeBeforeMovingBegins = 0;
	m_SpinSpeed = 1.0f/ROULETTE_SWITCH_SECONDS;
	//GAMESTATE->m_SortOrder.Set( SORT_ROULETTE );
	//SetOpenSection( "" );
	RebuildWheelItems();
}

void MusicWheel::StartRandom()
{
	MESSAGEMAN->Broadcast("StartRandom");
	/* If RANDOM_PICKS_LOCKED_SONGS is disabled, pick a song from the active sort and
	 * section.  If enabled, picking from the section makes it too easy to trick the
	 * game into picking a locked song, so pick from SORT_ROULETTE. */
	if( RANDOM_PICKS_LOCKED_SONGS )
	{
		// Shuffle and use the roulette wheel.
		RandomGen rnd;
		vector<MusicWheelItemData *> from;
		getWheelItemsData(SORT_ROULETTE,from);
		random_shuffle( from.begin(), from.end(), rnd );
		GAMESTATE->m_SortOrder.Set( SORT_ROULETTE );
	}
	else
	{
		GAMESTATE->m_SortOrder.Set( GAMESTATE->m_PreferredSortOrder );
	}
	SetOpenSection( "" );

	m_Moving = -1;
	m_TimeBeforeMovingBegins = 0;
	m_SpinSpeed = 1.0f/ROULETTE_SWITCH_SECONDS;
	m_SpinSpeed *= 20.0f; /* faster! */
	m_WheelState = STATE_RANDOM_SPINNING;

	SelectSong( GetPreferredSelectionForRandomOrPortal() );

	RebuildWheelItems();
}

void MusicWheel::SetOpenSection( RString group )
{
	//LOG->Trace( "MusicWheel::SetOpenSection: %s", group.c_str() );
	
	m_sExpandedSectionName = group;
	GAMESTATE->sExpandedSectionName = group;
	
	// Clears current wheel items data
	m_CurWheelItemData.clear();
	
	// Gets new items data according to the group opened
	vector<MusicWheelItemData *> from;
	getWheelChannelItemsData( group, from );
	m_CurWheelItemData.reserve( from.size() );
	for( unsigned i = 0; i < from.size(); ++i )
	{
		m_CurWheelItemData.push_back(from[i]);
	}

	// Restarts selection
	m_iSelection = 0;
	
	// Rebuilds items
	RebuildWheelItems();
	
	// Set the position of each item on the wheel (no moving)
	SetPositions();
}

// sm-ssc additions: jump to group
RString MusicWheel::JumpToNextGroup()
{
	// Thanks to Juanelote for this logic:
	if( HIDE_INACTIVE_SECTIONS )
	{
		//todo: make it work with other sort types
		unsigned iNumGroups = SONGMAN->GetNumSongGroups();

		for(unsigned i = 0 ; i < iNumGroups ; i++)
		{
			if( m_sExpandedSectionName == SONGMAN->GetSongGroupByIndex(i) )
			{
				if ( i < iNumGroups - 1 )
					return SONGMAN->GetSongGroupByIndex(i+1);
				else
				{
					//i = 0;
					return SONGMAN->GetSongGroupByIndex(0);
				}
			}
		}
	}
	else
	{
		unsigned int iLastSelection = m_iSelection;
		for( unsigned int i = m_iSelection; i < m_CurWheelItemData.size(); ++i )
		{
			if( m_CurWheelItemData[i]->m_Type == WheelItemDataType_Section && i != (unsigned int)m_iSelection )
			{
				m_iSelection = i;
				return m_CurWheelItemData[i]->m_sText;
			}
		}
		// it should not get down here, but it might happen... only search up to
		// the previous selection.
		for( unsigned int i = 0; i < iLastSelection; ++i )
		{
			if( m_CurWheelItemData[i]->m_Type == WheelItemDataType_Section && i != (unsigned int)m_iSelection )
			{
				m_iSelection = i;
				return m_CurWheelItemData[i]->m_sText;
			}
		}
	}
	// it shouldn't get here, but just in case...
	return "";
}

RString MusicWheel::JumpToPrevGroup()
{
	if( HIDE_INACTIVE_SECTIONS )
	{
		unsigned iNumGroups = SONGMAN->GetNumSongGroups();

		for(unsigned i = 0 ; i < iNumGroups ; i++)
		{
			if( m_sExpandedSectionName == SONGMAN->GetSongGroupByIndex(i) )
			{
				if ( i > 0 )
					return SONGMAN->GetSongGroupByIndex(i-1);
				else
				{
					//i = iNumGroups - 1;
					return SONGMAN->GetSongGroupByIndex(iNumGroups - 1);
				}
			}
		}
	}
	else
	{
		for( unsigned int i = m_iSelection; i > 0; --i )
		{
			if( m_CurWheelItemData[i]->m_Type == WheelItemDataType_Section && i != (unsigned int)m_iSelection )
			{
				m_iSelection = i;
				return m_CurWheelItemData[i]->m_sText;
			}
		}
		// in case it wasn't found above:
		for( unsigned int i = m_CurWheelItemData.size()-1; i > 0; --i )
		{
			LOG->Trace( "JumpToPrevGroup iteration 2 | i = %u",i );
			if( m_CurWheelItemData[i]->m_Type == WheelItemDataType_Section )
			{
				m_iSelection = i;
				LOG->Trace( "finding it in #2 | i = %u | text = %s",i, m_CurWheelItemData[i]->m_sText.c_str() );
				return m_CurWheelItemData[i]->m_sText;
			}
		}
	}
	// it shouldn't get here, but just in case...
	return "";
}

// Called on late join. Selectable courses may have changed; reopen the section.
void MusicWheel::PlayerJoined()
{
	// Release all the cached/stored items when a new player joins the game
	ReleaseWheelItems();
	
	// Re-open the last group or sort to update the songs list
	OpenGroupOrChangeSort( GAMESTATE->m_sCurrentGroupName );
	
	/*
	// TRICKY: If Autogen is off and someone joins, the first player may be on
	// a song that has an illegal stepstype for the current amount of players.
	// (e.g. a song that only has doubles difficulties and a second player joins.)
	// We need to rebuild the wheel item data in this situation. -aj
	if( !GAMESTATE->IsCourseMode() && !PREFSMAN->m_bAutogenSteps )
	{
		FOREACH_ENUM(SortOrder, so)
		{
			m_WheelItemDatasStatus[so] = INVALID;
		}
		RebuildWheelItems();
	}
	//SetOpenSection( m_sExpandedSectionName );	//xMAx
	*/
}

bool MusicWheel::IsRouletting() const
{
	return m_WheelState == STATE_ROULETTE_SPINNING || m_WheelState == STATE_ROULETTE_SLOWING_DOWN ||
		   m_WheelState == STATE_RANDOM_SPINNING;
}

Song* MusicWheel::GetSelectedSong()
{
	switch( m_CurWheelItemData[m_iSelection]->m_Type )
	{
		case WheelItemDataType_Portal:
			return GetPreferredSelectionForRandomOrPortal();
		default:
			return GetCurWheelItemData(m_iSelection)->m_pSong;
	}
}

/* Find a random song.  If possible, find one that has the preferred difficulties of
 * each player.  Prefer songs in the active group, if any. 
 *
 * Note that if this is called, we *must* find a song.  We will only be called if
 * the active sort has at least one song, but there may be no open group.  This means
 * that any filters and preferences applied here must be optional. */
Song *MusicWheel::GetPreferredSelectionForRandomOrPortal()
{
	// probe to find a song that has the preferred 
	// difficulties of each player
	vector<Difficulty> vDifficultiesToRequire;
	FOREACH_HumanPlayer(p)
	{
		if( GAMESTATE->m_PreferredDifficulty[p] == Difficulty_Invalid )
			continue;	// skip

		// TRICKY: Don't require that edits be present if perferred 
		// difficulty is Difficulty_Edit.  Otherwise, players could use this 
		// to set up a 100% chance of getting a particular locked song by 
		// having a single edit for a locked song.
		if( GAMESTATE->m_PreferredDifficulty[p] == Difficulty_Edit )
			continue;	// skip

		vDifficultiesToRequire.push_back( GAMESTATE->m_PreferredDifficulty[p] );
	}

	RString sPreferredGroup = m_sExpandedSectionName;
	vector<MusicWheelItemData *> wid;
	getWheelItemsData(GAMESTATE->m_SortOrder,wid);

	StepsType st = GAMESTATE->GetCurrentStyle()->m_StepsType;

#define NUM_PROBES 1000
	for( int i=0; i<NUM_PROBES; i++ )
	{
		bool isValid = true;
		/* Maintaining difficulties is higher priority than maintaining
		 * the current group. */
		if( i == NUM_PROBES/4 )
			sPreferredGroup = "";
		if( i == NUM_PROBES/2 )
			vDifficultiesToRequire.clear();

		int iSelection = RandomInt( wid.size() );
		if( wid[iSelection]->m_Type != WheelItemDataType_Song )
			continue;

		const Song *pSong = wid[iSelection]->m_pSong;

		if( !sPreferredGroup.empty() && wid[iSelection]->m_sText != sPreferredGroup )
			continue;

		// There's an off possibility that somebody might have only one song with only beginner steps.
		if( i < 900 && pSong->IsTutorial() )
			continue;

		for( auto d = vDifficultiesToRequire.begin(); d != vDifficultiesToRequire.end(); ++d )
		{
			if( !pSong->HasStepsTypeAndDifficulty(st,*d) )
			{
				isValid = false;
				break;
			}
		}

		if (isValid)
		{
			return wid[iSelection]->m_pSong;
		}
	}
	LuaHelpers::ReportScriptError( "Couldn't find any songs" );
	return wid[0]->m_pSong;
}

void MusicWheel::FinishChangingSorts()
{
	FinishTweening();
	m_WheelState = STATE_SELECTING;
	m_fTimeLeftInState = 0;
}

// lua start
/*#include "LuaBinding.h"

class LunaMusicWheel: public Luna<MusicWheel>
{
public:
	static int ChangeSort( T* p, lua_State *L )
	{
		if( lua_isnil(L,1) ) { lua_pushboolean( L, false ); }
		else
		{
			SortOrder so = Enum::Check<SortOrder>(L, 1);
			lua_pushboolean( L, p->ChangeSort( so ) );
		}
		return 1;
	}
	static int IsRouletting( T* p, lua_State *L ){ lua_pushboolean( L, p->IsRouletting() ); return 1; }
	static int SelectSong( T* p, lua_State *L )
	{
		if( lua_isnil(L,1) ) { lua_pushboolean( L, false ); }
		else
		{
			Song *pS = Luna<Song>::check( L, 1, true );
			lua_pushboolean( L, p->SelectSong( pS ) );
		}
		return 1;
	}
	static int SelectCourse( T* p, lua_State *L )
	{
		if( lua_isnil(L,1) ) { lua_pushboolean( L, false ); }
		else
		{
			Course *pC = Luna<Course>::check( L, 1, true );
			lua_pushboolean( L, p->SelectCourse( pC ) );
		}
		return 1;
	}


	LunaMusicWheel()
	{
		ADD_METHOD( ChangeSort );
		ADD_METHOD( IsRouletting );
		ADD_METHOD( SelectSong );
		ADD_METHOD( SelectCourse );
	}
};

LUA_REGISTER_DERIVED_CLASS( MusicWheel, WheelBase )
// lua end
*/

/*
 * (c) 2001-2004 Chris Danford, Chris Gomez, Glenn Maynard
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
