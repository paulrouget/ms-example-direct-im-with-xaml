#include "pch.h"
#include "HolographicMain.h"
#include "Common\DirectXHelper.h"

#include <windows.graphics.directx.direct3d11.interop.h>
#include "App.xaml.h"

using namespace App4;

using namespace concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI::ViewManagement;
using namespace Windows::Graphics::Holographic;
using namespace Windows::Perception::Spatial;
using namespace Windows::UI::Input::Spatial;
using namespace std::placeholders;

// Loads and initializes application assets when the application is loaded.
HolographicMain::HolographicMain(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources),
	Visible(false),
	m_mouseChange(0),
	m_scrollChange(0),
	m_mouseDown(false)
{
	// Register to be notified if the Device is lost or recreated
	m_deviceResources->RegisterDeviceNotify(this);
}

void HolographicMain::SetHolographicSpace(HolographicSpace^ holographicSpace)
{
	UnregisterHolographicEventHandlers();

	m_holographicSpace = holographicSpace;

	// Initialize the sample hologram.
	m_sceneRenderer = std::unique_ptr<Sample3DSceneRenderer>(new Sample3DSceneRenderer(m_deviceResources));

	// Use the default SpatialLocator to track the motion of the device.
	m_locator = SpatialLocator::GetDefault();

	// Be able to respond to changes in the positional tracking state.
	m_locatabilityChangedToken =
		m_locator->LocatabilityChanged +=
		ref new Windows::Foundation::TypedEventHandler<SpatialLocator^, Object^>(
			std::bind(&HolographicMain::OnLocatabilityChanged, this, _1, _2)
			);

	// Respond to camera added events by creating any resources that are specific
	// to that camera, such as the back buffer render target view.
	// When we add an event handler for CameraAdded, the API layer will avoid putting
	// the new camera in new HolographicFrames until we complete the deferral we created
	// for that handler, or return from the handler without creating a deferral. This
	// allows the app to take more than one frame to finish creating resources and
	// loading assets for the new holographic camera.
	// This function should be registered before the app creates any HolographicFrames.
	m_cameraAddedToken =
		m_holographicSpace->CameraAdded +=
		ref new Windows::Foundation::TypedEventHandler<HolographicSpace^, HolographicSpaceCameraAddedEventArgs^>(
			std::bind(&HolographicMain::OnCameraAdded, this, _1, _2)
			);

	// Respond to camera removed events by releasing resources that were created for that
	// camera.
	// When the app receives a CameraRemoved event, it releases all references to the back
	// buffer right away. This includes render target views, Direct2D target bitmaps, and so on.
	// The app must also ensure that the back buffer is not attached as a render target, as
	// shown in DeviceResources::ReleaseResourcesForBackBuffer.
	m_cameraRemovedToken =
		m_holographicSpace->CameraRemoved +=
		ref new Windows::Foundation::TypedEventHandler<HolographicSpace^, HolographicSpaceCameraRemovedEventArgs^>(
			std::bind(&HolographicMain::OnCameraRemoved, this, _1, _2)
			);

	// The simplest way to render world-locked holograms is to create a stationary reference frame
	// when the app is launched. This is roughly analogous to creating a "world" coordinate system
	// with the origin placed at the device's position as the app is launched.
	m_referenceFrame = m_locator->CreateStationaryFrameOfReferenceAtCurrentLocation();

	// Notes on spatial tracking APIs:
	// * Stationary reference frames are designed to provide a best-fit position relative to the
	//   overall space. Individual positions within that reference frame are allowed to drift slightly
	//   as the device learns more about the environment.
	// * When precise placement of individual holograms is required, a SpatialAnchor should be used to
	//   anchor the individual hologram to a position in the real world - for example, a point the user
	//   indicates to be of special interest. Anchor positions do not drift, but can be corrected; the
	//   anchor will use the corrected position starting in the next frame after the correction has
	//   occurred.

	// To be able to detect when the user "selects" we will need a GestureRecognizer
	m_spInteractionMgr = SpatialInteractionManager::GetForCurrentView();
	// We need to indicate that we are interested in the select gesture
	m_gestureRecognizer = ref new SpatialGestureRecognizer(SpatialGestureSettings::Tap);

	// Bind a handler with the Person's finger and voice gestures to feed to our gesture recognizer.
	m_spInteractionMgr->InteractionDetected += ref new TypedEventHandler<SpatialInteractionManager^, SpatialInteractionDetectedEventArgs^>(
		std::bind(&HolographicMain::OnInteractionDetected, this, _1, _2)
		);
}

void HolographicMain::UnregisterHolographicEventHandlers()
{
	if (m_holographicSpace != nullptr)
	{
		// Clear previous event registrations.

		if (m_cameraAddedToken.Value != 0)
		{
			m_holographicSpace->CameraAdded -= m_cameraAddedToken;
			m_cameraAddedToken.Value = 0;
		}

		if (m_cameraRemovedToken.Value != 0)
		{
			m_holographicSpace->CameraRemoved -= m_cameraRemovedToken;
			m_cameraRemovedToken.Value = 0;
		}
	}

	if (m_locator != nullptr)
	{
		m_locator->LocatabilityChanged -= m_locatabilityChangedToken;
	}
}

HolographicMain::~HolographicMain()
{
	// Deregister device notification.
	m_deviceResources->RegisterDeviceNotify(nullptr);

	UnregisterHolographicEventHandlers();
}

// Updates the application state once per frame.
HolographicFrame^ HolographicMain::Update()
{
	// Before doing the timer update, there is some work to do per-frame
	// to maintain holographic rendering. First, we will get information
	// about the current frame.

	// The HolographicFrame has information that the app needs in order
	// to update and render the current frame. The app begins each new
	// frame by calling CreateNextFrame.
	HolographicFrame^ holographicFrame = m_holographicSpace->CreateNextFrame();

	// Get a prediction of where holographic cameras will be when this frame
	// is presented.
	HolographicFramePrediction^ prediction = holographicFrame->CurrentPrediction;

	// Back buffers can change from frame to frame. Validate each buffer, and recreate
	// resource views and depth buffers as needed.
	m_deviceResources->EnsureCameraResources(holographicFrame, prediction);

	// Next, we get a coordinate system from the attached frame of reference that is
	// associated with the current frame. Later, this coordinate system is used for
	// for creating the stereo view matrices when rendering the sample content.
	SpatialCoordinateSystem^ currentCoordinateSystem = m_referenceFrame->CoordinateSystem;

	// Update scene objects.
	m_timer.Tick([&]()
	{
		m_sceneRenderer->Update(
			m_timer,
			m_mouseChange,
			m_scrollChange,
			m_mouseDown);
	});

	// We complete the frame update by using information about our content positioning
	// to set the focus point.

	for (auto cameraPose : prediction->CameraPoses)
	{
		// The HolographicCameraRenderingParameters class provides access to set
		// the image stabilization parameters.
		HolographicCameraRenderingParameters^ renderingParameters = holographicFrame->GetRenderingParameters(cameraPose);

		// SetFocusPoint informs the system about a specific point in your scene to
		// prioritize for image stabilization. The focus point is set independently
		// for each holographic camera.
		// You should set the focus point near the content that the user is looking at.
		// In this example, we put the focus point at the center of the sample hologram,
		// since that is the only hologram available for the user to focus on.
		// You can also set the relative velocity and facing of that content; the sample
		// hologram is at a fixed point so we only need to indicate its position.
		renderingParameters->SetFocusPoint(
			currentCoordinateSystem,
			m_sceneRenderer->GetPosition()
		);
	}

	// The holographic frame will be used to get up-to-date view and projection matrices and
	// to present the swap chain.
	return holographicFrame;
}

// Renders the current frame to each holographic camera, according to the
// current application and spatial positioning state. Returns true if the
// frame was rendered to at least one camera.
bool HolographicMain::Render(Windows::Graphics::Holographic::HolographicFrame^ holographicFrame)
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

	// Lock the set of holographic camera resources, then draw to each camera
	// in this frame.
	return m_deviceResources->UseHolographicCameraResources<bool>(
		[this, holographicFrame](std::map<UINT32, std::unique_ptr<DX::CameraResources>>& cameraResourceMap)
	{
		// Up-to-date frame predictions enhance the effectiveness of image stablization and
		// allow more accurate positioning of holograms.
		holographicFrame->UpdateCurrentPrediction();
		HolographicFramePrediction^ prediction = holographicFrame->CurrentPrediction;

		bool atLeastOneCameraRendered = false;
		for (auto cameraPose : prediction->CameraPoses)
		{
			// This represents the device-based resources for a HolographicCamera.
			DX::CameraResources* pCameraResources = cameraResourceMap[cameraPose->HolographicCamera->Id].get();

			// Get the device context.
			const auto context = m_deviceResources->GetD3DDeviceContext();
			const auto depthStencilView = pCameraResources->GetDepthStencilView();

			// Set render targets to the current holographic camera.
			ID3D11RenderTargetView *const targets[1] = { pCameraResources->GetBackBufferRenderTargetView() };
			context->OMSetRenderTargets(1, targets, depthStencilView);

			// Clear the back buffer and depth stencil view.
			context->ClearRenderTargetView(targets[0], DirectX::Colors::Transparent);
			context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

			// The view and projection matrices for each holographic camera will change
			// every frame. This function refreshes the data in the constant buffer for
			// the holographic camera indicated by cameraPose.
			pCameraResources->UpdateViewProjectionBuffer(m_deviceResources, cameraPose, m_referenceFrame->CoordinateSystem);

			// Attach the view/projection constant buffer for this camera to the graphics pipeline.
			bool cameraActive = pCameraResources->AttachViewProjectionBuffer(m_deviceResources);

			// Only render world-locked content when positional tracking is active.
			if (cameraActive)
			{
				// Draw the sample hologram.
				m_sceneRenderer->Render();
			}
			atLeastOneCameraRendered = true;
		}

		return atLeastOneCameraRendered;
	});
}

void HolographicMain::SaveAppState()
{
}

void HolographicMain::LoadAppState()
{
}

// Notifies classes that use Direct3D device resources that the device resources
// need to be released before this method returns.
void HolographicMain::OnDeviceLost()
{
	m_sceneRenderer->ReleaseDeviceDependentResources();
}

// Notifies classes that use Direct3D device resources that the device resources
// may now be recreated.
void HolographicMain::OnDeviceRestored()
{
	m_sceneRenderer->CreateDeviceDependentResources();
}

void HolographicMain::OnLocatabilityChanged(SpatialLocator^ sender, Object^ args)
{
	switch (sender->Locatability)
	{
	case SpatialLocatability::Unavailable:
		// Holograms cannot be rendered.
	{
		String^ message = L"Warning! Positional tracking is " +
			sender->Locatability.ToString() + L".\n";
		OutputDebugStringW(message->Data());
	}
	break;

	// In the following three cases, it is still possible to place holograms using a
	// SpatialLocatorAttachedFrameOfReference.
	case SpatialLocatability::PositionalTrackingActivating:
		// The system is preparing to use positional tracking.

	case SpatialLocatability::OrientationOnly:
		// Positional tracking has not been activated.

	case SpatialLocatability::PositionalTrackingInhibited:
		// Positional tracking is temporarily inhibited. User action may be required
		// in order to restore positional tracking.
		break;

	case SpatialLocatability::PositionalTrackingActive:
		// Positional tracking is active. World-locked content can be rendered.
		break;
	}
}

void HolographicMain::OnCameraAdded(
	HolographicSpace^ sender,
	HolographicSpaceCameraAddedEventArgs^ args
)
{
	Deferral^ deferral = args->GetDeferral();
	HolographicCamera^ holographicCamera = args->Camera;
	create_task([this, deferral, holographicCamera]()
	{
		// Create device-based resources for the holographic camera and add it to the list of
		// cameras used for updates and rendering. Notes:
		//   * Since this function may be called at any time, the AddHolographicCamera function
		//     waits until it can get a lock on the set of holographic camera resources before
		//     adding the new camera. At 60 frames per second this wait should not take long.
		//   * A subsequent Update will take the back buffer from the RenderingParameters of this
		//     camera's CameraPose and use it to create the ID3D11RenderTargetView for this camera.
		//     Content can then be rendered for the HolographicCamera.
		m_deviceResources->AddHolographicCamera(holographicCamera);

		// Holographic frame predictions will not include any information about this camera until
		// the deferral is completed.
		deferral->Complete();
	});
}

void HolographicMain::OnCameraRemoved(
	HolographicSpace^ sender,
	HolographicSpaceCameraRemovedEventArgs^ args
)
{
	// Before letting this callback return, ensure that all references to the back buffer
	// are released.
	// Since this function may be called at any time, the RemoveHolographicCamera function
	// waits until it can get a lock on the set of holographic camera resources before
	// deallocating resources for this camera. At 60 frames per second this wait should
	// not take long.
	m_deviceResources->RemoveHolographicCamera(args->Camera);
}

void HolographicMain::OnInteractionDetected(SpatialInteractionManager^, SpatialInteractionDetectedEventArgs^ spArgs)
{
	m_gestureRecognizer->CaptureInteraction(spArgs->Interaction);
}
