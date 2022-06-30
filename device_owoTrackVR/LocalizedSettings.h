#pragma once
#include <map>
#include <string>

/* The locale maps */

// Map each status to all available locales
inline std::map<std::wstring, std::wstring> local_ip_single_label_map
{
	{
		L"en-US",
		L"Your Local IP: "
	},
	{
		L"lc-LC",
		L"Your Local IP: "
	}
};

inline std::map<std::wstring, std::wstring> local_ip_multiple_label_map
{
	{
		L"en-US",
		L"Your Local IP: (One of) "
	},
	{
		L"lc-LC",
		L"Your Local IP: (One of) "
	}
};

inline std::map<std::wstring, std::wstring> connection_port_label_map
{
	// Yes, along with the \n
	{
		L"en-US",
		L"Connection Port: \n"
	},
	{
		L"lc-LC",
		L"Connection Port: \n"
	}
};

inline std::map<std::wstring, std::wstring> notice_not_started_map
{
	// Yes, along with the \n
	{
		L"en-US",
		L"Please start the server first!"
	},
	{
		L"lc-LC",
		L"Please start the server first!"
	}
};

inline std::map<std::wstring, std::wstring> notice_not_connected_map
{
	// Yes, along with the \n
	{
		L"en-US",
		L"Please connect your phone!"
	},
	{
		L"lc-LC",
		L"Please connect your phone!"
	}
};

inline std::map<std::wstring, std::wstring> button_calibrate_forward_map
{
	// Yes, along with the \n
	{
		L"en-US",
		L"Calibrate Forward"
	},
	{
		L"lc-LC",
		L"Calibrate Forward"
	}
};

inline std::map<std::wstring, std::wstring> button_calibrate_down_map
{
	// Yes, along with the \n
	{
		L"en-US",
		L"Calibrate Down"
	},
	{
		L"lc-LC",
		L"Calibrate Down"
	}
};

inline std::map<std::wstring, std::wstring> calibration_instructions_forward_map
{
	// \n is a line break
	{
		L"en-US",
		L"Hold your phone in the same direction\nas your VR headset orientation, screen facing up..."
	},
	{
		L"lc-LC",
		L"Hold your phone in the same direction\nas your VR headset orientation, screen facing up..."
	}
};

inline std::map<std::wstring, std::wstring> calibration_instructions_down_map
{
	// \n is a line break
	{
		L"en-US",
		L"Hold your phone in the same direction\nas your VR headset orientation, screen facing up..."
	},
	{
		L"lc-LC",
		L"Hold your phone in the same direction\nas your VR headset orientation, screen facing up..."
	}
};

inline std::map<std::wstring, std::wstring> stay_still_map
{
	// \n is a line break
	{
		L"en-US",
		L"Please stay like that a bit..."
	},
	{
		L"lc-LC",
		L"Please stay like that a bit..."
	}
};


inline std::map<std::wstring, std::wstring> server_failure_map
{
	// \n is a line break
	{
		L"en-US",
		L"Server has failed to start up!"
	},
	{
		L"lc-LC",
		L"Server has failed to start up!"
	}
};
