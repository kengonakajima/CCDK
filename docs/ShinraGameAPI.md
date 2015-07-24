# Game API

The Game API from Shinra Technologies is intend to provide complementary functionalities
over the standard Windows API already wrapped by Shinra.  Actually, the Shinra API is only
required, and useful, for 1:N games, but some functionalities could later be added that could
benefits any game.

## Using the Game API

The API is fully documented inside the C++ header file [`ShinraGame.h`](ShinraGame.h).

### The Game loop

A basic example of a normal game loop with Shinra is the following:
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

### Using the API with XAudio2

The `IMMDevice` provide by the Shinra API is compatible with the low-level `CoreAudio` API from
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

## Linking the Game API

The whole API is include in `CloudCoreClient.dll`, and we provide a library stub in
`CloudCoreClient.lib` for linking.  Just add the right version (x32 for 32 bits applications,
and x64 for 64 bits application) to your linker and it's ready.  Note that you must not provided
your own version of the library in our deployment.  Our service will always deployed your game
with the latest version of our libraries.

## FAQ

1. Do I have to use the Game API for my single player game ?

  No, the Game API is only necessary for accessing functionality specific to Shinra Technologies.
  A single player game only required to run on Windows platform using DirectX11 and other Windows
  API supported by Shinra Technologies.
