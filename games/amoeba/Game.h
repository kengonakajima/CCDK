//
// Game.h
//

#pragma once

#include "pch.h"
#include "StepTimer.h"
#include "SpriteFont.h"
#include "Audio.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using Microsoft::WRL::ComPtr;


class AnimatedTexture
{
public:
    AnimatedTexture() :
        mPaused(false),
        mFrame(0),
        mFrameCount(0),
        mTextureWidth(0),
        mTextureHeight(0),
        mTimePerFrame(0.f),
        mTotalElapsed(0.f),
        mRotation(0.f),
        mScale(1.f,1.f),
        mDepth(0.f),
        mOrigin(0.f,0.f)
    {
    }

    AnimatedTexture( const DirectX::XMFLOAT2& origin,
                     float rotation,
                     float scale,
                     float depth ) :
        mPaused(false),
        mFrame(0),
        mFrameCount(0),
        mTextureWidth(0),
        mTextureHeight(0),
        mTimePerFrame(0.f),
        mTotalElapsed(0.f),
        mRotation( rotation ),
        mScale( scale, scale ),
        mDepth( depth ),
        mOrigin( origin )
    {
    }

    void Load( ID3D11ShaderResourceView* texture, int frameCount, int framesPerSecond )
    {
        if ( frameCount < 0 || framesPerSecond <= 0 )
            throw std::invalid_argument( "AnimatedTexture" );

        mPaused = false;
        mFrame = 0;
        mFrameCount = frameCount;
        mTimePerFrame = 1.f / float(framesPerSecond);
        mTotalElapsed = 0.f;
        mTexture = texture;

        if ( texture )
        {
            Microsoft::WRL::ComPtr<ID3D11Resource> resource;
            texture->GetResource( resource.GetAddressOf() );

            D3D11_RESOURCE_DIMENSION dim;
            resource->GetType( &dim );

            if ( dim != D3D11_RESOURCE_DIMENSION_TEXTURE2D )
                throw std::exception( "AnimatedTexture expects a Texture2D" );

            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2D;
            resource.As( &tex2D );

            D3D11_TEXTURE2D_DESC desc;
            tex2D->GetDesc( &desc );

            mTextureWidth = int( desc.Width );
            mTextureHeight = int( desc.Height );
        }
    }

    void Update( float elapsed )
    {
        if ( mPaused )
            return;

        mTotalElapsed += elapsed;

        if ( mTotalElapsed > mTimePerFrame )
        {
            ++mFrame;
            mFrame = mFrame % mFrameCount;
            mTotalElapsed -= mTimePerFrame;
        }
    }

    void Draw( DirectX::SpriteBatch* batch, const DirectX::XMFLOAT2& screenPos ) const
    {
        Draw( batch, mFrame, screenPos );
    }

    void Draw( DirectX::SpriteBatch* batch, int frame, const DirectX::XMFLOAT2& screenPos ) const
    {
        int frameWidth = mTextureWidth / mFrameCount;

        RECT sourceRect;
        sourceRect.left = frameWidth * frame;
        sourceRect.top = 0;
        sourceRect.right = sourceRect.left + frameWidth;
        sourceRect.bottom = mTextureHeight;

        batch->Draw( mTexture.Get(), screenPos, &sourceRect, DirectX::Colors::White,
                     mRotation, mOrigin, mScale, DirectX::SpriteEffects_None, mDepth );
    }

    void Reset()
    {
        mFrame = 0;
        mTotalElapsed = 0.f;
    }

    void Stop()
    {
        mPaused = true;
        mFrame = 0;
        mTotalElapsed = 0.f;
    }

    void Play() { mPaused = false; }
    void Paused() { mPaused = true; }

    bool IsPaused() const { return mPaused; }

private:
    bool                                                mPaused;
    int                                                 mFrame;
    int                                                 mFrameCount;
    int                                                 mTextureWidth;
    int                                                 mTextureHeight;
    float                                               mTimePerFrame;
    float                                               mTotalElapsed;
    float                                               mDepth;
    float                                               mRotation;
    DirectX::XMFLOAT2                                   mOrigin;
    DirectX::XMFLOAT2                                   mScale;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    mTexture;
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
	void OnKeydown(int keycode);

    // Properites
    void GetDefaultSize( size_t& width, size_t& height ) const;

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

    // Game state
    DX::StepTimer                                   m_timer;
	int m_framecnt;

	// Show status logs
	SpriteBatch* m_spriteBatch;
	SpriteFont* m_spriteFont;

    // game sprites
    AnimatedTexture *m_cellAnimTex;


	AudioEngine *m_audioEngine;
	SoundEffect *m_soundEffect;
};
