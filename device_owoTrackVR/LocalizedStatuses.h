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
	},
	{
		L"ru-RU",
		L"Успешно!\nS_OK\nВсе работает!"
	},
	{
		L"de-DE",
		L"Erfolg!\nS_OK\nAlles ist gut!"
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
	},
	{
		L"ru-RU",
		L"Ошибка подключения!\nE_CONNECTION_DEAD\nУбедитесь, что приложение owoTrackVR работает и подключено."
	},
	{
		L"de-DE",
		L"Verbindungsfehler!\nE_CONNECTION_DEAD\nÜberprüfe ob die owoTrackVR-App läuft, funktioniert und verbunden ist."
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
	},
	{
		L"ru-RU",
		L"Ошибка подключения!\nE_NO_DATA\nУбедитесь, что приложение owoTrackVR работает и подключено."
	},
	{
		L"de-DE",
		L"Verbindungsfehler!\nE_NO_DATA\nÜberprüfe ob die owoTrackVR-App läuft, funktioniert und verbunden ist."
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
	},
	{
		L"ru-RU",
		L"Ошибка запуска сервера!\nE_INIT_FAILED\nНе удалось запустить сервер, проверьте журнал событий и номер порта."
	},
	{
		L"de-DE",
		L"Serverfehler!\nE_INIT_FAILED\nDer Datenserver konnte nicht gestartet werden. Überprüfe die Logs und die Portnummer."
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
	},
	{
		L"ru-RU",
		L"Ошибка подключения!\nE_NOT_STARTED\nНажмите кнопку \"Обновить\" для запуска сервера."
	},
	{
		L"de-DE",
		L"Verbindungsfehler!\nE_NOT_STARTED\nKlicke auf „Aktualisieren“ um den Listener des Servers zu starten."
	}
};

/* Helper functions */

// Get the current use language, e.g. en-US
inline std::wstring GetUserLocale()
{
	wchar_t localeName[LOCALE_NAME_MAX_LENGTH] = {0};
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
