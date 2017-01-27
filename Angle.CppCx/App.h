#pragma once

#include <string>

#include "pch.h"
#include "SimpleRenderer.h"

namespace Angle
{
    ref class App sealed : public Windows::ApplicationModel::Core::IFrameworkView
    {
    public:
        App();

        // IFrameworkView Methods.
        virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView){}
        virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
		virtual void Load(Platform::String^ entryPoint) {}
		virtual void Uninitialize() {}
		virtual void Run() {}

    private:
		void RunApp();
		void RecreateRenderer();

        // Application lifecycle event handlers.
        //void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);

        // Window event handlers.
        void OnWindowVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
        void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);

        void InitializeEGL(Windows::UI::Core::CoreWindow^ window);
        void CleanupEGL();

        bool mWindowClosed;
        bool mWindowVisible;
        
        EGLDisplay mEglDisplay;
        EGLContext mEglContext;
        EGLSurface mEglSurface;

        std::unique_ptr<SimpleRenderer> mCubeRenderer;
    };

}