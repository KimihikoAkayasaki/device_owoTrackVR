#pragma once
#include <map>
#include <string>

/* The locale maps */

// Map each status to all available locales
inline std::map<std::wstring, std::wstring> status_ok_map
{
	{
		L"en-US",
		L"Success!\nS_OK\nEverything's good!"
	},
	{
		L"lc-LC",
		L"Shucshesh!\nS_OK\nEvewytshin's gud!"
	},
	{
		L"fr-FR",
		L"Succès!\nS_OK\nTout fonctionne!"
	}
};

inline std::map<std::wstring, std::wstring> status_dead_map
{
	{
		L"en-US",
		L"Connection error!\nE_CONNECTION_DEAD\nCheck if owoTrackVR app is running, working and connected."
	},
	{
		L"lc-LC",
		L"Connecshion ewwow!\nE_CONNECTION_DEAD\nCshechk f owoTrackVR ap s wunninwg, wowkinwg n connecshted."
	},
	{
		L"fr-FR",
		L"Erreur de connexion!\nE_CONNECTION_DEAD\nVérifiez si l'app owoTrackVR fonctionne et est connectée."
	}
};

inline std::map<std::wstring, std::wstring> status_no_data_map
{
	{
		L"en-US",
		L"Connection error!\nE_NO_DATA\nCheck if owoTrackVR app is running, working and connected."
	},
	{
		L"lc-LC",
		L"Connecshion ewwow!\nE_NO_DATA\nCshechk f owoTrackVR ap s wunninwg, wowkinwg n connecshted."
	},
	{
		L"fr-FR",
		L"Erreur de connexion!\nE_NO_DATA\nVérifiez si l'app owoTrackVR fonctionne et est connectée."
	}
};

inline std::map<std::wstring, std::wstring> status_init_fail_map
{
	{
		L"en-US",
		L"Listener startup fail!\nE_INIT_FAILED\nThe data server failed to start, check logs and port number."
	},
	{
		L"lc-LC",
		L"Listwenew shtawtup faiw!\nE_INIT_FAILED\nDe dat sewvew faiwd to shtawt, cshechk logws n powt nuwmbew."
	},
	{
		L"fr-FR",
		L"Erreur du serveur!\nE_INIT_FAILED\nLe serveur n'a pas démarré correctement. Vérifiez les logs et le port."
	}
};

inline std::map<std::wstring, std::wstring> status_not_started_map
{
	{
		L"en-US",
		L"Connection error!\nE_NOT_STARTED\nPress the 'Refresh' button to start the server's listener up."
	},
	{
		L"lc-LC",
		L"Connecshion ewwow!\nE_NOT_STARTED\nPwess de 'Weconnect' wuwwow t stawt de sewvew's listewnew up."
	},
	{
		L"fr-FR",
		L"Erreur de connexion!\nE_NOT_STARTED\nCliquez sur 'Actualiser' pour démarrer le serveur owoTrack."
	}
};

/* Helper functions */

// Get the current use language, e.g. en-US
inline std::wstring GetUserLocale()
{
	wchar_t localeName[LOCALE_NAME_MAX_LENGTH] = { 0 };
	return GetUserDefaultLocaleName(
		localeName, sizeof(localeName) / sizeof(*(localeName))) == 0
		? std::wstring()
		: localeName;
}

// Get the current status string (but localized)
inline std::wstring GetLocalizedStatusWString(
	std::wstring locale, std::map<std::wstring, std::wstring> status_map)
{
	return status_map.contains(locale)
		? status_map[locale]
		: status_map[L"en-US"];
}

// Get the current status string (but localized)
inline std::wstring GetLocalizedStatusWStringAutomatic(
	std::map<std::wstring, std::wstring> status_map)
{
	return status_map.contains(GetUserLocale())
		? status_map[GetUserLocale()]
		: status_map[L"en-US"];
}
