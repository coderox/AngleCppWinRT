//
// Declaration of the MainPage class.
//

#pragma once

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Graphics.Display.h>
#include "OpenGLES.h"
#include "SimpleRenderer.h"

namespace Angle
{
    struct MainPage : winrt::Windows::UI::Xaml::Controls::PageT<MainPage>
    {
        MainPage();
		MainPage(std::shared_ptr<OpenGLES> openGLES);
	
	private:
		//void OnPageLoaded(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& e);
		void CreateRenderSurface();
		void DestroyRenderSurface();
		void RenderLoop(winrt::Windows::Foundation::IAsyncAction const& action);
		void RecreateRenderer();

		unsigned int mLastReleasedPointerId;
		std::shared_ptr<OpenGLES> mOpenGLES;

		winrt::Windows::Foundation::Size mCustomRenderSurfaceSize;
		bool mUseCustomRenderSurfaceSize;

		EGLSurface mRenderSurface; // This surface is associated with a swapChainPanel on the page
		bool mContentLoaded;
		float mLogicalDpi;
		float mAspectRatio;
		winrt::Windows::Graphics::Display::DisplayOrientations mCurrentOrientation;
		winrt::Windows::Graphics::Display::DisplayOrientations mLastOrientation;

		bool mIsWindowActive;
		winrt::Windows::UI::Core::CoreWindowActivationState mLatestActivateState;
		bool mUpdateRenderSurface;

		// NOTE: this is needed because swapChainPanel should always blend against black or
		// we will get rendering artifacts
		winrt::Windows::UI::Xaml::Controls::Border mSwapChainPanelBackground;
		winrt::Windows::UI::Xaml::Controls::Border mShadowHost;
		winrt::Windows::UI::Xaml::Controls::Grid mContentRoot;
		winrt::Windows::UI::Xaml::Controls::SwapChainPanel mSwapChainPanel;

		std::unique_ptr<Angle::SimpleRenderer> mCubeRenderer;
    };
}
