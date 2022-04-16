#pragma once
#include <chrono>
#include <glog/logging.h>

#include <Eigen/Dense>
#include <KinectToVR_API_Devices.h>
#include <KinectToVR_API_Paths.h>

#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/xml.hpp>
#include <fstream>
#include <thread>

#include <InfoServer.h>
#include <PositionPredictor.h>
#include <UDPDeviceQuatServer.h>

/* Status enumeration */
#define R_E_CONNECTION_DEAD 0x83010001 // No connection
#define R_E_NO_DATA 0x83010002   // No data received
#define R_E_INIT_FAILED 0x83010003   // Init failed

#define R_E_DISCONNECTED 0x83010005 // Disconnected (initial)

/* Eigen serialization */

namespace cereal
{
	template <class Archive>
	void serialize(Archive& archive,
	               Eigen::Vector3d& v)
	{
		archive(v.x(), v.y(), v.z());
	}

	template <class Archive>
	void serialize(Archive& archive,
	               Eigen::Quaterniond& q)
	{
		archive(q.w(), q.x(), q.y(), q.z());
	}
}

/* Not exported */

class DeviceHandler : public ktvr::K2TrackingDeviceBase_JointsBasis
{
public:
	/* K2API's things, which KTVR will make use of */

	DeviceHandler()
	{
		LOG(INFO) << "Constructing the OWO Handler for JointsBasis K2TrackingDevice...";

		K2TrackingDeviceBase_JointsBasis::deviceType = ktvr::K2_Joints;
		K2TrackingDeviceBase_JointsBasis::deviceName = "owoTrackVR";
		K2TrackingDeviceBase_JointsBasis::settingsSupported = true;

		// Mark that our device supports settings
		settingsSupported = true;

		load_settings(); // Load settings

		// Construct the networking server
		m_data_server = new UDPDeviceQuatServer(m_net_port);
		m_info_server = new InfoServer();

		m_info_server->set_port_no(m_data_server->get_port());
		m_info_server->add_tracker();

		// Start listening
		try
		{
			m_data_server->startListening();
		}
		catch (std::system_error& e)
		{
			LOG(ERROR) << "OWO Device Error: Failed to start the data listener up!";
			LOG(ERROR) << "Error message: " << e.what();
			m_status_result = R_E_INIT_FAILED;
		}
	}

	~DeviceHandler() override
	{
	}

	void onLoad() override
	{
		// Construct the device's settings here

		// Create elements
		m_port_number_box = CreateNumberBox(m_net_port);
		m_hip_height_number_box = CreateNumberBox(-(m_tracker_offset.y() * 100.0));

		m_calibrate_forward_button = CreateButton("Calibrate Forward");
		m_calibrate_down_button = CreateButton("Calibrate Down");

		// Append the elements
		layoutRoot->AppendElementPair(
			CreateTextBlock(
				"owoTrack Server Connection Port:"),
			m_port_number_box);

		layoutRoot->AppendSingleElement(
			CreateTextBlock(" ")); // A spacer

		layoutRoot->AppendElementPair(
			CreateTextBlock(
				"Hip Height:"),
			m_hip_height_number_box);

		layoutRoot->AppendElementPair(
			m_calibrate_forward_button,
			m_calibrate_down_button);

		/* Set up the handlers TODO */

		// "Full Calibration"
		m_calibrate_forward_button->OnClick = [&, this](ktvr::Interface::Button* sender)
		{
			// set calibrating forward bool, show instructions, then reset it
		};

		// "Down Calibration"
		m_calibrate_down_button->OnClick = [&, this](ktvr::Interface::Button* sender)
		{
			// set calibrating down bool, show instructions, then reset it
		};
	}

	HRESULT getStatusResult() override;
	std::string statusResultString(HRESULT stat) override;

	void initialize() override;
	void update() override;
	void shutdown() override;
	void signalJoint(uint32_t at) override;

	/* Device's own stuff */

	void save_settings()
	{
		if (std::ofstream output(
				ktvr::GetK2AppDataFileDir("Device_OWO_settings.xml"));
			output.fail())
		{
			//LOG(ERROR) << "OWO Device Error: Couldn't save settings!\n";
		}
		else
		{
			cereal::XMLOutputArchive archive(output);
			//LOG(INFO) << "OWO Device: Attempted to save settings";

			try
			{
				archive(
					CEREAL_NVP(m_net_port),
					CEREAL_NVP(m_global_offset),
					CEREAL_NVP(m_device_offset),
					CEREAL_NVP(m_tracker_offset),
					CEREAL_NVP(m_global_rotation),
					CEREAL_NVP(m_local_rotation)
				);
			}
			catch (...)
			{
				//LOG(ERROR) << "OWO Device Error: Couldn't save settings, an exception occurred!\n";
			}
		}
	}

	void load_settings()
	{
		if (std::ifstream input(
				ktvr::GetK2AppDataFileDir("Device_OWO_settings.xml"));
			input.fail())
		{
			//LOG(WARNING) << "OWO Device Error: Couldn't read settings, re-generating!\n";
			save_settings(); // Re-generate the file
		}
		else
		{
			//LOG(INFO) << "OWO Device: Attempting to read settings";

			try
			{
				cereal::XMLInputArchive archive(input);
				archive(
					CEREAL_NVP(m_net_port),
					CEREAL_NVP(m_global_offset),
					CEREAL_NVP(m_device_offset),
					CEREAL_NVP(m_tracker_offset),
					CEREAL_NVP(m_global_rotation),
					CEREAL_NVP(m_local_rotation)
				);
			}
			catch (...)
			{
				//LOG(ERROR) << "OWO Device Error: Couldn't read settings, an exception occurred!\n";
			}
		}
	}

	// OWO Tracker Settings' Hip Dislocation
	Eigen::Vector3d m_global_offset{0, 0, 0},
	                m_device_offset{0, 0, 0},
	                m_tracker_offset{0, -0.73, 0};

	// OWO Tracker Settings' Hip Rotation
	Eigen::Quaterniond m_global_rotation{1, 0, 0, 0},
	                   m_local_rotation{1, 0, 0, 0};

	// TODO NOT BACKED UP \/

	// OWO Position predict (should be ditched for the Kalman filter?)
	bool m_should_predict_position_tracker_wise = false;
	double m_position_prediction_strength_tracker_wise = 1.0;

	// TODO NOT BACKED UP /\
	
	// OWO Interfacing Port
	uint32_t m_net_port = 6969;

	// OWO Interfacing is_calibrating
	bool m_is_calibrating_forward = false,
	     m_is_calibrating_down = false;

	// Interface elements
	ktvr::Interface::NumberBox *m_port_number_box, *m_hip_height_number_box;
	ktvr::Interface::Button *m_calibrate_forward_button, *m_calibrate_down_button;

	/* Internal, helper variables */
	UDPDeviceQuatServer* m_data_server;
	InfoServer* m_info_server;
	PositionPredictor m_pos_predictor;

	HRESULT m_status_result = R_E_DISCONNECTED;

	std::unique_ptr<std::thread> m_update_server_thread;
	void update_server_thread_worker()
	{
		while (initialized)
		{
			/* Update the discovery server here */
			try
			{
				m_info_server->tick();
			}
			catch (std::system_error& e)
			{
				LOG(ERROR) << "OWO Device Error: Info server tick (heartbeat) failed!";
				LOG(ERROR) << "Error message: " << e.what();
			}

			/* Update the data server here */
			try
			{
				m_data_server->tick();
			}
			catch (std::system_error& e)
			{
				LOG(ERROR) << "OWO Device Error: Data listener tick (heartbeat) failed!";
				LOG(ERROR) << "Error message: " << e.what();
			}

			if (!m_data_server->isDataAvailable())
			{
				skeletonTracked = false;
				m_status_result =
					m_data_server->isConnectionAlive()
						? R_E_NO_DATA
						: R_E_CONNECTION_DEAD;
			}
			else m_status_result = S_OK;

			std::this_thread::sleep_for(
				std::chrono::milliseconds(22));
		}
	}
};

/* Exported for dynamic linking */
extern "C" __declspec(dllexport) void* TrackingDeviceBaseFactory(
	const char* pVersionName, int* pReturnCode)
{
	// Return the device handler for tracking
	// but only if interfaces are the same / up-to-date
	if (0 == strcmp(ktvr::IK2API_Devices_Version, pVersionName))
	{
		static DeviceHandler TrackingHandler; // Create a new device handler -> KinectV2

		*pReturnCode = ktvr::K2InitError_None;
		return &TrackingHandler;
	}

	// Return code for initialization
	*pReturnCode = ktvr::K2InitError_BadInterface;
}
