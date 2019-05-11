#include "PluginManager.h"
#ifdef _WIN32
#include <windows.h>
#endif

BAKKESMOD_PLUGIN(PluginManager, "Plugin Manager", "0.1", 0)

void PluginManager::onLoad()
{
	cvarManager->registerNotifier("plugin", std::bind(&PluginManager::OnPluginListUpdated, this, std::placeholders::_1), "plugin command hook for plugin manager", PERMISSION_ALL);
	OnPluginListUpdated(std::vector<std::string>());
	
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
			cvarManager->log(dllName);
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
