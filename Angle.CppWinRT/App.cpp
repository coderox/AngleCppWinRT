#include "pch.h"
#include "SimpleRenderer.h"

using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt;

WINRT_EXPORT namespace Angle {
	struct App : implements<App, IFrameworkView, IFrameworkViewSource>
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
		IFrameworkView CreateView()
		{
			return *this;
		}

		// IFrameworkView Methods.
		void Initialize(CoreApplicationView const &) { }
		void Load(hstring_ref) { }
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
			while (!mWindowClosed)
			{
				if (mWindowVisible)
				{
					CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

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
					CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
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


		void InitializeEGL(CoreWindow const & window) {
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
				throw winrt::hresult_error(E_FAIL, L"Failed to get function eglGetPlatformDisplayEXT");
			}

			mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, defaultDisplayAttributes);
			if (mEglDisplay == EGL_NO_DISPLAY)
			{
				throw winrt::hresult_error(E_FAIL, L"Failed to get EGL display");
			}

			if (eglInitialize(mEglDisplay, NULL, NULL) == EGL_FALSE)
			{
				mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, fl9_3DisplayAttributes);
				if (mEglDisplay == EGL_NO_DISPLAY)
				{
					throw winrt::hresult_error(E_FAIL, L"Failed to get EGL display");
				}

				if (eglInitialize(mEglDisplay, NULL, NULL) == EGL_FALSE)
				{
					mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, warpDisplayAttributes);
					if (mEglDisplay == EGL_NO_DISPLAY)
					{
						throw winrt::hresult_error(E_FAIL, L"Failed to get EGL display");
					}

					if (eglInitialize(mEglDisplay, NULL, NULL) == EGL_FALSE)
					{
						throw winrt::hresult_error(E_FAIL, L"Failed to initialize EGL");
					}
				}
			}

			EGLint numConfigs = 0;
			if ((eglChooseConfig(mEglDisplay, configAttributes, &config, 1, &numConfigs) == EGL_FALSE) || (numConfigs == 0))
			{
				throw winrt::hresult_error(E_FAIL, L"Failed to choose first EGLConfig");
			}

			PropertySet surfaceCreationProperties;
			surfaceCreationProperties.Insert(winrt::hstring(EGLNativeWindowTypeProperty), window);

			auto parameters = reinterpret_cast<::IInspectable*>(winrt::get(surfaceCreationProperties));
			mEglSurface = eglCreateWindowSurface(mEglDisplay, config, parameters, surfaceAttributes);
			if (mEglSurface == EGL_NO_SURFACE)
			{
				throw winrt::hresult_error(E_FAIL, L"Failed to create EGL fullscreen surface");
			}

			mEglContext = eglCreateContext(mEglDisplay, config, EGL_NO_CONTEXT, contextAttributes);
			if (mEglContext == EGL_NO_CONTEXT)
			{
				throw winrt::hresult_error(E_FAIL, L"Failed to create EGL context");
			}

			if (eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext) == EGL_FALSE)
			{
				throw winrt::hresult_error(E_FAIL, L"Failed to make fullscreen EGLSurface current");
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

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	CoreApplication::Run(Angle::App());
}