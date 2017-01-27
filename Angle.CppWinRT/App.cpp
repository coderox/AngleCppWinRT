#include "pch.h"
#include "SimpleRenderer.h"

namespace com = ::Windows::Storage::Streams;

using namespace Angle;

using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Pickers;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Controls;
using namespace winrt::Windows::UI::Xaml::Media;
using namespace winrt::Windows::UI::Xaml::Media::Imaging;

// Helper to convert a length in device-independent pixels (DIPs) to a length in physical pixels.
inline float ConvertDipsToPixels(float dips, float dpi)
{
	static const float dipsPerInch = 96.0f;
	return floor(dips * dpi / dipsPerInch + 0.5f); // Round to nearest integer.
}

struct App : ApplicationT<App>
{
public:

	void OnWindowCreated(const IInspectable&) {
		auto mCoreWindow = CoreWindow::GetForCurrentThread();
		
		mCoreWindow.VisibilityChanged({ this, &App::OnWindowVisibilityChanged });

		mCoreWindow.Closed({ this, &App::OnWindowClosed });

		InitializeEGL(CoreWindow::GetForCurrentThread());
		 
		RecreateRenderer();

		mCoreWindow.Activate();
		
		RunApp();
	}

	void OnWindowClosed(const IInspectable&, const IInspectable&) {
		mWindowClosed = true;
	}

	void OnWindowVisibilityChanged(const IInspectable&, const winrt::Windows::UI::Core::VisibilityChangedEventArgs& args) {
		mWindowVisible = args.Visible();		
	}

private:

	void RecreateRenderer() {
		if (!mCubeRenderer)
		{
			mCubeRenderer.reset(new SimpleRenderer());
		}
	}

	void RunApp()
	{
		//CoreWindow::GetForCurrentThread().Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [this]() {

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
		//});
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
			// EGL_ANGLE_SURFACE_RENDER_TO_BACK_BUFFER is part of the same optimization as EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER (see above).
			// If you have compilation issues with it then please update your Visual Studio templates.
			EGL_ANGLE_SURFACE_RENDER_TO_BACK_BUFFER, EGL_TRUE,
			EGL_NONE
		};

		const EGLint defaultDisplayAttributes[] =
		{
			// These are the default display attributes, used to request ANGLE's D3D11 renderer.
			// eglInitialize will only succeed with these attributes if the hardware supports D3D11 Feature Level 10_0+.
			EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,

			// EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER is an optimization that can have large performance benefits on mobile devices.
			// Its syntax is subject to change, though. Please update your Visual Studio templates if you experience compilation issues with it.
			EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,

			// EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE is an option that enables ANGLE to automatically call 
			// the IDXGIDevice3::Trim method on behalf of the application when it gets suspended. 
			// Calling IDXGIDevice3::Trim when an application is suspended is a Windows Store application certification requirement.
			EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
			EGL_NONE,
		};

		const EGLint fl9_3DisplayAttributes[] =
		{
			// These can be used to request ANGLE's D3D11 renderer, with D3D11 Feature Level 9_3.
			// These attributes are used if the call to eglInitialize fails with the default display attributes.
			EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
			EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE, 9,
			EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE, 3,
			EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,
			EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
			EGL_NONE,
		};

		const EGLint warpDisplayAttributes[] =
		{
			// These attributes can be used to request D3D11 WARP.
			// They are used if eglInitialize fails with both the default display attributes and the 9_3 display attributes.
			EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
			EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE,
			EGL_ANGLE_DISPLAY_ALLOW_RENDER_TO_BACK_BUFFER, EGL_TRUE,
			EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
			EGL_NONE,
		};

		EGLConfig config = NULL;

		// eglGetPlatformDisplayEXT is an alternative to eglGetDisplay. It allows us to pass in display attributes, used to configure D3D11.
		PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
		if (!eglGetPlatformDisplayEXT)
		{
			throw winrt::hresult_error(E_FAIL, L"Failed to get function eglGetPlatformDisplayEXT");
		}

		//
		// To initialize the display, we make three sets of calls to eglGetPlatformDisplayEXT and eglInitialize, with varying 
		// parameters passed to eglGetPlatformDisplayEXT:
		// 1) The first calls uses "defaultDisplayAttributes" as a parameter. This corresponds to D3D11 Feature Level 10_0+.
		// 2) If eglInitialize fails for step 1 (e.g. because 10_0+ isn't supported by the default GPU), then we try again 
		//    using "fl9_3DisplayAttributes". This corresponds to D3D11 Feature Level 9_3.
		// 3) If eglInitialize fails for step 2 (e.g. because 9_3+ isn't supported by the default GPU), then we try again 
		//    using "warpDisplayAttributes".  This corresponds to D3D11 Feature Level 11_0 on WARP, a D3D11 software rasterizer.
		//

		// This tries to initialize EGL to D3D11 Feature Level 10_0+. See above comment for details.
		mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, defaultDisplayAttributes);
		if (mEglDisplay == EGL_NO_DISPLAY)
		{
			throw winrt::hresult_error(E_FAIL, L"Failed to get EGL display");
		}

		if (eglInitialize(mEglDisplay, NULL, NULL) == EGL_FALSE)
		{
			// This tries to initialize EGL to D3D11 Feature Level 9_3, if 10_0+ is unavailable (e.g. on some mobile devices).
			mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, fl9_3DisplayAttributes);
			if (mEglDisplay == EGL_NO_DISPLAY)
			{
				throw winrt::hresult_error(E_FAIL, L"Failed to get EGL display");
			}

			if (eglInitialize(mEglDisplay, NULL, NULL) == EGL_FALSE)
			{
				// This initializes EGL to D3D11 Feature Level 11_0 on WARP, if 9_3+ is unavailable on the default GPU.
				mEglDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, warpDisplayAttributes);
				if (mEglDisplay == EGL_NO_DISPLAY)
				{
					throw winrt::hresult_error(E_FAIL, L"Failed to get EGL display");
				}

				if (eglInitialize(mEglDisplay, NULL, NULL) == EGL_FALSE)
				{
					// If all of the calls to eglInitialize returned EGL_FALSE then an error has occurred.
					throw winrt::hresult_error(E_FAIL, L"Failed to initialize EGL");
				}
			}
		}

		EGLint numConfigs = 0;
		if ((eglChooseConfig(mEglDisplay, configAttributes, &config, 1, &numConfigs) == EGL_FALSE) || (numConfigs == 0))
		{
			throw winrt::hresult_error(E_FAIL, L"Failed to choose first EGLConfig");
		}

		// Create a PropertySet and initialize with the EGLNativeWindowType.
		PropertySet surfaceCreationProperties;
		surfaceCreationProperties.Insert(winrt::hstring(EGLNativeWindowTypeProperty), window);

		// You can configure the surface to render at a lower resolution and be scaled up to
		// the full window size. This scaling is often free on mobile hardware.
		//
		// One way to configure the SwapChainPanel is to specify precisely which resolution it should render at.
		// Size customRenderSurfaceSize = Size(800, 600);
		// surfaceCreationProperties->Insert(ref new String(EGLRenderSurfaceSizeProperty), PropertyValue::CreateSize(customRenderSurfaceSize));
		//
		// Another way is to tell the SwapChainPanel to render at a certain scale factor compared to its size.
		// e.g. if the SwapChainPanel is 1920x1280 then setting a factor of 0.5f will make the app render at 960x640
		// float customResolutionScale = 0.5f;
		// surfaceCreationProperties->Insert(ref new String(EGLRenderResolutionScaleProperty), PropertyValue::CreateSingle(customResolutionScale));
		
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

	void CleanupEGL() {
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

	//CoreWindow mCoreWindow{ nullptr };
	//CoreDispatcher mDispatcher{ nullptr };

	int m_width{ 0 };
	int m_height{ 0 };
	int m_xCenter{ 0 };
	int m_yCenter{ 0 };

	bool mWindowClosed;
	bool mWindowVisible;

	EGLDisplay mEglDisplay;
	EGLContext mEglContext;
	EGLSurface mEglSurface;

	std::unique_ptr<SimpleRenderer> mCubeRenderer;
};

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	Application::Start([](auto &&)
	{
		winrt::make<App>();
	});
}