//
// BlankPage.xaml.h
// Declaration of the BlankPage class
//

#pragma once

#include "MainPage.g.h"

namespace App4
{
	/// <summary>
	/// This page provides buttons to switch to the supported view types.
	/// The _Click event handlers perform the tasks of creating the new view and then
	/// switching to it.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class MainPage sealed
	{
	public:
		MainPage();

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

	private:
		Platform::Agile<Windows::ApplicationModel::Core::CoreApplicationView^> m_exclusiveView;
		int m_exclusiveViewId = 0;
		Concurrency::task_completion_event<void> m_exclusiveViewCreated;

		void SwitchToExclusive(bool shouldConsolidate);
		void ShowView(int viewId, bool shouldConsolidate);

		void SwitchExclusiveView_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}
