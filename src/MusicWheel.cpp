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
#include "NetworkSyncManager.h"
#include "ProfileManager.h"
#include "Song.h"
#include "Course.h"
#include "Steps.h"
#include "UnlockManager.h"
#include "GameCommand.h"
#include "ActorUtil.h"
#include "SongUtil.h"
#include "CourseUtil.h"
#include "Style.h"
#include "PlayerState.h"
#include "CommonMetrics.h"
#include "MessageManager.h"
#include "LocalizedString.h"

static Preference<bool> g_bMoveRandomToEnd("MoveRandomToEnd", false);
// static Preference<bool> g_bPrecacheAllSorts( "PreCacheAllWheelSorts", false); // xMAx
static Preference<bool> g_bPrecacheWheelItems("PreCacheWheelItems", true);	//xMAx

#define NUM_WHEEL_ITEMS		((int)ceil(NUM_WHEEL_ITEMS_TO_DRAW+2))
#define WHEEL_TEXT(s)		THEME->GetString( "MusicWheel", ssprintf("%sText",s.c_str()) );
#define CUSTOM_ITEM_WHEEL_TEXT(s)		THEME->GetString( "MusicWheel", ssprintf("CustomItem%sText",s.c_str()) );

static RString SECTION_COLORS_NAME(size_t i) { return ssprintf("SectionColor%d", int(i + 1)); }
static RString CHOICE_NAME(RString s) { return ssprintf("Choice%s", s.c_str()); }
static RString CUSTOM_WHEEL_ITEM_NAME(RString s) { return ssprintf("CustomWheelItem%s", s.c_str()); }
static RString CUSTOM_WHEEL_ITEM_COLOR(RString s) { return ssprintf("%sColor", s.c_str()); }

static LocalizedString EMPTY_STRING("MusicWheel", "Empty");

AutoScreenMessage(SM_SongChanged); // TODO: Replace this with a Message and MESSAGEMAN
AutoScreenMessage(SM_SortOrderChanging);
AutoScreenMessage(SM_SortOrderChanged);

static SortOrder ForceAppropriateSort(PlayMode pm, SortOrder so)
{
	switch (pm)
	{
		// in course modes, force a particular sort
	case PLAY_MODE_ONI:	return SORT_ONI_COURSES;
	case PLAY_MODE_NONSTOP:	return SORT_NONSTOP_COURSES;
	case PLAY_MODE_ENDLESS:	return SORT_ENDLESS_COURSES;
	default: break;
	}

	// If we're not in a course mode, don't start in a course sort.
	switch (so)
	{
	case SORT_ONI_COURSES:
	case SORT_NONSTOP_COURSES:
	case SORT_ENDLESS_COURSES:
		so = SortOrder_Invalid;
	default:
		return so;
	}
}

MusicWheelItem* MusicWheel::MakeItem()
{
	return new MusicWheelItem;
}

void MusicWheel::Load(RString sType)
{
	ROULETTE_SWITCH_SECONDS.Load(sType, "RouletteSwitchSeconds");
	ROULETTE_SLOW_DOWN_SWITCHES.Load(sType, "RouletteSlowDownSwitches");
	NUM_SECTION_COLORS.Load(sType, "NumSectionColors");
	SONG_REAL_EXTRA_COLOR.Load(sType, "SongRealExtraColor");
	SORT_MENU_COLOR.Load(sType, "SortMenuColor");
	SHOW_ROULETTE.Load(sType, "ShowRoulette");
	SHOW_RANDOM.Load(sType, "ShowRandom");
	SHOW_PORTAL.Load(sType, "ShowPortal");
	RANDOM_PICKS_LOCKED_SONGS.Load(sType, "RandomPicksLockedSongs");
	MOST_PLAYED_SONGS_TO_SHOW.Load(sType, "MostPlayedSongsToShow");
	RECENT_SONGS_TO_SHOW.Load(sType, "RecentSongsToShow");
	MODE_MENU_CHOICE_NAMES.Load(sType, "ModeMenuChoiceNames");
	SORT_ORDERS.Load(sType, "SortOrders");
	SHOW_EASY_FLAG.Load(sType, "UseEasyMarkerFlag");
	USE_SECTIONS_WITH_PREFERRED_GROUP.Load(sType, "UseSectionsWithPreferredGroup");
	HIDE_INACTIVE_SECTIONS.Load(sType, "OnlyShowActiveSection");
	HIDE_ACTIVE_SECTION_TITLE.Load(sType, "HideActiveSectionTitle");
	REMIND_WHEEL_POSITIONS.Load(sType, "RemindWheelPositions");
	vector<RString> vsModeChoiceNames;
	split(MODE_MENU_CHOICE_NAMES, ",", vsModeChoiceNames);
	CHOICE.Load(sType, CHOICE_NAME, vsModeChoiceNames);
	SECTION_COLORS.Load(sType, SECTION_COLORS_NAME, NUM_SECTION_COLORS);

	CUSTOM_WHEEL_ITEM_NAMES.Load(sType, "CustomWheelItemNames");
	vector<RString> vsCustomItemNames;
	split(CUSTOM_WHEEL_ITEM_NAMES, ",", vsCustomItemNames);
	CUSTOM_CHOICES.Load(sType, CUSTOM_WHEEL_ITEM_NAME, vsCustomItemNames);
	CUSTOM_CHOICE_COLORS.Load(sType, CUSTOM_WHEEL_ITEM_COLOR, vsCustomItemNames);

	ROULETTE_COLOR.Load(sType, "RouletteColor");
	RANDOM_COLOR.Load(sType, "RandomColor");
	PORTAL_COLOR.Load(sType, "PortalColor");
	EMPTY_COLOR.Load(sType, "EmptyColor");

	WheelBase::Load(sType);

	SONGMAN->UpdateRankingCourses();

	m_soundChangeSort.Load(THEME->GetPathS(sType, "sort"));
	m_soundExpand.Load(THEME->GetPathS(sType, "expand"), true);
	m_soundCollapse.Load(THEME->GetPathS(sType, "collapse"), true);

	// Update for SORT_MOST_PLAYED.
	// SONGMAN->UpdatePopular(); // xMAx

	/* Sort SONGMAN's songs by CompareSongPointersByTitle, so we can do other sorts (with
	 * stable_sort) from its output, and title will be the secondary sort, without having
	 * to re-sort by title each time. */
	 // SONGMAN->SortSongs(); // xMAx - Not necessary. Songs are sorted during initial program loading


	FOREACH_ENUM(SortOrder, so) {
		m_WheelItemDatasStatus[so] = INVALID;
	}

	// xMAx - added for an empty music wheel after filtered steps type
	dummyMWID = new MusicWheelItemData(WheelItemDataType_Custom, NULL, "DUMMYME", NULL, RageColor(1, 1, 1, 1), 0);
}

void MusicWheel::BeginScreen()
{
	// xMAx -----------------------------------------------------------------------------------------
	if (g_bPrecacheWheelItems)
	{
		RageTimer	timer;
		vector<RString>	vsAllAvailableChannels;

		// xMAx - List channels by remaining life tokens and players
		SONGMAN->GetSongGroupNamesAvailables(vsAllAvailableChannels);

		// xMAx - Search for automatic channels (sorts) and regular channels (song groups)
		for (unsigned i = 0; i < vsAllAvailableChannels.size(); i++)
		{
			if (vsAllAvailableChannels[i].find("SO_") != std::string::npos || vsAllAvailableChannels[i] == "AllCourses")
			{
				SortOrder so = StringToSortOrder(vsAllAvailableChannels[i]);
				if (so != SortOrder_Invalid)
				{
					readyWheelItemsData(so);
					LOG->Trace("MusicWheel::Pre-cached sort order: %s", vsAllAvailableChannels[i].c_str());
				}
			}
			else
			{
				readyWheelChannelItemsData(vsAllAvailableChannels[i]);
				LOG->Trace("MusicWheel::Pre-cached song group: %s", vsAllAvailableChannels[i].c_str());
			}
		}
		LOG->Trace("MusicWheel::MusicWheel precache items took: %s", ssprintf("%.3f ", timer.GetDeltaTime()).c_str());
	}

	WheelBase::BeginScreen();
}

//
void MusicWheel::CheckCurrentSongOnWheel()
{
	if (SelectSongOrCourse())
		RebuildWheelItems();
}
//

MusicWheel::~MusicWheel()
{
	// Release caches items
	ReleaseWheelItems();

	// Delete dummy item
	if (dummyMWID != nullptr)
	{
		SAFE_DELETE(dummyMWID);
	}
	// ----------------------------------------------------------------------------------------------
}

void MusicWheel::ReleaseWheelItems()
{
	FOREACH_ENUM(SortOrder, so)
	{
		vector<MusicWheelItemData*>& vMWID = m_WheelItemsData[so];

		if (vMWID.empty())
			continue;

		vector<MusicWheelItemData*>::iterator i = vMWID.begin();
		for (; i != vMWID.end(); ++i)
		{
			SAFE_DELETE(*i);
		}
		vMWID.clear();
	}

	// xMAx - Clear groups cache
	map<RString, vector<MusicWheelItemData*>>::iterator it = m_WheelItemsDataForGroups.begin();
	for (; it != m_WheelItemsDataForGroups.end(); ++it)
	{
		vector<MusicWheelItemData*>& vMWID = it->second;
		vector<MusicWheelItemData*>::iterator i = vMWID.begin();
		for (; i != vMWID.end(); ++i)
		{
			SAFE_DELETE(*i);
		}
		vMWID.clear();
	}
	m_WheelItemsDataForGroups.clear();
}

void MusicWheel::ReloadSongList()
{
	LOG->Trace("MusicWheel::ReloadSongList() - Starting");

	// Release all cached wheel items
	ReleaseWheelItems();

	// Reset wheel state
	m_CurWheelItemData.clear();
	m_iSelection = 0;
	m_sExpandedSectionName = "";
	GAMESTATE->sExpandedSectionName = "";

	// Get available song groups
	vector<RString> vsAvailableGroups;
	SONGMAN->GetSongGroupNamesAvailables(vsAvailableGroups);

	LOG->Trace("MusicWheel::ReloadSongList() - Found %d available groups", vsAvailableGroups.size());
	for (unsigned i = 0; i < vsAvailableGroups.size(); i++)
	{
		LOG->Trace("MusicWheel::ReloadSongList() - Group %d: %s", i, vsAvailableGroups[i].c_str());
	}

	// Try to find a suitable default group
	RString sDefaultGroup = "";

	// First, try to find "SO_ALLTUNES" (AllTunes sort)
	for (unsigned i = 0; i < vsAvailableGroups.size(); i++)
	{
		if (vsAvailableGroups[i] == "SO_ALLTUNES")
		{
			sDefaultGroup = vsAvailableGroups[i];
			LOG->Trace("MusicWheel::ReloadSongList() - Found SO_ALLTUNES, using as default");
			break;
		}
	}

	// If not found, try to find any group that's not a sort order
	if (sDefaultGroup.empty())
	{
		for (unsigned i = 0; i < vsAvailableGroups.size(); i++)
		{
			if (vsAvailableGroups[i].find("SO_") == std::string::npos)
			{
				sDefaultGroup = vsAvailableGroups[i];
				LOG->Trace("MusicWheel::ReloadSongList() - Found non-sort group: %s", sDefaultGroup.c_str());
				break;
			}
		}
	}

	// If still not found, use the first available group
	if (sDefaultGroup.empty() && !vsAvailableGroups.empty())
	{
		sDefaultGroup = vsAvailableGroups[0];
		LOG->Trace("MusicWheel::ReloadSongList() - Using first available group: %s", sDefaultGroup.c_str());
	}

	// Set the default section
	if (!sDefaultGroup.empty())
	{
		LOG->Trace("MusicWheel::ReloadSongList() - Setting open section to: %s", sDefaultGroup.c_str());
		SetOpenSection(sDefaultGroup);
	}
	else
	{
		LOG->Trace("MusicWheel::ReloadSongList() - No groups available, setting empty section");
		// If no groups available, try to open with empty string
		SetOpenSection("");
	}

	// Rebuild wheel items
	LOG->Trace("MusicWheel::ReloadSongList() - Rebuilding wheel items, current size: %d", m_CurWheelItemData.size());
	RebuildWheelItems();
	LOG->Trace("MusicWheel::ReloadSongList() - Finished, final size: %d", m_CurWheelItemData.size());
}

/* If a song or course is set in GAMESTATE and available, select it.  Otherwise, choose the
 * first available song or course.  Return true if an item was set, false if no items are
 * available. */
bool MusicWheel::SelectSongOrCourse()
{
	if (GAMESTATE->m_pPreferredSong && SelectSong(GAMESTATE->m_pPreferredSong))
		return true;
	if (GAMESTATE->m_pCurSong && SelectSong(GAMESTATE->m_pCurSong))
		return true;
	if (GAMESTATE->m_pPreferredCourse && SelectCourse(GAMESTATE->m_pPreferredCourse))
		return true;
	if (GAMESTATE->m_pCurCourse && SelectCourse(GAMESTATE->m_pCurCourse))
		return true;

	/* xMAx
	  // Select the first selectable song based on the sort order...
	  vector<MusicWheelItemData *> &wiWheelItems = getWheelItemsData(GAMESTATE->m_SortOrder);
	  for( unsigned i = 0; i < wiWheelItems.size(); i++ )
	  {
		  if( wiWheelItems[i]->m_pSong )
			  return SelectSong( wiWheelItems[i]->m_pSong );
		  else if ( wiWheelItems[i]->m_pCourse )
			  return SelectCourse( wiWheelItems[i]->m_pCourse );
	  }
	*/

	LOG->Trace("MusicWheel::MusicWheel() - No selectable songs or courses found in WheelData");
	return false;
}

bool MusicWheel::SelectSection(const RString& SectionName)
{
	for (unsigned int i = 0; i < m_CurWheelItemData.size(); ++i)
	{
		if (m_CurWheelItemData[i]->m_sText == SectionName)
		{
			m_iSelection = i;		// select it
			return true;
		}
	}

	return false;
}

bool MusicWheel::SelectSong(const Song* p)
{
	if (p == nullptr)
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
	for (i = 0; i < m_CurWheelItemData.size(); i++)
	{
		if (GetCurWheelItemData(i)->m_pSong == p)
		{
			m_iSelection = i;		// select it
			return true;
		}
	}

	return false;
}

bool MusicWheel::SelectCourse(const Course* p)
{
	if (p == nullptr)
		return false;

	unsigned i;
	vector<MusicWheelItemData*> from;
	getWheelItemsData(GAMESTATE->m_SortOrder, from);
	for (i = 0; i < from.size(); i++)
	{
		if (from[i]->m_pCourse == p)
		{
			// make its group the currently expanded group
			SetOpenSection(from[i]->m_sText);
			break;
		}
	}

	if (i == from.size())
		return false;

	for (i = 0; i < m_CurWheelItemData.size(); i++)
	{
		if (GetCurWheelItemData(i)->m_pCourse == p)
			m_iSelection = i; // select it
	}

	return true;
}

bool MusicWheel::SelectModeMenuItem()
{
	// Select the last-chosen option.
	ASSERT(GAMESTATE->m_SortOrder == SORT_MODE_MENU);
	vector<MusicWheelItemData*> from;
	getWheelItemsData(GAMESTATE->m_SortOrder, from);
	unsigned i;
	for (i = 0; i < from.size(); i++)
	{
		const GameCommand& gc = *from[i]->m_pAction;
		if (gc.m_sName == m_sLastModeMenuItem)
			break;
	}
	if (i == from.size())
		return false;

	// make its group the currently expanded group
	SetOpenSection(from[i]->m_sText);

	for (i = 0; i < m_CurWheelItemData.size(); i++)
	{
		if (GetCurWheelItemData(i)->m_pAction->m_sName != m_sLastModeMenuItem)
			continue;
		m_iSelection = i;		// select it
		break;
	}

	return true;
}

// bool MusicWheel::SelectCustomItem()

// void MusicWheel::GetSongList( vector<Song*> &arraySongs, SortOrder so ) // xMAx
void MusicWheel::GetSongList(vector<Song*>& arraySongs, SortOrder so, RString SongGroup) // xMAx
{
	LOG->Trace("MusicWheel::GetSongList - SortOrder: %d, SongGroup: %s", so, SongGroup.c_str());
	vector<Song*> apAllSongs;
	switch (so)
	{
		/* xMAx
		  case SORT_PREFERRED:
			  SONGMAN->GetPreferredSortSongs( apAllSongs );
			  break;
		  case SORT_POPULARITY:
			  apAllSongs = SONGMAN->GetPopularSongs();
			  break;
		*/

		// xMAx -----------------------------------------------------------------------------------------
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
	case SORT_LEVEL_1: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[0]).m_pAllStyles; }; break;
	case SORT_LEVEL_2: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[1]).m_pAllStyles; }; break;
	case SORT_LEVEL_3: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[2]).m_pAllStyles; }; break;
	case SORT_LEVEL_4: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[3]).m_pAllStyles; }; break;
	case SORT_LEVEL_5: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[4]).m_pAllStyles; }; break;
	case SORT_LEVEL_6: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[5]).m_pAllStyles; }; break;
	case SORT_LEVEL_7: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[6]).m_pAllStyles; }; break;
	case SORT_LEVEL_8: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[7]).m_pAllStyles; }; break;
	case SORT_LEVEL_9: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[8]).m_pAllStyles; }; break;
	case SORT_LEVEL_10: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[9]).m_pAllStyles; }; break;
	case SORT_LEVEL_11: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[10]).m_pAllStyles; }; break;
	case SORT_LEVEL_12: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[11]).m_pAllStyles; }; break;
	case SORT_LEVEL_13: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[12]).m_pAllStyles; }; break;
	case SORT_LEVEL_14: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[13]).m_pAllStyles; }; break;
	case SORT_LEVEL_15: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[14]).m_pAllStyles; }; break;
	case SORT_LEVEL_16: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[15]).m_pAllStyles; }; break;
	case SORT_LEVEL_17: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[16]).m_pAllStyles; }; break;
	case SORT_LEVEL_18: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[17]).m_pAllStyles; }; break;
	case SORT_LEVEL_19: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[18]).m_pAllStyles; }; break;
	case SORT_LEVEL_20: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[19]).m_pAllStyles; }; break;
	case SORT_LEVEL_21: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[20]).m_pAllStyles; }; break;
	case SORT_LEVEL_22: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[21]).m_pAllStyles; }; break;
	case SORT_LEVEL_23: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[22]).m_pAllStyles; }; break;
	case SORT_LEVEL_24_OVER: { vector<LevelSongList*> m_CurList = SONGMAN->GetLevelSections(); apAllSongs = (*m_CurList[23]).m_pAllStyles; }; break;
		// ----------------------------------------------------------------------------------------------

	case SORT_GROUP:
		// xMAx - GetRandomSongs
		if (SongGroup != "")
		{
			apAllSongs = SONGMAN->GetSongs(SongGroup);
			break;
		}

		// if we're not using sections with a preferred song group, and there
		// is a group to load, only load those songs. -aj
		// if(GAMESTATE->m_sPreferredSongGroup != GROUP_ALL && !USE_SECTIONS_WITH_PREFERRED_GROUP ) // xMAx
		if (GAMESTATE->sExpandedSectionName != "") // xMAx
		{
			// apAllSongs = SONGMAN->GetSongs(GAMESTATE->m_sPreferredSongGroup); // xMAx
			apAllSongs = SONGMAN->GetSongs(GAMESTATE->sExpandedSectionName); // xMAx
			break;
		}
		// otherwise fall through
	default:
		apAllSongs = SONGMAN->GetAllSongs();
		break;
	}

	// xMAx
	if (apAllSongs.empty())
		return;

	// filter songs that we don't have enough stages to play
  // vector<Song*> vTempSongs; // xMAx
	vector<Song*> vFilteredSongs; // xMAx
	SongCriteria sc;
	// sc.m_iMaxStagesForSong = GAMESTATE->GetSmallestNumStagesLeftForAnyHumanPlayer(); //xMAx
	sc.m_iMaxStagesForSong = GAMESTATE->GetHighestNumStagesLeftForAnyHumanPlayer(); // xMAx
	// SongUtil::FilterSongs( sc, apAllSongs, vTempSongs ); // xMAx
	SongUtil::FilterSongs(sc, apAllSongs, vFilteredSongs); // xMAx
	// apAllSongs = vTempSongs; // xMAx
	apAllSongs.clear(); // xMAx

	// copy only songs that have at least one Steps for the current GameMode
	// for( unsigned i=0; i<apAllSongs.size(); i++ ) // xMAx
	for (unsigned i = 0; i < vFilteredSongs.size(); i++) // xMAx
	{
		// Song* pSong = apAllSongs[i]; // xMAx
		Song* pSong = vFilteredSongs[i]; // xMAx

		/* xMAx
			int iLocked = UNLOCKMAN->SongIsLocked( pSong );
			if( iLocked & LOCKED_DISABLED )
				continue;

			// If we're on an extra stage, and this song is selected, ignore #SELECTABLE.
			if( pSong != GAMESTATE->m_pCurSong || !GAMESTATE->IsAnExtraStage() )
			{
				// Hide songs that asked to be hidden via #SELECTABLE.
				if( iLocked & LOCKED_SELECTABLE )
					continue;
				if( so != SORT_ROULETTE && iLocked & LOCKED_ROULETTE )
					continue;
			}

			// Hide locked songs. If RANDOM_PICKS_LOCKED_SONGS, hide in Roulette
			// and Random, too.
			if( (so!=SORT_ROULETTE || !RANDOM_PICKS_LOCKED_SONGS) && iLocked )
				continue;

			if( PREFSMAN->m_bOnlyPreferredDifficulties )
			{
				// if the song has steps that fit the preferred difficulty of the default player
				if( pSong->HasStepsTypeAndDifficulty( GAMESTATE->GetCurrentStyle(PLAYER_INVALID)->m_StepsType,GAMESTATE->m_PreferredDifficulty[GAMESTATE->GetFirstHumanPlayer()] ) )
					arraySongs.push_back( pSong );
			}
			else
			{
				// Online mode doesn't support auto set style.  A song that only has
				// dance-double steps will show up when dance-single was selected, with
				// no playable steps.  Then the game will crash when trying to play it.
				// -Kyz
				if(CommonMetrics::AUTO_SET_STYLE && !NSMAN->isSMOnline)
				{
					// with AUTO_SET_STYLE on and Autogen off, some songs may get
					// hidden. Search through every playable StepsType until you
					// find one, then add the song.
					// see Issue 147 for more information. -aj
					// http://ssc.ajworld.net/sm-ssc/bugtracker/view.php?id=147
					set<StepsType> vStepsType;
					SongUtil::GetPlayableStepsTypes( pSong, vStepsType );

					for (StepsType const &type : vStepsType)
					{
						if(pSong->HasStepsType(type))
						{
							arraySongs.push_back( pSong );
							break;
						}
					}
				}
				else
				{
					// If the song has at least one steps, add it.
					if( pSong->HasStepsType(GAMESTATE->GetCurrentStyle(PLAYER_INVALID)->m_StepsType) )
						arraySongs.push_back( pSong );
				}
			}
		*/

		// xMAx ---------------------------------------------------------------------------------------
		vector<Steps*>	m_vpSteps;
		SongUtil::GetPlayableSteps(pSong, m_vpSteps, so);
		if (m_vpSteps.empty())
			continue;

		arraySongs.push_back(pSong);
		// --------------------------------------------------------------------------------------------
	}

	LOG->Trace("MusicWheel::GetSongList - Returning %d songs", arraySongs.size());
}

// void MusicWheel::BuildWheelItemsData( vector<MusicWheelItemData *> &arrayWheelItemDatas, SortOrder so ) // xMAx
void MusicWheel::BuildWheelItemsData(vector<MusicWheelItemData*>& arrayWheelItemDatas, SortOrder so, RString SongGroup) // xMAx
{
	LOG->Trace("MusicWheel::BuildWheelItemsData - SortOrder: %d, SongGroup: %s", so, SongGroup.c_str());
	switch (so)
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
		if (so == SORT_GROUP)
		{
			if (SongGroup != "")
			{
				GetSongList(arraySongs, so, SongGroup);
			}
		}
		else
		{
			GetSongList(arraySongs, so, SongGroup);
		}

		if (arraySongs.empty())
			return;

		arrayWheelItemDatas.clear();
		arrayWheelItemDatas.reserve(arraySongs.size());

		for (unsigned i = 0; i < arraySongs.size(); i++)
		{
			Song* pSong = arraySongs[i];
			arrayWheelItemDatas.push_back(new MusicWheelItemData(WheelItemDataType_Song, pSong, "", NULL, RageColor(1, 1, 1, 1), 0));
		}
		break;
	}
	case SORT_ALL_COURSES:
	case SORT_NONSTOP_COURSES:
	case SORT_ONI_COURSES:
	case SORT_ENDLESS_COURSES:
	{
		//bool bOnlyPreferred = PREFSMAN->m_CourseSortOrder == COURSE_SORT_PREFERRED;
		bool bOnlyPreferred = false;

		vector<CourseType> vct;
		switch (so)
		{
		case SORT_NONSTOP_COURSES:
			vct.push_back(COURSE_TYPE_NONSTOP);
			break;
		case SORT_ONI_COURSES:
			vct.push_back(COURSE_TYPE_ONI);
			vct.push_back(COURSE_TYPE_SURVIVAL);
			break;
		case SORT_ENDLESS_COURSES:
			vct.push_back(COURSE_TYPE_ENDLESS);
			break;
		case SORT_ALL_COURSES:
			FOREACH_ENUM(CourseType, i)
				vct.push_back(i);
			break;
		default:
			FAIL_M(ssprintf("Wrong sort order: %i", so));
		}

		vector<Course*> apCourses;
		for (CourseType const& ct : vct)
		{
			if (bOnlyPreferred)
				SONGMAN->GetPreferredSortCourses(ct, apCourses, PREFSMAN->m_bAutogenGroupCourses);
			else
				SONGMAN->GetCourses(ct, apCourses, PREFSMAN->m_bAutogenGroupCourses);
		}

		LOG->Trace("xMAx::We have %d courses", apCourses.size());


		switch (PREFSMAN->m_CourseSortOrder)
		{
		case COURSE_SORT_SONGS:
			CourseUtil::SortCoursePointerArrayByDifficulty(apCourses);
			break;
		case COURSE_SORT_PREFERRED:
			break;
		case COURSE_SORT_METER:
			CourseUtil::SortCoursePointerArrayByAvgDifficulty(apCourses);
			break;
		case COURSE_SORT_METER_SUM:
			CourseUtil::SortCoursePointerArrayByTotalDifficulty(apCourses);
			break;
		case COURSE_SORT_RANK:
			CourseUtil::SortCoursePointerArrayByRanking(apCourses);
			break;
		default:	FAIL_M("Impossible to sort the courses! Aborting...");
		}

		// since we can't agree, make it an option
		if (PREFSMAN->m_CourseSortOrder != COURSE_SORT_SONGS && g_bMoveRandomToEnd)
			CourseUtil::MoveRandomToEnd(apCourses);

		if (so == SORT_ALL_COURSES)
			CourseUtil::SortCoursePointerArrayByType(apCourses);

		arrayWheelItemDatas.clear();	// clear out the previous wheel items

		RString sLastSection = "";
		int iSectionColorIndex = 0;
		for (unsigned i = 0; i < apCourses.size(); i++)	// foreach course
		{
			Course* pCourse = apCourses[i];

			// if unlocks are on, make sure it is unlocked
			if (UNLOCKMAN->CourseIsLocked(pCourse))
				continue;

			RString sThisSection = "";
			if (so == SORT_ALL_COURSES)
			{
				switch (pCourse->GetPlayMode())
				{
				case PLAY_MODE_ONI:	sThisSection = "Oni";		break;
				case PLAY_MODE_NONSTOP:	sThisSection = "Nonstop";	break;
				case PLAY_MODE_ENDLESS:	sThisSection = "Endless";	break;
				default: break;
				}
			}

			// check that this course has at least one song playable in the current style
			if (!pCourse->IsPlayableIn(GAMESTATE->GetCurrentStyle(PLAYER_INVALID)->m_StepsType))
				continue;

			arrayWheelItemDatas.push_back(new MusicWheelItemData(WheelItemDataType_Course, NULL, sThisSection, pCourse, RageColor(1, 1, 1, 1), 0));
		}

		LOG->Trace("xMAx::We have %d courses on the wheel", arrayWheelItemDatas.size());

		break;
	}
	default:
		break;
	}

	LOG->Trace("MusicWheel::BuildWheelItemsData - Finished, built %d items", arrayWheelItemDatas.size());
}

void MusicWheel::getWheelItemsData(SortOrder so, vector<MusicWheelItemData*>& out)
{
	LOG->Trace("MusicWheel::getWheelItemsData - SortOrder: %d", so);
	readyWheelItemsData(so);

	out = m_WheelItemsData[so];
	LOG->Trace("MusicWheel::getWheelItemsData - Returning %d items", out.size());
}

void MusicWheel::readyWheelItemsData(SortOrder so) {
	LOG->Trace("MusicWheel::readyWheelItemsData - SortOrder: %d, current size: %d", so, m_WheelItemsData[so].size());
	if (m_WheelItemsData[so].empty())
	{
		LOG->Trace("MusicWheel::readyWheelItemsData - Building wheel items data for SortOrder: %d", so);
		vector<MusicWheelItemData*> aUnFilteredDatas;
		BuildWheelItemsData(aUnFilteredDatas, so);
		m_WheelItemsData[so] = aUnFilteredDatas;
		LOG->Trace("MusicWheel::readyWheelItemsData - Built %d items", aUnFilteredDatas.size());
	}
	else
	{
		LOG->Trace("MusicWheel::readyWheelItemsData - Using cached data for SortOrder: %d", so);
	}
}

// xMAx - use this to get items for normal Channels (group of songs)
void MusicWheel::getWheelChannelItemsData(RString group, vector<MusicWheelItemData*>& out)
{
	LOG->Trace("MusicWheel::getWheelChannelItemsData - Group: %s", group.c_str());

	// Handle empty group string - try to use a default group
	if (group.empty())
	{
		LOG->Trace("MusicWheel::getWheelChannelItemsData - Empty group, trying to find default");
		vector<RString> vsAvailableGroups;
		SONGMAN->GetSongGroupNamesAvailables(vsAvailableGroups);

		if (!vsAvailableGroups.empty())
		{
			// Try to find a non-sort order group first
			for (unsigned i = 0; i < vsAvailableGroups.size(); i++)
			{
				if (vsAvailableGroups[i].find("SO_") == std::string::npos)
				{
					group = vsAvailableGroups[i];
					LOG->Trace("MusicWheel::getWheelChannelItemsData - Using default group: %s", group.c_str());
					break;
				}
			}

			// If no non-sort order group found, use the first available
			if (group.empty())
			{
				group = vsAvailableGroups[0];
				LOG->Trace("MusicWheel::getWheelChannelItemsData - Using first available group: %s", group.c_str());
			}
		}
		else
		{
			LOG->Warn("MusicWheel::getWheelChannelItemsData - No available groups found");
			out.clear();
			return;
		}
	}

	// Check if this is a sort order group (starts with "SO_")
	if (group.find("SO_") == 0)
	{
		LOG->Trace("MusicWheel::getWheelChannelItemsData - Detected sort order group");

		// Caso especial para SO_ALLTUNES (não converter, usar SORT_ALLTUNES diretamente)
		if (group == "SO_ALLTUNES")
		{
			LOG->Trace("MusicWheel::getWheelChannelItemsData - Using SORT_ALLTUNES directly");
			getWheelItemsData(SORT_ALLTUNES, out);
		}
		else
		{
			// Remove o "SO_" prefix para conversão
			RString sortOrderName = group.substr(3);
			LOG->Trace("MusicWheel::getWheelChannelItemsData - Sort order name: %s", sortOrderName.c_str());

			// Converte a string para SortOrder
			SortOrder so = StringToSortOrder(sortOrderName);
			LOG->Trace("MusicWheel::getWheelChannelItemsData - Converted to SortOrder: %d", so);

			if (so != SortOrder_Invalid)
			{
				getWheelItemsData(so, out);
			}
			else
			{
				LOG->Warn("MusicWheel::getWheelChannelItemsData - Invalid sort order: %s", sortOrderName.c_str());
				return;
			}
		}

		LOG->Trace("MusicWheel::getWheelChannelItemsData - Got %d items from getWheelItemsData", out.size());
		return;
	}

	// For regular song groups, check if the group exists
	if (!SONGMAN->DoesSongGroupExist(group))
	{
		LOG->Warn("MusicWheel::Group %s doesn't exist", group.c_str());
		out.clear();
		return;
	}

	LOG->Trace("MusicWheel::getWheelChannelItemsData - Regular song group, preparing data");
	// xMAx - we have to check if it's available or not since we're going to include it to the wheel items map
	readyWheelChannelItemsData(group);

	// Return the items list
	map<RString, vector<MusicWheelItemData*>>::iterator it = m_WheelItemsDataForGroups.find(group);
	if (it == m_WheelItemsDataForGroups.end())
	{
		LOG->Warn("MusicWheel::Couldn't find group %s in the precached groups list", group.c_str());
		out.clear();
		return;
	}

	out = it->second;
	LOG->Trace("MusicWheel::getWheelChannelItemsData - Returning %d items", out.size());
	// Aplicar el filtro de items
	//FilterItems( it->second, out, SORT_GROUP );
}

// xMAx -------------------------------------------------------------------------------------------
void MusicWheel::readyWheelChannelItemsData(RString group)
{
	map<RString, vector<MusicWheelItemData*>>::iterator it = m_WheelItemsDataForGroups.find(group);

	// Si no se encuentra el item hay que crearlo y precargarlo
	if (it == m_WheelItemsDataForGroups.end())
	{
		vector<MusicWheelItemData*> aUnfilteredItemsData;
		BuildWheelItemsData(aUnfilteredItemsData, SORT_GROUP, group);
		m_WheelItemsDataForGroups[group] = aUnfilteredItemsData;
	}
}
// ------------------------------------------------------------------------------------------------

void MusicWheel::FilterItems(vector<MusicWheelItemData*> arrayWheelItemDatas, vector<MusicWheelItemData*>& out, SortOrder so)
{
	out.clear();

	if (!GAMESTATE->m_SongOptions.GetCurrent().m_bShowSingles ||
		!GAMESTATE->m_SongOptions.GetCurrent().m_bShowHalfDoubles ||
		!GAMESTATE->m_SongOptions.GetCurrent().m_bShowDoubles)
	{
		for (unsigned i = 0; i < arrayWheelItemDatas.size(); ++i)
		{
			MusicWheelItemData& d = *arrayWheelItemDatas[i];
			Song* tempSong = d.m_pSong;
			vector<Steps*> m_vpSteps;
			SongUtil::GetFilteredPlayableSteps(tempSong, m_vpSteps, so);

			if (m_vpSteps.size() > 0)
			{
				out.push_back(arrayWheelItemDatas[i]);
			}
		}

		if (out.empty())
		{
			out.push_back(dummyMWID);
		}
	}
	else
	{
		for (unsigned i = 0; i < arrayWheelItemDatas.size(); ++i)
		{
			out.push_back(arrayWheelItemDatas[i]);
		}
	}
}

void MusicWheel::UpdateSwitch()
{
	switch (m_WheelState)
	{
	case STATE_FLYING_OFF_BEFORE_NEXT_SORT:
	{
		const Song* pPrevSelectedSong = GetCurWheelItemData(m_iSelection)->m_pSong;

		SCREENMAN->PostMessageToTopScreen(SM_SortOrderChanged, 0);

		SetOpenSection(SongUtil::GetSectionNameFromSongAndSort(pPrevSelectedSong, GAMESTATE->m_SortOrder));

		m_iSelection = 0;

		// Select the previously selected item
		switch (GAMESTATE->m_SortOrder)
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
		if (SongUtil::GetStepsTypeAndDifficultyFromSortOrder(GAMESTATE->m_SortOrder, st, dc))
		{
			ASSERT(dc != Difficulty_Invalid);
			FOREACH_PlayerNumber(p)
				if (GAMESTATE->IsPlayerEnabled(p))
					GAMESTATE->m_PreferredDifficulty[p].Set(dc);
		}

		SCREENMAN->PostMessageToTopScreen(SM_SongChanged, 0);
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
		if (m_iSwitchesLeftInSpinDown == 0)
		{
			m_WheelState = STATE_LOCKED;
			m_fTimeLeftInState = 0;
			SCREENMAN->PlayStartSound();
			m_fLockedWheelVelocity = 0;

			// Send this again so the screen starts sample music.
			SCREENMAN->PostMessageToTopScreen(SM_SongChanged, 0);
			MESSAGEMAN->Broadcast("RouletteStopped");
		}
		else
		{
			--m_iSwitchesLeftInSpinDown;
			const float SwitchTimes[] = { 0.5f, 1.3f, 0.8f, 0.4f, 0.2f };
			ASSERT(m_iSwitchesLeftInSpinDown >= 0 && m_iSwitchesLeftInSpinDown <= 4);
			m_fTimeLeftInState = SwitchTimes[m_iSwitchesLeftInSpinDown];
			m_Moving = 0;

			LOG->Trace("m_iSwitchesLeftInSpinDown id %d, m_fTimeLeftInState is %f", m_iSwitchesLeftInSpinDown, m_fTimeLeftInState);

			if (m_iSwitchesLeftInSpinDown == 0)
				ChangeMusic(randomf(0, 1) >= 0.5f ? 1 : -1);
			else
				ChangeMusic(1);
		}
		break;
	default:
		FAIL_M(ssprintf("Invalid wheel state: %i", m_WheelState));
	}
}

void MusicWheel::ChangeMusic(int iDist)
{
	m_iSelection += iDist;
	wrap(m_iSelection, m_CurWheelItemData.size());

	if (REMIND_WHEEL_POSITIONS && HIDE_INACTIVE_SECTIONS)
	{
		// store the group song index
		for (unsigned idx = 0; idx < m_viWheelPositions.size(); idx++)
		{
			if (m_sExpandedSectionName == SONGMAN->GetSongGroupByIndex(idx))
			{
				m_viWheelPositions[idx] = m_iSelection;
			}
		}
	}

	RebuildWheelItems(iDist);
	//m_fPositionOffsetFromSelection += iDist;
	m_fPositionOffsetFromSelection = iDist;
	SetPositions(iDist);

	SCREENMAN->PostMessageToTopScreen(SM_SongChanged, 0);

	// xMAx - Mensajes de Next y Previous Song
	if (iDist > 0)
		MESSAGEMAN->Broadcast("NextSong");
	else
		MESSAGEMAN->Broadcast("PreviousSong");

	// If we're moving automatically, don't play this; it'll be called in Update.
	//if(!IsMoving()) //xMAx - Se reproduce el sonido sin condici�n
	if (m_soundChangeMusic.IsPlaying())
	{
		m_soundChangeMusic.Stop();
	}

	m_soundChangeMusic.Play(true);
}

void MusicWheel::OpenGroupOrChangeSort(RString sChannel)
{
	// xMAx
	SortOrder SO = StringToSortOrder(sChannel);
	if (SO != SortOrder_Invalid && (sChannel.find("SO_") != std::string::npos || sChannel == "AllCourses"))
	{
		ChangeSort(SO);
		return;
	}

	if (SONGMAN->DoesSongGroupExist(sChannel))
	{
		GAMESTATE->m_SortOrder.Set(SORT_GROUP);
		GAMESTATE->m_PreferredSortOrder = SORT_GROUP;
		GAMESTATE->m_sCurrentGroupName = sChannel;	//esto se est� realizando en SSM tambien - xMAx 
		SetOpenSection(sChannel);
	}
}

bool MusicWheel::ChangeSort(SortOrder new_so, bool allowSameSort)	// return true if change successful
{
	ASSERT_M(new_so < NUM_SortOrder, "MusicWheel::Invalid sort order");

	if (IsSongSort(new_so) || new_so == SORT_ALL_COURSES)
	{
		GAMESTATE->m_PreferredSortOrder = new_so;
		GAMESTATE->m_SortOrder.Set(new_so);
	}
	else
		return false;


	// Clears current wheel items data
	m_CurWheelItemData.clear();

	// Gets new items data according to the selected sort order
	vector<MusicWheelItemData*> from;
	getWheelItemsData(new_so, from);
	m_CurWheelItemData.reserve(from.size());
	for (unsigned i = 0; i < from.size(); ++i)
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
	SCREENMAN->PostMessageToTopScreen(SM_SortOrderChanged, 0);

	return true;
}

bool MusicWheel::NextSort()		// return true if change successful
{
	// don't allow NextSort when on the mode menu
	if (GAMESTATE->m_SortOrder == SORT_MODE_MENU)
		return false;

	vector<SortOrder> aSortOrders;
	{
		Lua* L = LUA->Get();
		SORT_ORDERS.PushSelf(L);
		FOREACH_LUATABLEI(L, -1, i)
		{
			SortOrder so = Enum::Check<SortOrder>(L, -1, true);
			aSortOrders.push_back(so);
		}
		lua_pop(L, 1);
		LUA->Release(L);
	}

	// find the index of the current sort
	int cur = 0;
	while (cur < int(aSortOrders.size()) && aSortOrders[cur] != GAMESTATE->m_SortOrder)
		++cur;

	// move to the next sort with wrapping
	++cur;
	wrap(cur, aSortOrders.size());

	// apply new sort
	SortOrder soNew = aSortOrders[cur];
	return ChangeSort(soNew);
}

bool MusicWheel::Select()	// return true if this selection ends the screen
{
	LOG->Trace("MusicWheel::Select()");

	switch (m_WheelState)
	{
	case STATE_FLYING_OFF_BEFORE_NEXT_SORT:
	case STATE_ROULETTE_SLOWING_DOWN:
		return false;
	case STATE_ROULETTE_SPINNING:
		m_WheelState = STATE_ROULETTE_SLOWING_DOWN;
		m_iSwitchesLeftInSpinDown = ROULETTE_SLOW_DOWN_SWITCHES / 2 + 1 + RandomInt(ROULETTE_SLOW_DOWN_SWITCHES / 2);
		m_fTimeLeftInState = 0.1f;
		return false;
	case STATE_RANDOM_SPINNING:
		m_fPositionOffsetFromSelection = max(m_fPositionOffsetFromSelection, 0.3f);
		m_WheelState = STATE_LOCKED;
		SCREENMAN->PlayStartSound();
		m_fLockedWheelVelocity = 0;
		// Set m_Moving to zero to stop the sounds from playing.
		m_Moving = 0;
		SCREENMAN->PostMessageToTopScreen(SM_SongChanged, 0);
		return true;
	default: break;
	}

	if (!WheelBase::Select())
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
	*/ // xMAx
}

void MusicWheel::StartRoulette()
{
	MESSAGEMAN->Broadcast("StartRoulette");
	m_WheelState = STATE_ROULETTE_SPINNING;
	m_Moving = 1;
	m_TimeBeforeMovingBegins = 0;
	m_SpinSpeed = 1.0f / ROULETTE_SWITCH_SECONDS;
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
	if (RANDOM_PICKS_LOCKED_SONGS)
	{
		// Shuffle and use the roulette wheel.
		RandomGen rnd;
		vector<MusicWheelItemData*> from;
		getWheelItemsData(SORT_ROULETTE, from);
		random_shuffle(from.begin(), from.end(), rnd);
		GAMESTATE->m_SortOrder.Set(SORT_ROULETTE);
	}
	else
	{
		GAMESTATE->m_SortOrder.Set(GAMESTATE->m_PreferredSortOrder);
	}
	SetOpenSection("");

	m_Moving = -1;
	m_TimeBeforeMovingBegins = 0;
	m_SpinSpeed = 1.0f / ROULETTE_SWITCH_SECONDS;
	m_SpinSpeed *= 20.0f; /* faster! */
	m_WheelState = STATE_RANDOM_SPINNING;

	SelectSong(GetPreferredSelectionForRandomOrPortal());

	RebuildWheelItems();
}

void MusicWheel::SetOpenSection(RString group)
{
	LOG->Trace("MusicWheel::SetOpenSection %s", group.c_str());
	m_sExpandedSectionName = group;
	GAMESTATE->sExpandedSectionName = group;

	// Clears current wheel items data
	m_CurWheelItemData.clear();

	// Gets new items data according to the group opened
	vector<MusicWheelItemData*> from;
	getWheelChannelItemsData(group, from);
	LOG->Trace("MusicWheel::SetOpenSection - Got %d items from getWheelChannelItemsData", from.size());

	// If we got no items, try to find a valid group
	if (from.empty() && !group.empty())
	{
		LOG->Trace("MusicWheel::SetOpenSection - No items found, trying to find valid group");
		vector<RString> vsAvailableGroups;
		SONGMAN->GetSongGroupNamesAvailables(vsAvailableGroups);

		if (!vsAvailableGroups.empty())
		{
			// Try to find a non-sort order group first
			for (unsigned i = 0; i < vsAvailableGroups.size(); i++)
			{
				if (vsAvailableGroups[i].find("SO_") == std::string::npos)
				{
					RString newGroup = vsAvailableGroups[i];
					LOG->Trace("MusicWheel::SetOpenSection - Trying alternative group: %s", newGroup.c_str());
					getWheelChannelItemsData(newGroup, from);
					if (!from.empty())
					{
						m_sExpandedSectionName = newGroup;
						GAMESTATE->sExpandedSectionName = newGroup;
						LOG->Trace("MusicWheel::SetOpenSection - Successfully switched to group: %s", newGroup.c_str());
						break;
					}
				}
			}

			// If still no items, try the first available group
			if (from.empty())
			{
				RString newGroup = vsAvailableGroups[0];
				LOG->Trace("MusicWheel::SetOpenSection - Trying first available group: %s", newGroup.c_str());
				getWheelChannelItemsData(newGroup, from);
				if (!from.empty())
				{
					m_sExpandedSectionName = newGroup;
					GAMESTATE->sExpandedSectionName = newGroup;
					LOG->Trace("MusicWheel::SetOpenSection - Successfully switched to first available group: %s", newGroup.c_str());
				}
			}
		}
	}

	m_CurWheelItemData.reserve(from.size());
	for (unsigned i = 0; i < from.size(); ++i)
	{
		m_CurWheelItemData.push_back(from[i]);
	}

	// Restarts selection
	m_iSelection = 0;

	// Rebuilds items
	RebuildWheelItems();

	// Set the position of each item on the wheel (no moving)
	SetPositions();

	LOG->Trace("MusicWheel::SetOpenSection - Finished, final size: %d", m_CurWheelItemData.size());
}


// sm-ssc additions: jump to group
RString MusicWheel::JumpToNextGroup()
{
	// Thanks to Juanelote for this logic:
	if (HIDE_INACTIVE_SECTIONS)
	{
		//todo: make it work with other sort types
		unsigned iNumGroups = SONGMAN->GetNumSongGroups();

		for (unsigned i = 0; i < iNumGroups; i++)
		{
			if (m_sExpandedSectionName == SONGMAN->GetSongGroupByIndex(i))
			{
				if (i < iNumGroups - 1)
					return SONGMAN->GetSongGroupByIndex(i + 1);
				else
				{
					//i = 0;
					return SONGMAN->GetSongGroupByIndex(0);
				}
			}
		}

		// If we didn't find the current group, return the first available group
		if (iNumGroups > 0)
			return SONGMAN->GetSongGroupByIndex(0);
	}
	else
	{
		unsigned int iLastSelection = m_iSelection;
		for (unsigned int i = m_iSelection; i < m_CurWheelItemData.size(); ++i)
		{
			if (m_CurWheelItemData[i]->m_Type == WheelItemDataType_Section && i != (unsigned int)m_iSelection)
			{
				m_iSelection = i;
				return m_CurWheelItemData[i]->m_sText;
			}
		}
		// it should not get down here, but it might happen... only search up to
		// the previous selection.
		for (unsigned int i = 0; i < iLastSelection; ++i)
		{
			if (m_CurWheelItemData[i]->m_Type == WheelItemDataType_Section && i != (unsigned int)m_iSelection)
			{
				m_iSelection = i;
				return m_CurWheelItemData[i]->m_sText;
			}
		}
	}

	// If we still don't have a valid group, try to get the first available group
	vector<RString> vsAvailableGroups;
	SONGMAN->GetSongGroupNamesAvailables(vsAvailableGroups);
	if (!vsAvailableGroups.empty())
	{
		LOG->Trace("MusicWheel::JumpToNextGroup - No valid group found, using first available: %s", vsAvailableGroups[0].c_str());
		return vsAvailableGroups[0];
	}

	// Last resort: return current expanded section name if it's not empty
	if (!m_sExpandedSectionName.empty())
	{
		LOG->Trace("MusicWheel::JumpToNextGroup - Using current expanded section: %s", m_sExpandedSectionName.c_str());
		return m_sExpandedSectionName;
	}

	LOG->Warn("MusicWheel::JumpToNextGroup - No valid group found, returning empty string");
	return "";
}

RString MusicWheel::JumpToPrevGroup()
{
	if (HIDE_INACTIVE_SECTIONS)
	{
		unsigned iNumGroups = SONGMAN->GetNumSongGroups();

		for (unsigned i = 0; i < iNumGroups; i++)
		{
			if (m_sExpandedSectionName == SONGMAN->GetSongGroupByIndex(i))
			{
				if (i > 0)
					return SONGMAN->GetSongGroupByIndex(i - 1);
				else
				{
					//i = iNumGroups - 1;
					return SONGMAN->GetSongGroupByIndex(iNumGroups - 1);
				}
			}
		}

		// If we didn't find the current group, return the last available group
		if (iNumGroups > 0)
			return SONGMAN->GetSongGroupByIndex(iNumGroups - 1);
	}
	else
	{
		for (unsigned int i = m_iSelection; i > 0; --i)
		{
			if (m_CurWheelItemData[i]->m_Type == WheelItemDataType_Section && i != (unsigned int)m_iSelection)
			{
				m_iSelection = i;
				return m_CurWheelItemData[i]->m_sText;
			}
		}
		// in case it wasn't found above:
		for (unsigned int i = m_CurWheelItemData.size() - 1; i > 0; --i)
		{
			LOG->Trace("JumpToPrevGroup iteration 2 | i = %u", i);
			if (m_CurWheelItemData[i]->m_Type == WheelItemDataType_Section)
			{
				m_iSelection = i;
				LOG->Trace("finding it in #2 | i = %u | text = %s", i, m_CurWheelItemData[i]->m_sText.c_str());
				return m_CurWheelItemData[i]->m_sText;
			}
		}
	}

	// If we still don't have a valid group, try to get the last available group
	vector<RString> vsAvailableGroups;
	SONGMAN->GetSongGroupNamesAvailables(vsAvailableGroups);
	if (!vsAvailableGroups.empty())
	{
		LOG->Trace("MusicWheel::JumpToPrevGroup - No valid group found, using last available: %s", vsAvailableGroups.back().c_str());
		return vsAvailableGroups.back();
	}

	// Last resort: return current expanded section name if it's not empty
	if (!m_sExpandedSectionName.empty())
	{
		LOG->Trace("MusicWheel::JumpToPrevGroup - Using current expanded section: %s", m_sExpandedSectionName.c_str());
		return m_sExpandedSectionName;
	}

	LOG->Warn("MusicWheel::JumpToPrevGroup - No valid group found, returning empty string");
	return "";
}

// Called on late join. Selectable courses may have changed; reopen the section.
void MusicWheel::PlayerJoined()
{
	// Release all the cached/stored items when a new player joins the game
	ReleaseWheelItems();

	// Re-open the last group or sort to update the songs list
	OpenGroupOrChangeSort(GAMESTATE->m_sCurrentGroupName);

	// If someone joins, there may be songs on the wheel that should not be
	// selectable, or there may be songs that have become selectable.
	// Set the status of all the wheel item data vectors to invalid so that
	// readyWheelItemsData will rebuild all the data next time
	// getWheelItemsData is called for that SortOrder.  SetOpenSection calls
	// readyWheelItemsData to get the items, and RebuildWheelItems when its
	// done, so invalidating and calling SetOpenSection is all we need to do.
	// -Kyz
	// Also removed the weird checks for course mode and autogen because
	// it seems weird that courses wouldn't also be affected by a player
	// joining, and not doing it in autogen causes other weird problems. -Kyz
	//FOREACH_ENUM(SortOrder, so)
	//{
	//	m_WheelItemDatasStatus[so] = INVALID;
	//}
	//SetOpenSection(m_sExpandedSectionName);
}

bool MusicWheel::IsRouletting() const
{
	return m_WheelState == STATE_ROULETTE_SPINNING || m_WheelState == STATE_ROULETTE_SLOWING_DOWN ||
		m_WheelState == STATE_RANDOM_SPINNING;
}

Song* MusicWheel::GetSelectedSong()
{
	switch (m_CurWheelItemData[m_iSelection]->m_Type)
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
Song* MusicWheel::GetPreferredSelectionForRandomOrPortal()
{
	// probe to find a song that has the preferred
	// difficulties of each player
	vector<Difficulty> vDifficultiesToRequire;
	FOREACH_HumanPlayer(p)
	{
		if (GAMESTATE->m_PreferredDifficulty[p] == Difficulty_Invalid)
			continue;	// skip

		// TRICKY: Don't require that edits be present if perferred
		// difficulty is Difficulty_Edit.  Otherwise, players could use this
		// to set up a 100% chance of getting a particular locked song by
		// having a single edit for a locked song.
		if (GAMESTATE->m_PreferredDifficulty[p] == Difficulty_Edit)
			continue;	// skip

		vDifficultiesToRequire.push_back(GAMESTATE->m_PreferredDifficulty[p]);
	}

	RString sPreferredGroup = m_sExpandedSectionName;
	vector<MusicWheelItemData*> wid;
	getWheelItemsData(GAMESTATE->m_SortOrder, wid);

	StepsType st = GAMESTATE->GetCurrentStyle(PLAYER_INVALID)->m_StepsType;

#define NUM_PROBES 1000
	for (int i = 0; i < NUM_PROBES; i++)
	{
		bool isValid = true;
		/* Maintaining difficulties is higher priority than maintaining
		 * the current group. */
		if (i == NUM_PROBES / 4)
			sPreferredGroup = "";
		if (i == NUM_PROBES / 2)
			vDifficultiesToRequire.clear();

		int iSelection = RandomInt(wid.size());
		if (wid[iSelection]->m_Type != WheelItemDataType_Song)
			continue;

		const Song* pSong = wid[iSelection]->m_pSong;

		if (!sPreferredGroup.empty() && wid[iSelection]->m_sText != sPreferredGroup)
			continue;

		// There's an off possibility that somebody might have only one song with only beginner steps.
		if (i < 900 && pSong->IsTutorial())
			continue;

		isValid = std::none_of(vDifficultiesToRequire.begin(), vDifficultiesToRequire.end(), [&](Difficulty const& d) {
			return !pSong->HasStepsTypeAndDifficulty(st, d);
			});

		if (isValid)
		{
			return wid[iSelection]->m_pSong;
		}
	}
	LuaHelpers::ReportScriptError("Couldn't find any songs");
	return wid[0]->m_pSong;
}

void MusicWheel::FinishChangingSorts()
{
	FinishTweening();
	m_WheelState = STATE_SELECTING;
	m_fTimeLeftInState = 0;
}

// lua start
#include "LuaBinding.h"

class LunaMusicWheel : public Luna<MusicWheel>
{
public:
	static int ChangeSort(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1)) { lua_pushboolean(L, false); }
		else
		{
			SortOrder so = Enum::Check<SortOrder>(L, 1);
			lua_pushboolean(L, p->ChangeSort(so));
		}
		return 1;
	}
	DEFINE_METHOD(GetSelectedSection, GetSelectedSection());
	//static int GetCurrentSections( T* p, lua_State *L )
	//{
	//	vector<RString> v;
	//	p->GetCurrentSections(v);
	//	LuaHelpers::CreateTableFromArray<RString>( v, L );
	//	return 1;
	//}
	static int IsRouletting(T* p, lua_State* L) { lua_pushboolean(L, p->IsRouletting()); return 1; }
	static int SelectSong(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1)) { lua_pushboolean(L, false); }
		else
		{
			Song* pS = Luna<Song>::check(L, 1, true);
			lua_pushboolean(L, p->SelectSong(pS));
		}
		return 1;
	}
	static int SelectCourse(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1)) { lua_pushboolean(L, false); }
		else
		{
			Course* pC = Luna<Course>::check(L, 1, true);
			lua_pushboolean(L, p->SelectCourse(pC));
		}
		return 1;
	}

	static int Move(T* p, lua_State* L)
	{
		if (lua_isnil(L, 1)) { p->Move(0); }
		else
		{
			p->Move(IArg(1));
		}
		return 1;
	}

	LunaMusicWheel()
	{
		ADD_METHOD(ChangeSort);
		ADD_METHOD(GetSelectedSection);
		ADD_METHOD(IsRouletting);
		ADD_METHOD(SelectSong);
		ADD_METHOD(SelectCourse);
		ADD_METHOD(Move);
		//ADD_METHOD( GetCurrentSections );
	}
};

LUA_REGISTER_DERIVED_CLASS(MusicWheel, WheelBase)
// lua end

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
