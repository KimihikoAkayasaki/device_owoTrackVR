#include "pch.h"
#include "DeviceHandler.h"

#include <iostream>
#include <chrono>
#include <ppl.h>

#include <basis.h>

HRESULT DeviceHandler::getStatusResult()
{
	return m_status_result;
}

std::string DeviceHandler::statusResultString(HRESULT stat)
{
	// Parse your device's status into some nice text here,
	// it has to be formatted like [HEADER]\n[TYPE]\n[MESSAGE]

	switch (stat)
	{
	case S_OK: return "Success!\nS_OK\nEverything's good!";

	case R_E_CONNECTION_DEAD: return
			"Connection error!\nE_CONNECTION_DEAD\nCheck if owoTrackVR app is running, working and connected.";

	case R_E_NO_DATA: return
			"Connection error!\nE_NO_DATA\nCheck if owoTrackVR app is running, working and connected.";

	case R_E_INIT_FAILED: return
			"Listener startup fail!\nE_INIT_FAILED\nThe data server failed to start, check logs and port number.";

	case R_E_DISCONNECTED: return
			"Connection error!\nE_DISCONNECTED\nStart owoTrackVR app and try to connect it with this computer.";

	case S_FALSE:
	default: return "Undefined: " + std::to_string(stat) +
			"\nE_UNDEFINED\nSomething weird has happened, though we can't tell what.";
	}
}

void DeviceHandler::initialize()
{
	// Initialize your device here
	trackedJoints.clear();
	trackedJoints.push_back(ktvr::K2TrackedJoint("Default"));

	// Mark the device as initialized
	if (m_status_result != R_E_INIT_FAILED)
	{
		initialized = false; // Mark the device as NOT initialized (kill)
		if (m_update_server_thread)
			m_update_server_thread->join();

		initialized = true; // Mark the device as initialized
		m_update_server_thread.reset(new std::thread(&DeviceHandler::update_server_thread_worker, this));
	}
}

void DeviceHandler::update()
{
	// Update joints' positions here
	// Note: this is fired up every loop

	if (isInitialized())
	{
		// Make sure that we're running correctly
		if (m_status_result != S_OK)return;

		// Mark that we see the user
		skeletonTracked = true;

		/* Prepare for the position calculations */

		Basis offset_basis;

		Vector3 offset_global = m_global_offset;
		Vector3 offset_local_device = m_device_offset;
		Vector3 offset_local_tracker = m_tracker_offset;

		std::pair<Eigen::Vector3f, Eigen::Quaternionf> pose{Eigen::Vector3f{0, 0, 0}, getHMDOrientation()};
		Eigen::Matrix3f rotation = getHMDOrientation().toRotationMatrix();

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

		double const* p_remote_rotation = m_data_server->getRotationQuaternion();

		Quat p_remote_quaternion = Quat(
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
				Quat(Vector3(0, 1, 0), -getHMDOrientationYaw())).to_eigen<double>();

		p_remote_quaternion = p_remote_quaternion * Quat(m_local_rotation);
		pose.second = p_remote_quaternion.to_eigen<float>();

		// Angular velocity is not used as of now
		// double* gyro = m_data_server->getGyroscope();

		Basis final_tracker_basis = Basis(p_remote_quaternion);

		for (int i = 0; i < 3; i++)
		{
			pose.first(i) += offset_global.get_axis(i);
			pose.first(i) += offset_basis.xform(offset_local_device).get_axis(i);
			pose.first(i) += final_tracker_basis.xform(offset_local_tracker).get_axis(i);
		}

		if (!m_is_calibrating_forward && m_should_predict_position_tracker_wise)
		{
			Basis b = Basis(p_remote_quaternion);
			Vector3 result = m_pos_predictor.predict(*m_data_server, b) *
				m_position_prediction_strength_tracker_wise;

			pose.first(0) += result.x;
			pose.first(1) += result.y;
			pose.first(2) += result.z;
		}

		/* Send the positions to the host */

		trackedJoints.at(0).update(
			pose.first,
			pose.second,
			ktvr::State_Tracked);
	}
}

void DeviceHandler::shutdown()
{
	// Turn your device off here

	initialized = false; // Mark the device as NOT initialized (kill)
	if (m_update_server_thread)
		m_update_server_thread->join();
}

void DeviceHandler::signalJoint(uint32_t at)
{
	m_data_server->buzz(0.7, 100.0, 0.5);
}
