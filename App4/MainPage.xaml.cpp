//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "HolographicView.h"

using namespace App4;

using namespace std;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::UI::ViewManagement;
using namespace Concurrency;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::System::Threading;
using namespace Windows::System;

using namespace concurrency;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

MainPage::MainPage()
{
	InitializeComponent();
}

void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	String^ arg = safe_cast<String^>(e->Parameter);
	if (arg == L"ProtocolSwitchExclusive")
	{
		// Since this is a protocol activation, we want to consolidate in order to ensure that
		// the 2D view is removed from the app starting.
		SwitchToExclusive(true /*shouldConsolidate*/);
	}
}


void MainPage::ShowView(int viewId, bool shouldConsolidate)
{
	if (viewId != 0)
	{
		safe_cast<App^>(App::Current)->MainDispatcher->RunAsync(
			CoreDispatcherPriority::Normal,
			ref new DispatchedHandler([this, viewId, shouldConsolidate]()
		{
			if (shouldConsolidate)
			{
				auto asyncAction = ApplicationViewSwitcher::SwitchAsync(
					viewId,
					ApplicationView::GetForCurrentView()->Id,
					ApplicationViewSwitchingOptions::ConsolidateViews);
			}
			else
			{
				auto asyncAction = ApplicationViewSwitcher::TryShowAsStandaloneAsync(viewId);
			}
		}));
	}
}

void MainPage::SwitchToExclusive(bool shouldConsolidate)
{
	if (safe_cast<App^>(App::Current)->NewViewInProgress)
	{
		return;
	}

	if (m_exclusiveView == nullptr)
	{
		safe_cast<App^>(App::Current)->NewViewInProgress = true;
		m_exclusiveView = CoreApplication::CreateNewView(ref new HolographicViewSource());

		m_exclusiveView->Dispatcher->RunAsync(
			CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]()
		{
			safe_cast<App^>(App::Current)->NewViewInProgress = false;
			m_exclusiveViewId = ApplicationView::GetForCurrentView()->Id;

			CoreWindow::GetForCurrentThread()->Activate();
			m_exclusiveViewCreated.set();
		}));
	}

	create_task(m_exclusiveViewCreated).then([this, shouldConsolidate]()
	{
		ShowView(m_exclusiveViewId, shouldConsolidate);
	}, task_continuation_context::use_arbitrary());
}

void MainPage::SwitchExclusiveView_Click(Platform::Object^, Windows::UI::Xaml::RoutedEventArgs^)
{
	// If we're handling a user press to show the exclusive view, ensure that we don't consolidate the 2D
	// view, since the user may wish to return to this view.
	SwitchToExclusive(false /*shouldConsolidate*/);
}

