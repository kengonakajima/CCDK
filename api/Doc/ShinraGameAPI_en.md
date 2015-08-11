# Game API

The Game API from Shinra Technologies is intend to provide complementary functionalities
over the standard Windows API already wrapped by Shinra.  Actually, the Game API is only
required, and useful, for 1:N games, but some functionalities could later be added that could
benefits any game.

## Using the Game API

The API is fully documented inside the C++ header file [`ShinraGame.h`](../Sources/include/ShinraGame.h).

### The Game loop

A basic example of a normal game loop with Shinra Technologies is the following:
```
#include <ShinraGame.h>

int main()
{
    MyGame game;
    game.Initialize(); // Must create the D3D11 swap chain here.
    while (game.IsRunning())
    {
        shinra::Event* event = GetNextEvent();
        if (event != nullptr)
        {
            switch(event->type)
            {
                case shinra::ET_PLAYER_LOGGED_IN:
                {
                    game.AddPlayer(event->playerID);
                } break;
                case shinra::ET_PLAYER_LOGGED_OFF:
                {
                    game.RemovePlayer(event->playerID);
                } break;
                default:
                {
                    // Ignore event for forward compatibility.
                }
            }
        }
        game.Update();
    }
}
```

### Adding a player

The method `MyGame::AddPlayer()` should add the player avatar to the game.  The `playerID` member of
the `ET_PLAYER_LOGGED_IN` event is used for accessing the different devices used by your player:
```
#include <ShinraGame.h>

void MyGame::AddPlayer(shinra::PlayerID playerID)
{
  MyPlayer* player = new MyPlayer(playerID);
  
  // Set specific rendering context.
  ID3D11DeviceContext* d3dContext = shinra::GetPlayerRenderingContext(playerID);
  assert(d3dContext != nullptr);  
  player->SetD3D11ImmediateContext(d3dContext);
  
  // Set audio device to output.
  IMMDevice* audioDevice = shinra::GetPlayerAudioDevice(playerID);
  assert(audioDevice != nullptr);  
  player->SetMMAudioDevice(audioDevice);
  
  DWORD gamepadID = shinra::GetPlayerGamepadID(playerID);
  player->SetXInputGamepadID(gamepadID);
  
  AddPlayerToUpdateLoop(player);
}
```

### Removing the player

The method `MyGame::RemovePlayer()` should removed the player avatar to the game and free the associated
resources.

```
#include <ShinraGame.h>

void MyGame::RemovePlayer(shinra::PlayerID playerID)
{
  MyPlayer* player = RemovePlayerFromUpdateLoop(playerID);
  if (player == nullptr)
  {
    return;
  }
  
  // Set specific rendering context.
  ID3D11DeviceContext* d3dContext = player->GetD3D11ImmediateContext();
  assert(d3dContext != nullptr);  
  d3dContext->Release();
  
  // Set audio device to output.
  IMMDevice* audioDevice = player->GetMMAudioDevice();
  assert(audioDevice != nullptr);  
  audioDevice->Release();

  delete player;
}
```

### Using the D3D11 Device Context

The `ID3D11Device` and `ID3D11DeviceContext` created from the DXGI API (either with `D3D11CreateDeviceAndSwapChain`,
`D3D11CreateDevice` or `ID3D11Device::CreateSwapChain`) can be used to create all resources on all player contexts
and modifying them in batch. However, most `ID3D11DeviceContext` read functions aren't actually supported, and even
if a function seems to work actually, it's behavior is undefined and will likely to change in a future version of
the API.  For using those functions, you should used the device context return from the
`shinra::GetPlayerRenderingContext` API with the user you want to check.


### Using the API with XAudio2

The `IMMDevice` provide by the Game API is compatible with the low-level `CoreAudio` API from
Windows.  However, you can use an higher level API like
[XAudio2](https://msdn.microsoft.com/en-us/library/windows/desktop/hh405049%28v=vs.85%29.aspx).
You simply need to create the XAudio2 MasterVoice using the audio device ID of the player:

```
  ...
  HRESULT hr = S_OK;

  IXAudio2* xaudio2 = nullptr;
  hr = XAudio2Create( &xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR ) );
  assert(SUCCEEDED(hr));
  
  const std::wstring audioDeviceID = shinra::GetPlayerAudioDeviceID(playerID);
  IXAudio2MasteringVoice* masterVoice = nullptr;
  hr = pXAudio2->CreateMasteringVoice( 
                                      &masterVoice,
                                      XAUDIO2_DEFAULT_CHANNELS,
                                      XAUDIO2_DEFAULT_SAMPLERATE,
                                      0,
                                      audioDeviceID.c_str());
  assert(SUCCEEDED(hr));
  
  ... // Continue with initialization.
  
```

## Registering RawInput Device for Keyboard & Mouse

You can access individual players keyboard and mouse input using the
[Windows RawInput API](https://msdn.microsoft.com/en-us/library/windows/desktop/ms645536%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396).
As the normal Raw Input usage, you first need to call the 
[RegisterRawInputDevices](https://msdn.microsoft.com/en-us/library/windows/desktop/ms645600%28v=vs.85%29.aspx)
API to register the type of devices you would like to listen to.  You just need to do it once to
receive the events from all players. For example, the following code allow you to listen to both
keyboard and mouse events.  You


```
  RAWINPUTDEVICE  rids[2] = { 0 };
  
  // Mouse
  rid[0].usUsagePage = 0x01;        // Generic Desktop Page TLC
  rid[0].usUsage = 0x02;            // Pointer Usage ID
  rid[0].dwFlags = 0;
  rid[0].hwndTarget = windowHandle; // NULL for default window
  
  // Keyboard
  rid[1].usUsagePage = 0x01;        // Generic Desktop Page TLC
  rid[1].usUsage = 0x06;            // Keyboard Usage ID
  rid[1].dwFlags = RIDEV_NOLEGACY;  // do not generate legacy messages such as WM_KEYDOWN
  rid[1].hwndTarget = windowHandle; // NULL for default window
  
  BOOL res = RegisterRawInputDevices(&rids, 2, sizeof(rids));
  assert(res);
```

Once a player get connected, you will start getting WM_INPUT events.  Process it as usual to get
`RAWINPUT` data and used the `shinra::GetPlayerIDFromRawInputDevice` API for finding the PlayerID
associated the event:

```
LRESULT MyGameWindowProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	switch (nMsg)
	{
	...
	case WM_INPUT:
		{
			UINT dwSize = sizeof(RAWINPUT);
			
			char buffer[sizeof(RAWINPUT)] = {0};
			
			UINT res = GetRawInputData((HRAWINPUT)(lParam), RID_INPUT, buffer, &dwSize, sizeof(RAWINPUTHEADER));
			
			if (res == (UINT)(-1)) 
			  break; 

			// extract keyboard raw input data
			RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer);
			shinra::PlayerID playerID = shinra::GetPlayerIDFromRawInputDevice(raw->header.hDevice);

      if (playerID != shinra::PI_INVALID_PLAYER)
      {
        // Handle RawInput data for this player id.
        MyGame::GetPlayer(playerID).HandleRawInputData(raw);      
      }
    }
	// ... Handle other messages.		
	}

	return DefWindowProc(hWnd, nMsg, wParam, lParam);
}
```

## Linking the Game API

The whole API is include in `CloudCoreClient.dll`, and we provide a library stub in
`CloudCoreClient.lib` for linking.  Just add the right version (x32 for 32 bits applications,
`CloudCoreClient.lib` for linking.  Just add the right version (x32 for 32 bits applications,
and x64 for 64 bits application) to your linker and it's ready.  Note that you must not provided
your own version of the library in our deployment.  Our service will always deployed your game
with the latest version of our libraries.

## FAQ

1. Do I have to use the Game API for my single player game ?

  No, the Game API is only necessary for accessing functionality specific to Shinra Technologies.
  A single player game only required to run on Windows platform using DirectX11 and other Windows
  API supported by Shinra Technologies.
