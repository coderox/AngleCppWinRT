#pragma once

// EGL includes
#include <EGL/egl.h>
#include <EGL/eglplatform.h>

class OpenGLES {
public:
	OpenGLES();
	virtual ~OpenGLES();

	void Initialize();
#if __CPPWINRT
	EGLSurface CreateSurface(winrt::Windows::UI::Xaml::Controls::SwapChainPanel const& panel, const winrt::Windows::Foundation::Size* renderSurfaceSize, const float* renderResolutionScale);
#else
	EGLSurface CreateSurface(Windows::UI::Core::CoreWindow ^ panel, const Windows::Foundation::Size* renderSurfaceSize, const float* renderResolutionScale);
#endif
	void GetSurfaceDimensions(const EGLSurface surface, EGLint* width, EGLint* height);
	void DestroySurface(const EGLSurface surface);
	void MakeCurrent(const EGLSurface surface);
	EGLBoolean SwapBuffers(const EGLSurface surface);
	void Reset();
	void Cleanup();

private:
	EGLDisplay mEglDisplay;
	EGLContext mEglContext;
	EGLConfig mEglConfig;
};