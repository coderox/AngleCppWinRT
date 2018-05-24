#include "pch.h"
#include "MainPage.h"

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Windows::Graphics::Display;

#define SCALE_FACTOR 1.0f

namespace winrt::Angle_Xaml_CppWinRT::implementation
{
	MainPage::MainPage()
		: MainPage(nullptr)
	{ }

	MainPage::MainPage(std::shared_ptr<OpenGLES> openGLES)
		: mOpenGLES(openGLES)
	{
		InitializeComponent();

		auto info =  DisplayInformation::GetForCurrentView();

		mLogicalDpi = info.LogicalDpi() / 96.0f * SCALE_FACTOR;

		mSwapChainPanelBackground.Child(mSwapChainPanel);
		mSwapChainPanelBackground.BorderThickness(Thickness{ 0 });
		mSwapChainPanelBackground.Background(SolidColorBrush(Colors::Black()));

		mContentRoot.BorderThickness(Thickness{ 0 });
		mContentRoot.Background(nullptr);
		mContentRoot.Children().Append(mSwapChainPanelBackground);

		Content(mContentRoot);

		Loaded([=](auto, auto) {
			// Run task on a dedicated high priority background thread
			WorkItemHandler handler(this, &MainPage::RenderLoop);
			ThreadPool::RunAsync(handler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
		});
	}

	//void MainPage::OnPageLoaded(Windows::Foundation::IInspectable const& sender, RoutedEventArgs const& e) 
	//{
	//	// Run task on a dedicated high priority background thread
	//	WorkItemHandler handler(this, &MainPage::RenderLoop);
	//	ThreadPool::RunAsync(handler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
	//}

	void MainPage::CreateRenderSurface()
	{
		if (mOpenGLES) {
			mRenderSurface = mOpenGLES->CreateSurface(mSwapChainPanel, nullptr, &mLogicalDpi);
			mUpdateRenderSurface = mRenderSurface != EGL_NO_SURFACE;
		}
	}

	void MainPage::DestroyRenderSurface()
	{
		if (mOpenGLES) {
			mOpenGLES->DestroySurface(mRenderSurface);
		}
		mRenderSurface = EGL_NO_SURFACE;
	}

	void MainPage::RenderLoop(IAsyncAction const& /*action*/)
	{
		mOpenGLES->Initialize();		
		CreateRenderSurface();
		mOpenGLES->MakeCurrent(mRenderSurface);

		RecreateRenderer();

		while (true) {
			EGLint panelWidth = 0;
			EGLint panelHeight = 0;
			mOpenGLES->GetSurfaceDimensions(mRenderSurface, &panelWidth, &panelHeight);

			mCubeRenderer->UpdateWindowSize(panelWidth, panelHeight);
			mCubeRenderer->Draw();

			if (mOpenGLES->SwapBuffers(mRenderSurface) != GL_TRUE)
			{
				mCubeRenderer.reset(nullptr);
				RecreateRenderer();
			}
		}

		DestroyRenderSurface();
	}

	void MainPage::RecreateRenderer()
	{
		if (!mCubeRenderer)
		{
			mCubeRenderer.reset(new Angle::SimpleRenderer());
		}
	}
}
