#include "pch.h"
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
			mEglDisplay(EGL_NO_DISPLAY),
			mEglContext(EGL_NO_CONTEXT),
			mEglSurface(EGL_NO_SURFACE)
		{
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
					eglQuerySurface(mEglDisplay, mEglSurface, EGL_WIDTH, &panelWidth);
					eglQuerySurface(mEglDisplay, mEglSurface, EGL_HEIGHT, &panelHeight);

					mCubeRenderer->UpdateWindowSize(panelWidth, panelHeight);
					mCubeRenderer->Draw();

					if (eglSwapBuffers(mEglDisplay, mEglSurface) != GL_TRUE)
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
			const EGLint configAttributes[] =
			{
				EGL_RED_SIZE, 8,
				EGL_GREEN_SIZE, 8,
				EGL_BLUE_SIZE, 8,
				EGL_ALPHA_SIZE, 8,
				EGL_DEPTH_SIZE, 8,
				EGL_STENCIL_SIZE, 8,
				EGL_NONE
			};

			const EGLint contextAttributes[] =
			{
				EGL_CONTEXT_CLIENT_VERSION, 2,
				EGL_NONE
			};

			const EGLint surfaceAttributes[] =
			{
				EGL_ANGLE_SURFACE_RENDER_TO_BACK_BUFFER, EGL_TRUE,
				EGL_NONE
			};

			const EGLint defaultDisplayAttributes[] =
			{
				EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
				EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,
				EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
				EGL_NONE,
			};

			const EGLint fl9_3DisplayAttributes[] =
			{
				EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
				EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE, 9,
				EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE, 3,
				EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,
				EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
				EGL_NONE,
			};

			const EGLint warpDisplayAttributes[] =
			{
				EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
				EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE,
				EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,
				EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
				EGL_NONE,
			};

			EGLConfig config = NULL;

			PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
			if (!eglGetPlatformDisplayEXT)
			{
				throw Exception::CreateException(E_FAIL, L"Failed to get function eglGetPlatformDisplayEXT");
			}

			mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, defaultDisplayAttributes);
			if (mEglDisplay == EGL_NO_DISPLAY)
			{
				throw Exception::CreateException(E_FAIL, L"Failed to get EGL display");
			}

			if (eglInitialize(mEglDisplay, NULL, NULL) == EGL_FALSE)
			{
				mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, fl9_3DisplayAttributes);
				if (mEglDisplay == EGL_NO_DISPLAY)
				{
					throw Exception::CreateException(E_FAIL, L"Failed to get EGL display");
				}

				if (eglInitialize(mEglDisplay, NULL, NULL) == EGL_FALSE)
				{
					mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, warpDisplayAttributes);
					if (mEglDisplay == EGL_NO_DISPLAY)
					{
						throw Exception::CreateException(E_FAIL, L"Failed to get EGL display");
					}

					if (eglInitialize(mEglDisplay, NULL, NULL) == EGL_FALSE)
					{
						throw Exception::CreateException(E_FAIL, L"Failed to initialize EGL");
					}
				}
			}

			EGLint numConfigs = 0;
			if ((eglChooseConfig(mEglDisplay, configAttributes, &config, 1, &numConfigs) == EGL_FALSE) || (numConfigs == 0))
			{
				throw Exception::CreateException(E_FAIL, L"Failed to choose first EGLConfig");
			}

			PropertySet^ surfaceCreationProperties = ref new PropertySet();
			surfaceCreationProperties->Insert(ref new String(EGLNativeWindowTypeProperty), window);

			auto parameters = reinterpret_cast<IInspectable*>(surfaceCreationProperties);
			mEglSurface = eglCreateWindowSurface(mEglDisplay, config, parameters, surfaceAttributes);
			if (mEglSurface == EGL_NO_SURFACE)
			{
				throw Exception::CreateException(E_FAIL, L"Failed to create EGL fullscreen surface");
			}

			mEglContext = eglCreateContext(mEglDisplay, config, EGL_NO_CONTEXT, contextAttributes);
			if (mEglContext == EGL_NO_CONTEXT)
			{
				throw Exception::CreateException(E_FAIL, L"Failed to create EGL context");
			}

			if (eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext) == EGL_FALSE)
			{
				throw Exception::CreateException(E_FAIL, L"Failed to make fullscreen EGLSurface current");
			}
		}

		void CleanupEGL()
		{
			if (mEglDisplay != EGL_NO_DISPLAY && mEglSurface != EGL_NO_SURFACE)
			{
				eglDestroySurface(mEglDisplay, mEglSurface);
				mEglSurface = EGL_NO_SURFACE;
			}

			if (mEglDisplay != EGL_NO_DISPLAY && mEglContext != EGL_NO_CONTEXT)
			{
				eglDestroyContext(mEglDisplay, mEglContext);
				mEglContext = EGL_NO_CONTEXT;
			}

			if (mEglDisplay != EGL_NO_DISPLAY)
			{
				eglTerminate(mEglDisplay);
				mEglDisplay = EGL_NO_DISPLAY;
			}
		}

		bool mWindowClosed;
		bool mWindowVisible;

		EGLDisplay mEglDisplay;
		EGLContext mEglContext;
		EGLSurface mEglSurface;

		std::unique_ptr<SimpleRenderer> mCubeRenderer;
	};
}

[MTAThread]
int main(Array<String^>^)
{
	CoreApplication::Run(ref new Angle::App());
	return 0;
}
