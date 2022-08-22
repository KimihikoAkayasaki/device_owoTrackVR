#include "pch.h"
#include "DeviceHandler.h"

#include <iostream>
#include <chrono>
#include <ppl.h>

#include <basis.h>

HRESULT DeviceHandler::getStatusResult()
{
	if (hasBeenLoaded)
	{
		m_ip_text_block->Visibility(true);
		m_port_text_block->Visibility(true);
	}

	update_ui_worker();
	return m_status_result;
}

std::wstring DeviceHandler::statusResultWString(HRESULT stat)
{
	// Parse your device's status into some nice text here,
	// it has to be formatted like [HEADER]\n[TYPE]\n[MESSAGE]
	if (hasBeenLoaded)
		switch (stat)
		{
		case S_OK: return requestLocalizedString(L"/Plugins/OWO/Statuses/Success");
		case R_E_CONNECTION_DEAD: return requestLocalizedString(L"/Plugins/OWO/Statuses/ConnectionDead");
		case R_E_NO_DATA: return requestLocalizedString(L"/Plugins/OWO/Statuses/NoData");
		case R_E_INIT_FAILED: return requestLocalizedString(L"/Plugins/OWO/Statuses/InitFailure");
		case R_E_NOT_STARTED: return requestLocalizedString(L"/Plugins/OWO/Statuses/NotStarted");
		case S_FALSE:
		default: return L"Undefined: " + std::to_wstring(stat) +
				L"\nE_UNDEFINED\nSomething weird has happened, though we can't tell what.";
		}

	return L"Undefined: " + std::to_wstring(stat) +
		L"\nE_UNDEFINED\nSomething weird has happened, though we can't tell what.";
}

void DeviceHandler::initialize()
{
	// Initialize your device here
	settingsSupported = true;
	trackedJoints.clear();
	trackedJoints.push_back(ktvr::K2TrackedJoint("Default"));

	// Optionally initialize the server
	// (Warning: this can be done only once)
	if (m_status_result == R_E_NOT_STARTED)
	{
		// Construct the networking server
		m_data_server = new UDPDeviceQuatServer(&m_net_port);
		m_info_server = new InfoServer();

		m_info_server->set_port_no(m_data_server->get_port());
		m_info_server->add_tracker();

		// Start listening
		try
		{
			m_data_server->startListening();
			m_status_result = R_E_CONNECTION_DEAD;
		}
		catch (std::system_error& e)
		{
			LOG(ERROR) << "OWO Device Error: Failed to start the data listener up!";
			LOG(ERROR) << "Error message: " << e.what();
			m_status_result = R_E_INIT_FAILED;

			if (hasBeenLoaded)
			{
				m_message_text_block->Text(requestLocalizedString(L"/Plugins/OWO/Notices/Failure"));
			}
		}
	}

	// Mark the device as initialized
	if (m_status_result != R_E_INIT_FAILED)
	{
		initialized = true; // Mark the device as initialized

		if (!m_update_server_thread)
			m_update_server_thread.reset(new std::thread(&DeviceHandler::update_server_thread_worker, this));

		m_is_calibrating_forward = false;
		m_is_calibrating_down = false;

		update_ui_worker();
	}
}

void DeviceHandler::update()
{
	// Update joints' positions here
	// Note: this is fired up every loop

	if (initialized)
	{
		// Make sure that we're running correctly
		if (m_status_result != S_OK)return;

		// Note: All positions and orientations
		// are already calculated in the server thread

		/* Send the positions to the host */

		trackedJoints.at(0).update(
			m_pose.first,
			m_pose.second,
			ktvr::State_Tracked);
	}
}

void DeviceHandler::shutdown()
{
	// Turn your device off here

	initialized = false;
	save_settings(); // Back everything up
}

void DeviceHandler::signalJoint(uint32_t at)
{
	m_data_server->buzz(0.7, 100.0, 0.5);
}

void DeviceHandler::calculatePose()
{
	// Mark that we see the user
	skeletonTracked = true;

	/* Prepare for the position calculations */

	Basis offset_basis;

	Vector3 offset_global = m_global_offset;
	Vector3 offset_local_device = m_device_offset;
	Vector3 offset_local_tracker = m_tracker_offset;
	m_pose.first = getHMDPoseCalibrated().first; // Zero the position vector

	Eigen::Matrix3f rotation = getHMDPoseCalibrated().second.toRotationMatrix();
	offset_basis.set(
		rotation(0, 0),
		rotation(0, 1),
		rotation(0, 2),
		rotation(1, 0),
		rotation(1, 1),
		rotation(1, 2),
		rotation(2, 0),
		rotation(2, 1),
		rotation(2, 2)
	);

	/* Parse and calculate the positions */

	// Acceleration is not used as of now
	// double* acceleration = m_data_server->getAccel();

	const double* p_remote_rotation = m_data_server->getRotationQuaternion();

	auto p_remote_quaternion = Quat(
		p_remote_rotation[0], p_remote_rotation[1],
		p_remote_rotation[2], p_remote_rotation[3]);

	p_remote_quaternion =
		Quat(Vector3(1, 0, 0), -Math_PI / 2.0) * p_remote_quaternion;


	if (m_is_calibrating_forward)
	{
		m_global_rotation =
			Quat(Vector3(0, (get_yaw(p_remote_quaternion)) -
			             (get_yaw(offset_basis, Vector3(0, 0, -1))), 0)).to_eigen<double>();

		offset_global = (offset_basis.xform(Vector3(0, 0, -1)) *
			Vector3(1, 0, 1)).normalized() + Vector3(0, 0.2, 0);
		offset_local_device = Vector3(0, 0, 0);
		offset_local_tracker = Vector3(0, 0, 0);
	}

	p_remote_quaternion = Quat(m_global_rotation) * p_remote_quaternion;

	if (m_is_calibrating_down)
		m_local_rotation =
		(Quat(p_remote_quaternion.inverse().get_euler_yxz()) *
			Quat(Vector3(0, 1, 0), -getHMDOrientationYawCalibrated())).to_eigen<double>();

	p_remote_quaternion = p_remote_quaternion * Quat(m_local_rotation);
	m_pose.second = p_remote_quaternion.to_eigen<float>();

	// Angular velocity is not used as of now
	// double* gyro = m_data_server->getGyroscope();

	auto final_tracker_basis = Basis(p_remote_quaternion);

	for (int i = 0; i < 3; i++)
	{
		m_pose.first(i) += offset_global.get_axis(i);
		m_pose.first(i) += offset_basis.xform(offset_local_device).get_axis(i);
		m_pose.first(i) += final_tracker_basis.xform(offset_local_tracker).get_axis(i);
	}

	if (!m_is_calibrating_forward && m_should_predict_position_tracker_wise)
	{
		auto b = Basis(p_remote_quaternion);
		Vector3 result = m_pos_predictor.predict(*m_data_server, b) *
			m_position_prediction_strength_tracker_wise;

		m_pose.first(0) += result.x;
		m_pose.first(1) += result.y;
		m_pose.first(2) += result.z;
	}
}
