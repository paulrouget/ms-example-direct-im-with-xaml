#include <pch.h>
#include "DirectXApp.h"
#include "XamlApp.h"

using namespace App4;

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::Core;

// The main function is only used to initialize our IFrameworkView class.
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	//IActivatedEventArgs^ richArgs = AppInstance::GetActivatedEventArgs();
	//if (richArgs)
	//{
	//	auto direct3DApplicationSource = ref new Direct3DApplicationSource();
	//	CoreApplication::Run(direct3DApplicationSource);
	//}
	//else
	{
		// Launch straight into the XAML view
		Windows::UI::Xaml::Application::Start(ref new Windows::UI::Xaml::ApplicationInitializationCallback(
			[](Windows::UI::Xaml::ApplicationInitializationCallbackParams^ p) {
			(void)p; // Unused parameter
			auto app = ref new ::App4::XamlApp();
		}));
	}
	return 0;
}