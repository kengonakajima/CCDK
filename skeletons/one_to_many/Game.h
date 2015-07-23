//
// Game.h
//

#pragma once

#include "pch.h"
#include "StepTimer.h"
#include "SpriteFont.h"
#include "Audio.h"
#include "ShinraGame.h"

#include <vector>

using namespace DirectX;



class Player
{
public:
    Player( shinra::PlayerID playerID, std::shared_ptr<SpriteFont> font);
    shinra::PlayerID getPlayerID() { return playerID; }
    void Update();
    void Render(int frameCnt);
    void handleInput(const RAWINPUT& rawInput);
private:
    // Show status logs
    shinra::PlayerID playerID;
    std::unique_ptr<SpriteBatch> m_spriteBatch;
    std::shared_ptr<SpriteFont> m_spriteFont;
    std::unique_ptr<AudioEngine> m_audioEngine;
    std::unique_ptr<SoundEffect> m_soundEffect;
    WCHAR						m_lastKey;

};



// A basic game implementation that creates a D3D11 device and
// provides a game loop
class Game
{
public:

    Game();

    // Initialization and management
    void Initialize(HWND window);

    // Basic game loop
    void Tick();
    void Render();

    // Rendering helpers
    void Clear();
    void Present();

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged();


    // 1:N support
    void addPlayer(shinra::PlayerID playerID);
    void removePlayer(shinra::PlayerID playerID);


    // Properites
    void GetDefaultSize( size_t& width, size_t& height ) const;

    void handleInput(HRAWINPUT hRawInput);

private:

    void Update(DX::StepTimer const& timer);

    void CreateDevice();
    void CreateResources();
    
    void OnDeviceLost();

    // Application state
    HWND                                            m_window;

    // Direct3D Objects
    D3D_FEATURE_LEVEL                               m_featureLevel;
    Microsoft::WRL::ComPtr<ID3D11Device>            m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11Device1>           m_d3dDevice1;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>     m_d3dContext;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    m_d3dContext1;

    // Rendering resources
    Microsoft::WRL::ComPtr<IDXGISwapChain>          m_swapChain;
    Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_swapChain1;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>         m_depthStencil;
    std::shared_ptr<SpriteFont>                     m_spriteFont;

    // Game state
    DX::StepTimer                                   m_timer;
    int m_framecnt;

    // 1:N
    std::vector<Player*> m_players;

};
