#pragma once
#include <chrono>
#include <glog/logging.h>

#include <Eigen/Dense>
#include <Amethyst_API_Devices.h>
#include <Amethyst_API_Paths.h>

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

#define R_E_NOT_STARTED 0x83010005 // Disconnected (initial)

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
	}

	virtual ~DeviceHandler()
	{
	}

	bool hasBeenLoaded = false;

	void onLoad() override
	{
		// Construct the device's settings here

		// Create elements
		m_ip_text_block = CreateTextBlock("Your Local IP: [ 127.0.0.1 ]");
		m_port_text_block = CreateTextBlock("Connection Port: [ " + std::to_string(m_net_port) + " ]");

		m_message_text_block = CreateTextBlock("Please start the server first!");
		m_main_progress_bar = CreateProgressBar();

		//m_hip_height_number_box = CreateNumberBox(-(m_tracker_offset.y() * 100.0));

		m_calibrate_forward_button = CreateButton("Calibrate Forward");
		m_calibrate_down_button = CreateButton("Calibrate Down");

		// Set up elements
		m_main_progress_bar->Width(306);
		m_main_progress_bar->Progress(100);
		m_main_progress_bar->ShowPaused(true);

		m_calibrate_forward_button->Width(150);
		m_calibrate_down_button->Width(150);

		m_calibrate_forward_button->IsEnabled(false);
		m_calibrate_down_button->IsEnabled(false);

		// Append the elements : Static Data
		layoutRoot->AppendSingleElement(
			m_ip_text_block);

		layoutRoot->AppendSingleElement(
			m_port_text_block);

		// Append the elements : Spacer
		layoutRoot->AppendSingleElement(
			CreateTextBlock(" ")); // A spacer

		layoutRoot->AppendSingleElement(
			m_message_text_block,
			ktvr::Interface::SingleLayoutHorizontalAlignment::Center);

		layoutRoot->AppendSingleElement(
			m_main_progress_bar,
			ktvr::Interface::SingleLayoutHorizontalAlignment::Center);

		//layoutRoot->AppendElementPair(
		//	CreateTextBlock(
		//		"Hip Height:"),
		//	m_hip_height_number_box);

		layoutRoot->AppendElementPair(
			m_calibrate_forward_button,
			m_calibrate_down_button);

		// Set up particular handlers

		// "Full Calibration"
		m_calibrate_forward_button->OnClick = [&, this](ktvr::Interface::Button* sender)
		{
			// set calibrating forward bool, show instructions, then reset it
			std::thread([&, this]
			{
				if (!initialized)return;

				m_message_text_block->Text("Point your phone forward, screen up...");
				m_main_progress_bar->Progress(100);
				m_main_progress_bar->ShowPaused(true);

				m_calibrate_forward_button->IsEnabled(false);
				m_calibrate_down_button->IsEnabled(false);

				std::this_thread::sleep_for(std::chrono::seconds(4));
				if (!initialized)return;
				m_is_calibrating_forward = true;

				m_main_progress_bar->ShowPaused(false);
				m_main_progress_bar->Progress(-1);

				m_message_text_block->Text("Please stay like that a bit...");
				std::this_thread::sleep_for(std::chrono::seconds(4));

				m_is_calibrating_forward = false;
				save_settings(); // Back everything up
				update_ui_thread_worker();
			}).detach();
		};

		// "Down Calibration"
		m_calibrate_down_button->OnClick = [&, this](ktvr::Interface::Button* sender)
		{
			// set calibrating down bool, show instructions, then reset it
			std::thread([&, this]
			{
				if (!initialized)return;

				m_message_text_block->Text("Mount your phone in its place now...");
				m_main_progress_bar->Progress(100);
				m_main_progress_bar->ShowPaused(true);

				m_calibrate_forward_button->IsEnabled(false);
				m_calibrate_down_button->IsEnabled(false);

				std::this_thread::sleep_for(std::chrono::seconds(4));
				if (!initialized)return;
				m_is_calibrating_down = true;

				m_main_progress_bar->ShowPaused(false);
				m_main_progress_bar->Progress(-1);

				m_message_text_block->Text("Please stay like that a bit...");
				std::this_thread::sleep_for(std::chrono::seconds(4));

				m_is_calibrating_down = false;
				save_settings(); // Back everything up
				update_ui_thread_worker();
			}).detach();
		};

		// Grab & push local ip addresses to the UI
		std::thread([&]
		{
			char ac[80];
			if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR)
			{
				LOG(ERROR) << "OWO Device Error: " << WSAGetLastError() <<
					" when getting local host name";
				return;
			}
			LOG(INFO) << "OWO Device: My host name is " << ac;

			hostent* phe = gethostbyname(ac);
			if (phe == 0)
			{
				LOG(ERROR) << "OWO Device Error: Bad host lookup";
				return;
			}

			// Append to the UI
			std::string _addr_str("Your Local IP: (One of) [ ");
			int _format_i = 1;

			for (int i = 0; phe->h_addr_list[i] != 0; ++i)
			{
				in_addr addr;
				memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));

				_addr_str.append(inet_ntoa(addr));

				if (_format_i < 1)
				{
					_addr_str.append(", ");
					_format_i++;
				}
				else
				{
					_addr_str.append(",\n                      ");
					_format_i = 0;
				}
			}

			_addr_str = _addr_str.substr(
				0, _addr_str.rfind(","));
			m_ip_text_block->Text(_addr_str + " ]");
		}).detach();

		// Hide post-init ui elements
		m_ip_text_block->Visibility(false);
		m_port_text_block->Visibility(false);
		
		m_calibrate_forward_button->Visibility(false);
		m_calibrate_down_button->Visibility(false);

		// Mark everything as set up
		hasBeenLoaded = true;
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
			LOG(ERROR) << "OWO Device Error: Couldn't save settings!\n";
		}
		else
		{
			cereal::XMLOutputArchive archive(output);
			LOG(INFO) << "OWO Device: Attempted to save settings";

			try
			{
				archive(
					//CEREAL_NVP(m_net_port),
					CEREAL_NVP(m_global_offset),
					CEREAL_NVP(m_device_offset),
					CEREAL_NVP(m_tracker_offset),
					CEREAL_NVP(m_global_rotation),
					CEREAL_NVP(m_local_rotation)
				);
			}
			catch (...)
			{
				LOG(ERROR) << "OWO Device Error: Couldn't save settings, an exception occurred!\n";
			}
		}
	}

	void load_settings()
	{
		if (std::ifstream input(
				ktvr::GetK2AppDataFileDir("Device_OWO_settings.xml"));
			input.fail())
		{
			LOG(WARNING) << "OWO Device Error: Couldn't read settings, re-generating!\n";
			save_settings(); // Re-generate the file
		}
		else
		{
			LOG(INFO) << "OWO Device: Attempting to read settings";

			try
			{
				cereal::XMLInputArchive archive(input);
				archive(
					//CEREAL_NVP(m_net_port),
					CEREAL_NVP(m_global_offset),
					CEREAL_NVP(m_device_offset),
					CEREAL_NVP(m_tracker_offset),
					CEREAL_NVP(m_global_rotation),
					CEREAL_NVP(m_local_rotation)
				);
			}
			catch (...)
			{
				LOG(ERROR) << "OWO Device Error: Couldn't read settings, an exception occurred!\n";
			}
		}
	}

	// OWO Tracker Settings' Hip Dislocation
	Eigen::Vector3d m_global_offset{0, 0, 0},
	                m_device_offset{0, -0.045, 0.09},
	                m_tracker_offset{0, -0.75, 0};

	// OWO Tracker Settings' Hip Rotation
	Eigen::Quaterniond m_global_rotation{1, 0, 0, 0},
	                   m_local_rotation{1, 0, 0, 0};

	// TODO NOT BACKED UP \/

	// OWO Position predict (should be ditched for the Kalman filter?)
	bool m_should_predict_position_tracker_wise = false;
	double m_position_prediction_strength_tracker_wise = 1.0;

	// OWO Interfacing Port
	uint32_t m_net_port = 6969;

	// TODO NOT BACKED UP /\
	
	// OWO Interfacing is_calibrating
	bool m_is_calibrating_forward = false,
	     m_is_calibrating_down = false;

	// Interface elements
	ktvr::Interface::TextBlock *m_ip_text_block, *m_port_text_block, *m_message_text_block;
	ktvr::Interface::NumberBox* m_hip_height_number_box;
	ktvr::Interface::Button *m_calibrate_forward_button, *m_calibrate_down_button;
	ktvr::Interface::ProgressBar* m_main_progress_bar;

	/* Internal, helper variables */
	UDPDeviceQuatServer* m_data_server;
	InfoServer* m_info_server;
	PositionPredictor m_pos_predictor;

	HRESULT m_status_result = R_E_NOT_STARTED;

	void calculatePose(); // Implemented in .cpp
	std::pair<Eigen::Vector3f, Eigen::Quaternionf> m_pose
	{
		Eigen::Vector3f(0, 0, 0), Eigen::Quaternionf(1, 0, 0, 0)
	};

	std::unique_ptr<std::thread> m_update_server_thread,
	                             m_update_ui_thread;

	void update_ui_thread_worker()
	{
		if (!m_is_calibrating_forward && !m_is_calibrating_down
			&& initialized && hasBeenLoaded)
		{
			if (m_status_result == S_OK)
			{
				m_message_text_block->Text("owoTrackVR Running OK");
				m_main_progress_bar->Progress(100);
				m_main_progress_bar->ShowPaused(false);

				m_calibrate_forward_button->IsEnabled(true);
				m_calibrate_down_button->IsEnabled(true);
			}
			else
			{
				m_message_text_block->Text("Please connect your phone!");
				m_main_progress_bar->Progress(100);
				m_main_progress_bar->ShowPaused(true);

				m_calibrate_forward_button->IsEnabled(false);
				m_calibrate_down_button->IsEnabled(false);
			}
		}
	}

	[[noreturn]] void update_server_thread_worker()
	{
		while (true)
		{
			if (initialized)
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
				else
				{
					m_status_result = S_OK;

					/* Calculate the pose here */
					calculatePose();
				}
			}

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
		static DeviceHandler* TrackingHandler = new DeviceHandler(); // Create a new device handler -> owoTrack

		*pReturnCode = ktvr::K2InitError_None;
		return TrackingHandler;

		// If you wanna know why of all things, owo is constructed by `new` as an exception workaround
		// then you should read this: (it was causing a crash with its destructor, now the destructor isn't called)
		// https://stackoverflow.com/questions/54253145/c-application-crashes-with-atexit-error#comment95332406_54253145
	}

	// Return code for initialization
	*pReturnCode = ktvr::K2InitError_BadInterface;
}
