// LightsDriver_PacDrive for use with a PacDrive hooked up with LEDs
// You need PacDrive32.dll in the StepMania directory to use this.

#include "global.h"
#include "LightsDriver_PacDrive.h"
#include "windows.h"
#include "RageUtil.h"
#include "Preference.h"

REGISTER_LIGHTS_DRIVER_CLASS(PacDrive);

HINSTANCE PachDLL = nullptr;

bool PacDriveConnected = false;
typedef int (WINAPI PacInitialize)(void);
PacInitialize* m_pacinit = nullptr;
typedef void (WINAPI PacShutdown)(void);
PacShutdown* m_pacdone = nullptr;
typedef bool (WINAPI PacSetLEDStates)(int, short int);
PacSetLEDStates* m_pacset = nullptr;

//Adds new preference to allow for different light wiring setups


LightsDriver_PacDrive::LightsDriver_PacDrive()
{
	// init io.dll
	PachDLL = LoadLibrary("pacdrive32.dll");
	if(PachDLL == nullptr)
	{
		MessageBox(nullptr, "Could not LoadLibrary( pacdrive32.dll ).", "ERROR", MB_OK );
		return;
	}

	//Get the function pointers
	m_pacinit = (PacInitialize*)GetProcAddress(PachDLL, "PacInitialize");
	m_pacset = (PacSetLEDStates*)GetProcAddress(PachDLL, "PacSetLEDStates");
	m_pacdone = (PacShutdown*)GetProcAddress(PachDLL, "PacShutdown");

	int NumPacDrives = 0;

	if (m_pacinit)
		NumPacDrives = m_pacinit(); //initialize the pac drive

	if (NumPacDrives == 0)
	{
		PacDriveConnected = false; // set not connected
		MessageBox(nullptr, "Could not find connected PacDrive.", "ERROR", MB_OK);
		return;
	}
	else
	{
		PacDriveConnected = true; // set connected
		m_pacset(0, 0x0);  // clear all lights for device i
	}
}

LightsDriver_PacDrive::~LightsDriver_PacDrive()
{
	if (PacDriveConnected && m_pacset)
		m_pacset(0, 0x0);  // clear all lights for device i

	if (m_pacdone)
		m_pacdone();

	FreeLibrary(PachDLL);
}

void LightsDriver_PacDrive::Set(const LightsState *ls)
{
	// Pump layout: 5 pad panels per player, then marquee and bass.
	static const GameButton pads[] = { PUMP_BUTTON_UPLEFT, PUMP_BUTTON_UPRIGHT, PUMP_BUTTON_CENTER, PUMP_BUTTON_DOWNLEFT, PUMP_BUTTON_DOWNRIGHT };
	short int outb = 0;
	for (int i = 0; i < 5; ++i)
	{
		if (ls->m_bGameButtonLights[GameController_1][pads[i]]) outb |= BIT(i);
		if (ls->m_bGameButtonLights[GameController_2][pads[i]]) outb |= BIT(5 + i);
	}
	if (ls->m_bCabinetLights[LIGHT_MARQUEE_UP_LEFT]) outb |= BIT(10);
	if (ls->m_bCabinetLights[LIGHT_MARQUEE_UP_RIGHT]) outb |= BIT(11);
	if (ls->m_bCabinetLights[LIGHT_MARQUEE_LR_LEFT]) outb |= BIT(12);
	if (ls->m_bCabinetLights[LIGHT_MARQUEE_LR_RIGHT]) outb |= BIT(13);
	if (ls->m_bCabinetLights[LIGHT_BASS_LEFT] || ls->m_bCabinetLights[LIGHT_BASS_RIGHT]) outb |= BIT(14);

	//ensure m_pacset function call was loaded.
	if (m_pacset)
		m_pacset(0, outb);
}

/* Modified 2015 Dave Barribeau for StepMania 5.09
* (c) 2003-2004 Chris Danford
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
