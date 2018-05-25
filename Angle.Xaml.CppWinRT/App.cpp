//
// App.cpp
// Implementation of the App class.
//

#include "pch.h"

#include "App.h"
#include "OpenGLES.h"
#include "MainPage.h"

using namespace winrt;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;
using namespace Angle;

App::App()
{    
#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
    UnhandledException([this](IInspectable const&, UnhandledExceptionEventArgs const& e)
    {
        if (IsDebuggerPresent())
        {
            auto errorMessage = e.Message();
            __debugbreak();
        }
    });
#endif
}

void App::OnLaunched([[maybe_unused]] LaunchActivatedEventArgs const& e)
{
	auto content = Window::Current().Content();
	if (content == nullptr) {
		Window::Current().Content(make<MainPage>(std::make_shared<OpenGLES>())); 
	}

	Window::Current().Activate();
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	Application::Start([](auto &&) { make<Angle::App>(); });
}
