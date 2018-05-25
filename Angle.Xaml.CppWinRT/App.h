#pragma once
#include <winrt/Windows.UI.Xaml.h>

namespace Angle 
{
    struct App : winrt::Windows::UI::Xaml::ApplicationT<App>
    {
        App();
        void OnLaunched(winrt::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs const&);
    };
}
