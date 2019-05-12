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
#endif

BAKKESMOD_PLUGIN(PluginManager, "Plugin Manager", "0.1", 0)

void PluginManager::onLoad()
{
	cvarManager->registerNotifier("plugin", std::bind(&PluginManager::OnPluginListUpdated, this, std::placeholders::_1), "plugin command hook for plugin manager", PERMISSION_ALL);
	OnPluginListUpdated(std::vector<std::string>());

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
