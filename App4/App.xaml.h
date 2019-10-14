//
// App.xaml.h
// Declaration of the App class.
//

#pragma once

#include "App.g.h"

namespace App4
{
    /// <summary>
    /// Provides application-specific behavior to supplement the default Application class.
    /// </summary>
    ref class App sealed
    {
    protected:
        virtual void OnActivated(Windows::ApplicationModel::Activation::IActivatedEventArgs^ e) override;
        virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e) override;
		virtual void OnWindowCreated(Windows::UI::Xaml::WindowCreatedEventArgs^ args) override;

    internal:
        App();

        property Windows::UI::Core::CoreDispatcher^ MainDispatcher
        {
            Windows::UI::Core::CoreDispatcher^ get();
        };

		property bool NewViewInProgress
		{
			bool get() { return m_newViewInProgress; }
			void set(bool value) { m_newViewInProgress = value; }
		}

    private:
        void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
        void OnNavigationFailed(Platform::Object ^sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs ^e);

        void CreateFirstView(Platform::String^ arguments);
        void Switch2Dto3D();

        Windows::UI::Core::CoreDispatcher^ m_mainDispatcher;
        int m_mainViewId;
		std::atomic<bool> m_newViewInProgress;
    };
}