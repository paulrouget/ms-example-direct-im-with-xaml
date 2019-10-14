//
// App.xaml.cpp
// Implementation of the App class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "HolographicView.h"

using namespace App4;

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Core;
using namespace Windows::UI::ViewManagement;

using namespace concurrency;

// The Blank Application template is documented at http://go.microsoft.com/fwlink/?LinkId=402347&clcid=0x409

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
 : m_newViewInProgress(false)
{
    InitializeComponent();
    Suspending += ref new SuspendingEventHandler(this, &App::OnSuspending);
}

void App::OnActivated(Windows::ApplicationModel::Activation::IActivatedEventArgs^ e)
{
	// Handle traditional launch (ie from Start/Pins). This means we want to hit the
	// 2D XAML view
    if (e->Kind == ActivationKind::Launch)
    {
        auto launchArgs = dynamic_cast<LaunchActivatedEventArgs^>(e);
        if (auto switcher = launchArgs->ViewSwitcher)
        {
            // We have an ActivationViewSwitcher, meaning that this isn't our first time
            // being launched. Switch to our existing main view via the switcher
            switcher->ShowAsStandaloneAsync(m_mainViewId);
        }
        else
        {
            // This is our first launch, so create our first view
            CreateFirstView(launchArgs->Arguments);
        }
    }
	// Handle protocol activation here. In this case, we want to go straight to a 3D
	// view. However, if this is also the first activation, we need to create the 2D
	// view first, then immediately switch to the 3D view (via activation arg)
    else if (e->Kind == ActivationKind::Protocol)
    {
        auto protocolArgs = dynamic_cast<ProtocolActivatedEventArgs^>(e);
        if (auto switcher = protocolArgs->ViewSwitcher)
        {
            // We have an ActivationViewSwitcher, which indicates that we're already running. In that case, we can
            // create a new exclusive view, and target the protocol launch directly.
            auto exclusiveView = CoreApplication::CreateNewView(ref new HolographicViewSource());

            exclusiveView->Dispatcher->RunAsync(
                CoreDispatcherPriority::Normal, ref new DispatchedHandler([switcher]()
            {
                CoreWindow::GetForCurrentThread()->Activate();
                switcher->ShowAsStandaloneAsync(ApplicationView::GetForCurrentView()->Id);
            }));
        }
        else
        {
            // This is the initial launch, although it's via protocol. We do essentially the same thing as the the
            // Launch activation, except we add a specific argument that cues the MainPage to 'fast-switch' to an
            // exclusive view
            CreateFirstView(L"ProtocolSwitchExclusive");
        }
    }
}

/// <summary>
/// Invoked when the application is launched normally by the end user.  Other entry points
/// will be used such as when the application is launched to open a specific file.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
void App::OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e)
{
    return OnActivated(e);
}

/// <summary>
/// Invoked when application execution is being suspended.  Application state is saved
/// without knowing whether the application will be terminated or resumed with the contents
/// of memory still intact.
/// </summary>
/// <param name="sender">The source of the suspend request.</param>
/// <param name="e">Details about the suspend request.</param>
void App::OnSuspending(Object^ sender, SuspendingEventArgs^ e)
{
    (void) sender;    // Unused parameter
    (void) e;    // Unused parameter

    //TODO: Save application state and stop any background activity
}

/// <summary>
/// Invoked when Navigation to a certain page fails
/// </summary>
/// <param name="sender">The Frame which failed navigation</param>
/// <param name="e">Details about the navigation failure</param>
void App::OnNavigationFailed(Platform::Object ^sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs ^e)
{
    throw ref new FailureException("Failed to load Page " + e->SourcePageType.Name);
}

Windows::UI::Core::CoreDispatcher^ App::MainDispatcher::get()
{
    return m_mainDispatcher;
}

void App::OnWindowCreated(WindowCreatedEventArgs^ args)
{
	__super::OnWindowCreated(args);

	m_newViewInProgress = false;
}

void App::CreateFirstView(Platform::String^ arguments)
{
	// First off, disable the default system view activation policy. That enables us to select
	// the targeted view we want to activate in subsequent activations.
	ApplicationViewSwitcher::DisableSystemViewActivationPolicy();

    auto rootFrame = dynamic_cast<Frame^>(Window::Current->Content);

    // Do not repeat app initialization when the Window already has content,
    // just ensure that the window is active
    if (rootFrame == nullptr)
    {
        // Create a Frame to act as the navigation context and associate it with
        // a SuspensionManager key
        rootFrame = ref new Frame();

        // Set the default language
        rootFrame->Language = Windows::Globalization::ApplicationLanguages::Languages->GetAt(0);

        rootFrame->NavigationFailed += ref new Windows::UI::Xaml::Navigation::NavigationFailedEventHandler(this, &App::OnNavigationFailed);

        m_mainDispatcher = Window::Current->Dispatcher;
        m_mainViewId = ApplicationView::GetForCurrentView()->Id;

        if (rootFrame->Content == nullptr)
        {
            // When the navigation stack isn't restored navigate to the first page,
            // configuring the new page by passing required information as a navigation
            // parameter
            rootFrame->Navigate(TypeName(MainPage::typeid), arguments);
        }

        // Place the frame in the current Window
        Window::Current->Content = rootFrame;
        // Ensure the current window is active
        Window::Current->Activate();
    }
    else
    {
        if (rootFrame->Content == nullptr)
        {
            // When the navigation stack isn't restored navigate to the first page,
            // configuring the new page by passing required information as a navigation
            // parameter
            rootFrame->Navigate(TypeName(MainPage::typeid), arguments);
        }

        // Ensure the current window is active
        Window::Current->Activate();
    }
}
