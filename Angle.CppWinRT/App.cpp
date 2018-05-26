#include "pch.h"
#include "OpenGLES.h"
#include "SimpleRenderer.h"

using namespace winrt; 
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;


namespace Angle {
	struct App : implements<App, IFrameworkViewSource, IFrameworkView>
	{
	public:

		App() :
			mWindowClosed(false),
			mWindowVisible(true),
			mEglSurface(EGL_NO_SURFACE)
		{
			mOpenGLES = std::make_unique<OpenGLES>();
		}

		// IFrameworkViewSource Methods.
		IFrameworkView CreateView()
		{
			return *this;
		}

		// IFrameworkView Methods.
		void Initialize(CoreApplicationView const &) { }
		void Load(hstring) { }
		void Uninitialize() { }

		void SetWindow(CoreWindow const& window)
		{
			window.VisibilityChanged({ this, &App::OnWindowVisibilityChanged });

			window.Closed({ this, &App::OnWindowClosed });

			InitializeEGL(window);

			RecreateRenderer();

			window.Activate();
		}

		void Run()
		{
			auto dispatcher = CoreWindow::GetForCurrentThread().Dispatcher();

			while (!mWindowClosed)
			{
				if (mWindowVisible)
				{
					dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

					EGLint panelWidth = 0;
					EGLint panelHeight = 0;
					mOpenGLES->GetSurfaceDimensions(mEglSurface, &panelWidth, &panelHeight);

					mCubeRenderer->UpdateWindowSize(panelWidth, panelHeight);
					mCubeRenderer->Draw();

					if (mOpenGLES->SwapBuffers(mEglSurface) != GL_TRUE)
					{
						mCubeRenderer.reset(nullptr);
						CleanupEGL();

						InitializeEGL(CoreWindow::GetForCurrentThread());
						RecreateRenderer();
					}
				}
				else
				{
					dispatcher.ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
				}
			}

			CleanupEGL();
		}

	private:

		void RecreateRenderer()
		{
			if (!mCubeRenderer)
			{
				mCubeRenderer.reset(new SimpleRenderer());
			}
		}

		void OnWindowVisibilityChanged(const IInspectable&, const winrt::Windows::UI::Core::VisibilityChangedEventArgs& args)
		{
			mWindowVisible = args.Visible();
		}

		void OnWindowClosed(const IInspectable&, const IInspectable&)
		{
			mWindowClosed = true;
		}


		void InitializeEGL(CoreWindow const & window)
		{
			mOpenGLES->Initialize();
			mEglSurface = mOpenGLES->CreateSurface(window, nullptr, nullptr);
			mOpenGLES->MakeCurrent(mEglSurface);
		}

		void CleanupEGL()
		{
			mOpenGLES->DestroySurface(mEglSurface);
			mOpenGLES->Cleanup();
		}

		bool mWindowClosed;
		bool mWindowVisible;

		EGLSurface mEglSurface;

		std::unique_ptr<OpenGLES> mOpenGLES;
		std::unique_ptr<SimpleRenderer> mCubeRenderer;
	};
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	CoreApplication::Run(make<Angle::App>());
}