# Shinra Game API

Shinra Game APIは補完的な機能を提供するために用意され、ラップされたWindowsのスタンダードAPI上で動作します。

Shinra Game APIは１：Nゲームを製作する際のみ必要です。ただし、クラウドゲームの特性を引き出せるように、他のタイプのゲーム向けにも、後で機能が追加される可能性があります。



## Shinra Game API の使用方法


C++のヘッダ[`ShinraGame.h`](../include/ShinraGame.h).
 内に詳しいAPIのドキュメントがあります。
 ビルド時にShinra.hをincludeし、CloudCoreClient.libをリンクし、実行時に CloudCoreClient.DLLを読ませます。


### ゲームループ


下記はシンラシステム上での通常のゲームループの基本的な例です。

```
#include <ShinraGame.h>

int main()
{
    MyGame game;
    game.Initialize(); // D3D11のスワップチェインをここで作ります
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
                    // 将来の互換性のために無視しておく
                }
            }
        }
        game.Update();
    }
}
```

### プレイヤーの追加

MyGame::AddPlayer()メソッドは、プレイヤーアバターをゲームに追加します。
それぞれのプレイヤーのデバイスにアクセスするにはET_PLAYER_LOGGED_IN event において取得した、PlayerIDメンバを使用します。
次の例は、プレイヤーごとのレンダリングコンテキストとオーディオ出力デバイスをクラスのメンバに設定しています。


```
#include <ShinraGame.h>

void MyGame::AddPlayer(shinra::PlayerID playerID)
{
  MyPlayer* player = new MyPlayer(playerID);
  
  // プレイヤーごとのレンダリングコンテキストを設定
  ID3D11DeviceContext* d3dContext = shinra::GetPlayerRenderingContext(playerID);
  assert(d3dContext != nullptr);  
  player->SetD3D11ImmediateContext(d3dContext);
  
  // プレイヤーごとのオーディオ出力用デバイスを設定
  IMMDevice* audioDevice = shinra::GetPlayerAudioDevice(playerID);
  assert(audioDevice != nullptr);  
  player->SetMMAudioDevice(audioDevice);
  
  DWORD gamepadID = shinra::GetPlayerGamepadID(playerID);
  player->SetXInputGamepadID(gamepadID);
  
  AddPlayerToUpdateLoop(player);
}
```


### プレイヤーの削除

MyGame::RemovePlayer()メソッドはプレイヤーアバターをゲームから削除し、関連付けられたリソースを開放します。

次の例では、プレイヤーごとのレンダリングコンテキストとオーディオ出力用デバイスを解放しています。


```
#include <ShinraGame.h>

void MyGame::RemovePlayer(shinra::PlayerID playerID)
{
  MyPlayer* player = RemovePlayerFromUpdateLoop(playerID);
  if (player == nullptr)
  {
    return;
  }
  
  // レンダリングコンテキストを解放
  ID3D11DeviceContext* d3dContext = player->GetD3D11ImmediateContext();
  assert(d3dContext != nullptr);  
  d3dContext->Release();
  
  // オーディオ出力用デバイスを解放
  IMMDevice* audioDevice = player->GetMMAudioDevice();
  assert(audioDevice != nullptr);  
  audioDevice->Release();

  delete player;
}
```

### D3D11 Device Contextを使う

DXGI API経由で作成される ID3D11DeviceContext, ID3D11Deviceを使用してリソースを作成すると、すべてのプレイヤーに対して同じリソースが作成され、バッチで変更できます。

ただし、ID3D11DeviceContextのほとんどの読み込み関数は実装されていないか、実装されていたとしても動作は未定義です。
将来変更される可能性があります。

これらの(読み込み)関数を使用したい場合は、 shinra::GetPlayerRenderingContext関数を使って作られるデバイスコンテキストを使うようにしてください。

### XAudio2を使う

Shinra APIで提供されるIMMDeviceはwindowsのCore Audio APIと互換性があります。[XAudio2](https://msdn.microsoft.com/en-us/library/windows/desktop/hh405049%28v=vs.85%29.aspx) のような、よりハイレベルなAPIが使用可能です。XAudio2のMasterVoiceを、プレイヤーに応じたデバイスIDを使用して、作成する必要があるだけです。

次の例では、XAudio2のMasterVoiceを作成しています。


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
  
  ... // 初期化を続ける..
  
```

## キーボードとマウスのためにRawInputデバイスを登録する

Windows RawInput APIを使用して、各プレイヤーのキーボードとマウスにアクセスすることができます。
通常[Windows RawInput API](https://msdn.microsoft.com/en-us/library/windows/desktop/ms645536%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396) を使う方法と同じように、
最初にRegisterRawInputDevices APIをデータを取得したいデバイスのタイプを登録するために呼び出す必要があります。

以下はキーボードとマウスイベントの取得をするための例です:




```
  RAWINPUTDEVICE  rids[2] = { 0 };
  
  // Mouse
  rid[0].usUsagePage = 0x01;        // デスクトップタイプのアプリケーションであることを指定
  rid[0].usUsage = 0x02;            // ポインタの情報を取得するためのフラグ
  rid[0].dwFlags = 0;
  rid[0].hwndTarget = windowHandle; // デフォルトウインドウなのでNULL 
  // Keyboard
  rid[1].usUsagePage = 0x01;        // デスクトップタイプのアプリケーションであることを指定
  rid[1].usUsage = 0x06;            // キーボードの情報を取得するためのフラグ
  rid[1].dwFlags = RIDEV_NOLEGACY;  // WM_KEYDOWNのようなレガシーのイベントを受信しないようにするためのフラグ
  rid[1].hwndTarget = windowHandle; // デフォルトウインドウなのでNULL
  
  BOOL res = RegisterRawInputDevices(&rids, 2, sizeof(rids));
  assert(res);
```


一旦、プレイヤーが接続されると、WM_INPUTイベントを取得できるようになります。通常の手順でRAWINPUTデータを取得しshinra::GetPlayerIDFromRawInputDevice APIを使用して、関連付けられているPulayerIDを検索します。

次の例は、検索をしている部分です:

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

			// キーボードの入力を取り出す
			RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer);
			shinra::PlayerID playerID = shinra::GetPlayerIDFromRawInputDevice(raw->header.hDevice);

      if (playerID != shinra::PI_INVALID_PLAYER)
      {
        // プレイヤーごとに入力データを処理する
        MyGame::GetPlayer(playerID).HandleRawInputData(raw);      
      }
    }
	// ... 他のメッセージを処理
	}

	return DefWindowProc(hWnd, nMsg, wParam, lParam);
}
```

## Linking the Game API

全てのAPIはCloudCoreClient.dllに含まれています。
リンクするためのライブラリースタブは、CloudCoreClient.lib内に用意されています。
32bitアプリケーションにはX32、64bitアップリケーしょんにはX64というように、正しいバージョンをリンカーに設定してください。

このライブラリをディプロイメントパッケージ内に独自に作成しないでください。
シンラのサービスでは、常に最新バージョンのライブラリとゲームが自動的にディプロイされます。



## FAQ

1. シングルプレイヤーのゲームにShinra Game APIを使用する必要がありますか?

  いいえ、シンラゲームAPIはシンラテクノロジーの特定の機能にアクセスするときのみ必要です。 シングルプレイヤーゲームはDirectX11とWindowsAPIを使用することのみが必要です。
