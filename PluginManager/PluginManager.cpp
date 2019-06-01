#include "PluginManager.h"
#include <fstream>

//Need to undef these for zip_file
#undef min
#undef max
#include "zip_file.hpp"
#ifdef _WIN32
#include <windows.h>
#include <Shlobj_core.h>
#include <sstream>

#include <curl/curl.h>
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "libcurl_a.lib")
#endif

BAKKESMOD_PLUGIN(PluginManager, "Plugin Manager", "0.1", 0)

size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

void PluginManager::onLoad()
{
	cvarManager->registerNotifier("plugin", std::bind(&PluginManager::OnPluginListUpdated, this, std::placeholders::_1), "plugin command hook for plugin manager", PERMISSION_ALL);
	OnPluginListUpdated(std::vector<std::string>());
	fileDialog.SetAcceptableFileTypes("zip");
#ifdef _WIN32
	

	WCHAR* path[MAX_PATH] = { 0 };
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, path);

	if (FAILED(hr)) 
	{
		hr = SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, path);
	}
	if (!FAILED(hr))
	{
		std::wstring test = std::wstring(*path);
		fileDialog.SetPwd(test);
	}

	CURL *curl;
	CURLcode res;

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "https://dev.bakkesplugins.com/api/v1/plugins/get/short?pluginids=69,87");
		std::string readBuffer;
#ifdef SKIP_PEER_VERIFICATION
		/*
		 * If you want to connect to a site who isn't using a certificate that is
		 * signed by one of the certs in the CA bundle you have, you can skip the
		 * verification of the server's certificate. This makes the connection
		 * A LOT LESS SECURE.
		 *
		 * If you have a CA cert for the server stored someplace else than in the
		 * default bundle, then the CURLOPT_CAPATH option might come handy for
		 * you.
		 */
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
		/*
		 * If the site you're connecting to uses a different host name that what
		 * they have mentioned in their server certificate's commonName (or
		 * subjectAltName) fields, libcurl will refuse to connect. You can skip
		 * this check, but this will make the connection less secure.
		 */
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
		cvarManager->log(readBuffer);
		/* always cleanup */
		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();
#endif

}

void PluginManager::onUnload()
{
}

void PluginManager::OnPluginListUpdated(std::vector<std::string> params)
{
	allPluginsVectorMutex.lock();
	allPlugins.clear();
	auto loadedPlugins = gameWrapper->GetPluginManager().GetLoadedPlugins();
	for (auto it = loadedPlugins->begin(); it != loadedPlugins->end(); it++)
	{
		std::string dllName = std::string((*it)->_filename) + ".dll";
		PluginEntry pluginEntry = {
			dllName,
			std::string((((*it)->_details))->pluginName),
			std::string((((*it)->_details))->pluginVersion),
			true
		};

		std::string dllNameLower = dllName;
		std::transform(dllNameLower.begin(), dllNameLower.end(), dllNameLower.begin(), ::tolower);
		allPlugins[dllNameLower] = pluginEntry;
	}

#ifdef _WIN32
	std::string pattern("./bakkesmod/plugins/*.dll");
	WIN32_FIND_DATA data;
	HANDLE hFind;
	if ((hFind = FindFirstFile(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE) {
		do {
			std::string dllName = std::string(data.cFileName);
			std::string dllNameLower = dllName;
			std::transform(dllNameLower.begin(), dllNameLower.end(), dllNameLower.begin(), ::tolower);
			if (allPlugins.find(dllNameLower) == allPlugins.end()) //plugin is not loaded
			{
				allPlugins[dllNameLower] = {
					dllName, dllName.substr(0, dllName.rfind('.')), "-", false
				};
			}
		} while (FindNextFile(hFind, &data) != 0);
		FindClose(hFind);
	}
#endif

	allPluginsVectorMutex.unlock();
}

std::string PluginManager::InstallZip(std::filesystem::path path)
{
	std::basic_ifstream<BYTE> fileStr(path, std::ios::binary);
	std::string jsonResult = "";
	auto data = std::vector<BYTE>((std::istreambuf_iterator<BYTE>(fileStr)),
		std::istreambuf_iterator<BYTE>());

	miniz_cpp::zip_file file(data);
	std::string extractDir = "bakkesmod/";
	for (auto &member : file.infolist())
	{
		std::string fullPath = extractDir + member.filename;
		if (member.filename.compare("plugin.json") == 0)
		{
			std::string tempJson = extractDir + "data/";
			{
				file.extract(member, tempJson);
			}
			{
				std::ifstream jsonInput(tempJson + "plugin.json");
				
				jsonInput.seekg(0, std::ios::end);
				jsonResult.reserve(jsonInput.tellg());
				jsonInput.seekg(0, std::ios::beg);

				jsonResult.assign((std::istreambuf_iterator<char>(jsonInput)),
					std::istreambuf_iterator<char>());
			}
			std::filesystem::remove(tempJson + "plugin.json");
		}
		else if (member.filename.find(".") == std::string::npos) //It's a folder
		{
				std::filesystem::create_directory(fullPath);
		}
		else
		{
			if (member.filename.substr(member.filename.size() - std::string(".dll").size()).compare(".dll") == 0)
			{
				std::string dllName = member.filename.substr(0, member.filename.rfind('.'));
				dllName = dllName.substr(dllName.rfind('/') + 1);
				cvarManager->executeCommand("plugin unload " + dllName); //Plugin might already be installed and user is installing new version
			}
			file.extract(member, extractDir);
		}
	}
	return jsonResult;
}
