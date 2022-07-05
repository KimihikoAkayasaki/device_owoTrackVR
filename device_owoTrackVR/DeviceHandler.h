#pragma once
#include <chrono>
#include <glog/logging.h>

#include <Eigen/Dense>
#include <Amethyst_API_Devices.h>
#include <Amethyst_API_Paths.h>

#include "LocalizedStatuses.h"
#include "LocalizedSettings.h"

#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/xml.hpp>

#include <fstream>
#include <thread>
#include <WinSock2.h>
#include <iphlpapi.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

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

		deviceType = ktvr::K2_Joints;
		deviceName = "owoTrackVR";
		settingsSupported = false; // Not yet, but soonTM

		load_settings(); // Load settings
	}

	~DeviceHandler() override
	{
	}

	bool hasBeenLoaded = false,
	     calibrationPending = false;

	void onLoad() override
	{
		// Construct the device's settings here

		// Create elements
		m_ip_label_text_block = CreateTextBlock(
			GetLocalizedStatusWStringAutomatic(local_ip_single_label_map));
		m_ip_text_block = CreateTextBlock(L"127.0.0.1");

		m_port_label_text_block = CreateTextBlock(
			GetLocalizedStatusWStringAutomatic(connection_port_label_map));
		m_port_text_block = CreateTextBlock(std::to_wstring(m_net_port) + L"\n");

		m_ip_label_text_block->IsPrimary(false);
		m_port_label_text_block->IsPrimary(false);

		m_message_text_block = CreateTextBlock(
			GetLocalizedStatusWStringAutomatic(notice_not_started_map));

		m_calibration_text_block = CreateTextBlock(L"");
		m_calibration_text_block->Visibility(false);

		m_calibrate_forward_button = CreateButton(
			GetLocalizedStatusWStringAutomatic(button_calibrate_forward_map));
		m_calibrate_down_button = CreateButton(
			GetLocalizedStatusWStringAutomatic(button_calibrate_down_map));

		// Set up elements
		m_calibrate_forward_button->IsEnabled(false);
		m_calibrate_down_button->IsEnabled(false);

		// Append the elements : Static Data
		layoutRoot->AppendElementPairStack(
			m_ip_label_text_block,
			m_ip_text_block);

		layoutRoot->AppendElementPairStack(
			m_port_label_text_block,
			m_port_text_block);

		// Append the elements : Dynamic Data
		layoutRoot->AppendSingleElement(
			m_message_text_block,
			ktvr::Interface::SingleLayoutHorizontalAlignment::Left);

		layoutRoot->AppendElementPairStack(
			m_calibrate_forward_button,
			m_calibrate_down_button);

		layoutRoot->AppendSingleElement(m_calibration_text_block);

		// Hide post-init ui elements
		m_ip_text_block->Visibility(false);
		m_port_text_block->Visibility(false);

		m_calibrate_forward_button->Visibility(false);
		m_calibrate_down_button->Visibility(false);

		// Mark everything as set up
		hasBeenLoaded = true;

		// Set up particular handlers

		// "Full Calibration"
		m_calibrate_forward_button->OnClick = [&, this](ktvr::Interface::Button* sender)
		{
			// set calibrating forward bool, show instructions, then reset it
			std::thread([&, this]
			{
				if (!initialized)return;

				calibrationPending = true;

				m_calibration_text_block->Visibility(true);
				m_calibration_text_block->Text(
					GetLocalizedStatusWStringAutomatic(calibration_instructions_forward_map));

				m_calibrate_forward_button->IsEnabled(false);
				m_calibrate_down_button->IsEnabled(false);

				std::this_thread::sleep_for(std::chrono::seconds(7));
				if (!initialized)
				{
					m_is_calibrating_forward = false;
					m_calibration_text_block->Visibility(false);
					return; // Abort
				}

				m_is_calibrating_forward = true;

				m_calibration_text_block->Text(GetLocalizedStatusWStringAutomatic(stay_still_map));
				std::this_thread::sleep_for(std::chrono::seconds(4));

				m_is_calibrating_forward = false;
				m_calibration_text_block->Visibility(false);

				calibrationPending = false;

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

				calibrationPending = true;

				m_calibration_text_block->Visibility(true);
				m_calibration_text_block->Text(
					GetLocalizedStatusWStringAutomatic(calibration_instructions_down_map));

				m_calibrate_forward_button->IsEnabled(false);
				m_calibrate_down_button->IsEnabled(false);

				std::this_thread::sleep_for(std::chrono::seconds(7));
				if (!initialized)
				{
					m_is_calibrating_forward = false;
					m_calibration_text_block->Visibility(false);
					return; // Abort
				}

				m_is_calibrating_down = true;

				m_calibration_text_block->Text(GetLocalizedStatusWStringAutomatic(stay_still_map));
				std::this_thread::sleep_for(std::chrono::seconds(4));

				m_is_calibrating_down = false;
				m_calibration_text_block->Visibility(false);

				calibrationPending = false;

				save_settings(); // Back everything up
				update_ui_thread_worker();
			}).detach();
		};

		// Grab & push local ip addresses to the UI
		std::thread([&]
		{
			DWORD size;
			std::vector<std::string> _addr_vector;

			if (GetAdaptersAddresses(
					AF_INET, GAA_FLAG_INCLUDE_PREFIX,
					nullptr, nullptr, &size)
				!= ERROR_BUFFER_OVERFLOW)
			{
				LOG(ERROR) << "OWO Device Error: ERROR_BUFFER_OVERFLOW when getting local host name";
				return;
			}

			const auto adapter_addresses = static_cast<PIP_ADAPTER_ADDRESSES>(malloc(size));
			if (GetAdaptersAddresses(
					AF_INET, GAA_FLAG_INCLUDE_PREFIX,
					nullptr, adapter_addresses, &size)
				!= ERROR_SUCCESS)
			{
				LOG(ERROR) << "OWO Device Error: " << WSAGetLastError() << " when getting local host name";
				free(adapter_addresses);
				return;
			}

			for (auto aa = adapter_addresses; aa != nullptr; aa = aa->Next)
			{
				if (aa->OperStatus != IfOperStatusUp) continue;
				if (std::wstring(aa->Description).find(L"Virtual") != std::wstring::npos) continue;

				for (auto ua = aa->FirstUnicastAddress; ua != nullptr; ua = ua->Next)
				{
					char buf[BUFSIZ]{};
					getnameinfo(ua->Address.lpSockaddr,
					            ua->Address.iSockaddrLength,
					            buf, sizeof(buf),
					            nullptr, 0,
					            NI_NUMERICHOST);

					// If everything's right, push back to the vec
					if (std::string(buf) != "127.0.0.1")
						_addr_vector.push_back(buf);
				}
			}

			free(adapter_addresses);

			// Append to the UI
			if (!_addr_vector.empty())
			{
				std::string _addr_str =
					_addr_vector.size() > 1 ? "[ " : "";

				for (const auto& _address : _addr_vector)
					_addr_str += _address + ", ";

				_addr_str = _addr_str.substr(0, _addr_str.rfind(","));
				if (_addr_vector.size() > 1)_addr_str += " ]"; // End array

				m_ip_label_text_block->Text(
					_addr_vector.size() > 1
						? GetLocalizedStatusWStringAutomatic(local_ip_multiple_label_map)
						: GetLocalizedStatusWStringAutomatic(local_ip_single_label_map));

				m_ip_text_block->Text(StringToWString(_addr_str));
			}
		}).detach();
	}

	HRESULT getStatusResult() override;
	std::wstring statusResultWString(HRESULT stat) override;

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
	ktvr::Interface::TextBlock *m_ip_text_block, *m_ip_label_text_block,
	                           *m_port_text_block, *m_port_label_text_block,
	                           *m_message_text_block, *m_calibration_text_block;
	ktvr::Interface::NumberBox* m_hip_height_number_box;
	ktvr::Interface::Button *m_calibrate_forward_button, *m_calibrate_down_button;

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

	bool update_ui_thread_worker_pending = false;

	void update_ui_thread_worker()
	{
		if (!m_is_calibrating_forward && !m_is_calibrating_down
			&& initialized && hasBeenLoaded)
		{
			if (m_status_result == S_OK)
			{
				m_message_text_block->Visibility(false);

				m_calibrate_forward_button->Visibility(true);
				m_calibrate_down_button->Visibility(true);

				if (!calibrationPending)
				{
					m_calibrate_forward_button->IsEnabled(true);
					m_calibrate_down_button->IsEnabled(true);
				}
			}
			else
			{
				m_message_text_block->Visibility(true);
				m_message_text_block->Text(
					GetLocalizedStatusWStringAutomatic(notice_not_connected_map));

				m_calibrate_forward_button->Visibility(false);
				m_calibrate_down_button->Visibility(false);

				if (!calibrationPending)
				{
					m_calibrate_forward_button->IsEnabled(false);
					m_calibrate_down_button->IsEnabled(false);
				}

				std::thread([&, this]
				{
					if (!update_ui_thread_worker_pending)
					{
						update_ui_thread_worker_pending = true;
						std::this_thread::sleep_for(std::chrono::seconds(1));
						update_ui_thread_worker(); // Try again a bit later
						update_ui_thread_worker_pending = false;
					}
				}).detach();
			}
		}
	}

	[[noreturn]] void update_server_thread_worker()
	{
		// How many retries have been made before marking
		// the connection dead (assume max 180 retries or 3 seconds)
		int32_t e_retries = 0;

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
					if (e_retries >= 180)
					{
						e_retries = 0; // Reset
						skeletonTracked = false;
						m_status_result =
							m_data_server->isConnectionAlive()
								? R_E_NO_DATA
								: R_E_CONNECTION_DEAD;
					}
					else e_retries++;
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
		static auto TrackingHandler = new DeviceHandler(); // Create a new device handler -> owoTrack

		*pReturnCode = ktvr::K2InitError_None;
		return TrackingHandler;

		// If you wanna know why of all things, owo is constructed by `new` as an exception workaround
		// then you should read this: (it was causing a crash with its destructor, now the destructor isn't called)
		// https://stackoverflow.com/questions/54253145/c-application-crashes-with-atexit-error#comment95332406_54253145
	}

	// Return code for initialization
	*pReturnCode = ktvr::K2InitError_BadInterface;
}
