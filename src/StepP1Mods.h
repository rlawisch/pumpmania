/* StepP1Mods - the Pump it Up mod model of STEP_P1.exe: every option is a (category, item) pair. */

#ifndef STEPP1_MODS_H
#define STEPP1_MODS_H

#include "PlayerNumber.h"

class PlayerOptions;

namespace StepP1Mods
{
	enum Category
	{
		CAT_SPEED,		// 1x..6x, +0.25, +0.5, EW, RV, AV, AC, DC
		CAT_AV,			// +100, +10, +1, -1, -10, -100
		CAT_DISPLAY,	// hidden, sudden, stealth, dark, blink, random/auto/roulette noteskin, BGA off, BGA dark
		CAT_NOTESKIN,
		CAT_PATH,		// xmode, NX, under attack, drop, bumpy +/-, tornado, minis
		CAT_ALTERNATE,	// mirror, random
		CAT_JUDGE,		// EJ, HJ, VJ, XJ, UJ, judgment reverse, judge by note, free performance
		CAT_RATE,		// 0.8..1.5
		CAT_BATTLE,
		CAT_STEPSTYPES,	// show singles / half doubles / doubles
		CAT_RESET,
		NUM_CATEGORIES
	};

	struct NoteSkinEntry
	{
		RString sName;
		short iIcon;	// frame in OptionList/icons 11x11
		short iText;	// frame in OptionList/Texts 8x16
		bool bHasIcon;	// false: not in icons 11x11, use the skin's NOTE_LOGO.png
	};

	// Built like NoteSkinManager in the exe (0x5f3d59): a fixed table plus any other installed pump noteskin.
	const vector<NoteSkinEntry> &GetNoteSkins();

	int GetNumItems( int iCategory );
	int GetIcon( int iCategory, int iItem );
	int GetText( int iCategory, int iItem );
	// Recreated from 0x4a1252 / 0x4a1403: icons of the active items, and of every item.
	void GetActiveIcons( const PlayerOptions &po, int iCategory, vector<int> &vOut );
	void GetAllIcons( int iCategory, vector<int> &vOut );

	// Recreated from 0x49ef58. bOn is false when the item was turned off;
	// bSpecial is set for items that can't be picked from here.
	void Apply( PlayerOptions &po, int iCategory, int iItem, bool &bOn, bool &bSpecial );
	// Applies to every mods level of the player, like PO_GROUP_CALL in the exe.
	void ApplyToPlayer( PlayerNumber pn, int iCategory, int iItem, bool &bOn, bool &bSpecial );
	// Recreated from 0x49e36b.
	bool IsActive( const PlayerOptions &po, int iCategory, int iItem );
	// Categories that change song or game state instead of the player's options.
	bool IsGlobal( int iCategory, int iItem );
	// Recreated from 0x54df34: back to the default options.
	void Reset( PlayerNumber pn, bool &bStepsTypesChanged );
}

#endif
