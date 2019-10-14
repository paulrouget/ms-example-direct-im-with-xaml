#pragma once

#include "Common\DeviceResources.h"
#include "HolographicMain.h"

namespace App4
{
	// Main entry point for our holographic view. Connects the app with the HoloShell and handles application lifecycle events.
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class HolographicView sealed : public Windows::ApplicationModel::Core::IFrameworkView
	{
	public:
		HolographicView();

		// IFrameworkView Methods.
		virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
		virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
		virtual void Load(Platform::String^ entryPoint);
		virtual void Run();
		virtual void Uninitialize();

	protected:
		// Application lifecycle event handlers.
		void OnViewActivated(Windows::ApplicationModel::Core::CoreApplicationView^ sender, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
		void OnResuming(Platform::Object^ sender, Platform::Object^ args);

		// Window event handlers.
		void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
		void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);

	private:
		std::unique_ptr<HolographicMain> m_main;

		std::shared_ptr<DX::DeviceResources> m_deviceResources;
		bool m_windowClosed = false;
		bool m_windowVisible = true;

		// The holographic space the app will use for rendering.
		Windows::Graphics::Holographic::HolographicSpace^ m_holographicSpace = nullptr;
	};

	ref class HolographicViewSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
	{
	public:
		HolographicViewSource();
		virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
	};
}