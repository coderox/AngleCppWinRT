#include "pch.h"
#include "OpenGLES.h"
#include "SimpleRenderer.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Platform;

namespace Angle {
	ref class App sealed : public IFrameworkView, IFrameworkViewSource
	{
	public:

		App() :
			mWindowClosed(false),
			mWindowVisible(true),
			mEglSurface(EGL_NO_SURFACE),
			mOpenGLES(nullptr)
		{
			mOpenGLES = std::make_unique<OpenGLES>();
		}

		// IFrameworkViewSource Methods.
		virtual IFrameworkView^ CreateView()
		{
			return this;
		}

		// IFrameworkView Methods.
		virtual void Initialize(CoreApplicationView^ applicationView) {}
		virtual void Load(Platform::String^ entryPoint) {}
		virtual void Uninitialize() {}

		virtual void SetWindow(CoreWindow^ window)
		{
			window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &App::OnWindowVisibilityChanged);

			window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &App::OnWindowClosed);

			InitializeEGL(window);

			RecreateRenderer();

			window->Activate();
		}

		virtual void Run()
		{
			while (!mWindowClosed)
			{
				if (mWindowVisible)
				{
					CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

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
					CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
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

		void OnWindowVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
		{
			mWindowVisible = args->Visible;
		}

		void OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
		{
			mWindowClosed = true;
		}

		void InitializeEGL(CoreWindow^ window)
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

		std::unique_ptr<SimpleRenderer> mCubeRenderer;
		std::unique_ptr<OpenGLES> mOpenGLES;
	};
}

[MTAThread]
int main(Array<String^>^)
{
	CoreApplication::Run(ref new Angle::App());
	return 0;
}
