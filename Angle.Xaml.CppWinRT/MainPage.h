//
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "OpenGLES.h"
#include <winrt/Windows.UI.Composition.h>
#include "SimpleRenderer.h"

namespace winrt::Angle_Xaml_CppWinRT::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();
		MainPage(std::shared_ptr<OpenGLES> openGLES);
	
	private:
		//void OnPageLoaded(Windows::Foundation::IInspectable const& sender, Windows::UI::Xaml::RoutedEventArgs const& e);
		void CreateRenderSurface();
		void DestroyRenderSurface();
		void RenderLoop(Windows::Foundation::IAsyncAction const& action);
		void RecreateRenderer();

		unsigned int mLastReleasedPointerId;
		std::shared_ptr<OpenGLES> mOpenGLES;

		Windows::Foundation::Size mCustomRenderSurfaceSize;
		bool mUseCustomRenderSurfaceSize;

		EGLSurface mRenderSurface; // This surface is associated with a swapChainPanel on the page
		bool mContentLoaded;
		float mLogicalDpi;
		float mAspectRatio;
		Windows::Graphics::Display::DisplayOrientations mCurrentOrientation;
		Windows::Graphics::Display::DisplayOrientations mLastOrientation;

		bool mIsWindowActive;
		Windows::UI::Core::CoreWindowActivationState mLatestActivateState;
		bool mUpdateRenderSurface;

		// NOTE: this is needed because swapChainPanel should always blend against black or
		// we will get rendering artifacts
		Windows::UI::Xaml::Controls::Border mSwapChainPanelBackground;
		Windows::UI::Xaml::Controls::Border mShadowHost;
		Windows::UI::Xaml::Controls::Grid mContentRoot;
		Windows::UI::Xaml::Controls::SwapChainPanel mSwapChainPanel;

		std::unique_ptr<Angle::SimpleRenderer> mCubeRenderer;
    };
}

namespace winrt::Angle_Xaml_CppWinRT::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
