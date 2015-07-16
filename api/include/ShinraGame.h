/*
**  ShinraGame.h - Shinra Game API header file.
**
**  Copyright (c) Shinra Technologies. All rights reserved.
*/

#pragma once


#include <cstdint>
#include <string>

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#ifndef VC_EXTRALEAN
	#define VC_EXTRALEAN
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#ifndef NOSERVICE
	#define NOSERVICE
#endif
#ifndef NOIME
	#define NOIME
#endif
#ifndef NOMCX
	#define NOMCX
#endif
#include <Windows.h>


#if defined(_MSC_VER) || (defined(__GNUC__) && defined(_WIN32))
	#define SHINRA_DLLEXPORT    __declspec(dllexport)
	#define SHINRA_DLLIMPORT    __declspec(dllimport)
#elif defined(__MACH__) || defined(__linux__)
	#define SHINRA_DLLEXPORT     __attribute__ ((visibility("default")))
	#define SHINRA_DLLIMPORT    
#else
	#define SHINRA_DLLEXPORT
	#define SHINRA_DLLIMPORT
#endif


#ifdef SHINRAGAME_EXPORT
	#define SHINRAGAME_API      SHINRA_DLLEXPORT
#else
	#define SHINRAGAME_API      SHINRA_DLLIMPORT
#endif


struct IMMDevice;
struct ID3D11DeviceContext;


namespace shinra
{


/*
**  PlayerID
**
**  Unique for each currently connected client.
**
*/

typedef std::uint32_t   PlayerID;


/*
**  PlayerID Constants
*/

enum : PlayerID
{
	PI_INVALID_PLAYER   = 0xffffffff    // Player ID which is guaranteed not to be used to identify an actual player
};


/*
**  EventType
**
**  The type of events.  New events can be added anytime in the life of the API.
*/

enum EventType
{
	ET_PLAYER_LOGGED_IN,                // When a new player get connected.
	ET_PLAYER_LOGGED_OFF                // When a player get disconnected.
};


/*
**  Event
**
**  The structure for the event details.
*/

struct SHINRAGAME_API Event
{
	EventType   type;                   // The type of the event.
	PlayerID    playerID;               // The player affected by the event.
};


/*
**  GetNextEvent
**
**  Returns a pointer to the next event in the event queue. If the event queue
**  is empty, the return value is null.
**
**  The returned pointer must NOT be deleted. Each pointer is guaranteed to
**  stay valid until the next call to the function.
**
**  Events polling example:
**
**      for (Event* event = GetNextEvent(); event; event = GetNextEvent())
**      {
**          switch (event->type)
**          {
**              // handle event
**          }
**      }
*/

SHINRAGAME_API Event* GetNextEvent();

/*
**  GetPlayerGamepadID
**
**  Can be used with XInput API functions in place of the user index parameter.
**  Example:
**
**      if (Event* event = GetNextEvent())
**      {
**          if (event->type == ET_PLAYER_LOGGED_IN)
**          {
**              DWORD gamepadId = GetPlayerGamepadID(event->playerID);
**              XINPUT_STATE state;
**              XInputGetState(gamepadID, &state);
**          }
**      }

*/

SHINRAGAME_API ::DWORD GetPlayerGamepadID(PlayerID playerID);

/*
**  GetPlayerIDFromRawInputDevice
**
**  Return the PlayerId associated with a RAWINPUT device handle.
**  Example:
**
**    if(GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pRawInput, &dwSize, sizeof(RAWINPUTHEADER)) > 0)
**    {
**       PlayerId playerID = GetPlayerIDFromRawInputHandle(pRawInput->header.hDevice);
**       if (playerID != PI_INVALID_ID)
**       {
**           // Handle input data for this player.
**       }
**    }
*/

SHINRAGAME_API PlayerID GetPlayerIDFromRawInputDevice(HANDLE hRawInputDevice);

/*
**  GetPlayerAudioDeviceID
**
**  Returns the IMMDevice ID for the player device, without instantiating one.
**  Can be used with the IMMDeviceEnumerator::GetDevice() method.
*/

SHINRAGAME_API std::wstring GetPlayerAudioDeviceID(PlayerID playerID);

/*
**  GetPlayerAudioDevice
**
**  Returns the IMMDevice associate with the player.  Creates it if it doesn't exist.
*/

SHINRAGAME_API IMMDevice* GetPlayerAudioDevice(PlayerID playerID);

/*
**  GetPlayerRenderingContext
**
**  That's where the game renders client viewport
*/

SHINRAGAME_API ID3D11DeviceContext* GetPlayerRenderingContext(PlayerID playerID);

} // namespace shinra

