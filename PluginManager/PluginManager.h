#pragma once
#include "./imgui/imgui.h"
#include "./imgui/CustomImguiModifications.h"
#include "imguifilebrowser/imfilebrowser.h"
#pragma comment( lib, "bakkesmod.lib" )

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "./bakkesmod/wrappers/PluginManagerWrapper.h"
#include "bakkesmod\plugin\pluginwindow.h"

struct PluginEntry
{
	std::string dllName;
	std::string pluginName;
	std::string version;
	bool loaded = false;
};

class PluginManager : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginWindow
{
private:
	std::mutex allPluginsVectorMutex;
	std::map<std::string, PluginEntry> allPlugins;
public:
	virtual void onLoad();
	virtual void onUnload();

	/*
	Called whenever a "plugin load/unload/reload" command is executed
	*/
	void OnPluginListUpdated(std::vector<std::string> params);


	/*
	Installing related functions
	*/
private:
public:
	/*
	Extracts the plugin, trusts the developer packaged correctly and doesn't overwrite existing file.
	Returns the contents of plugin.json
	*/
	std::string InstallZip(std::filesystem::path path);

	/*
	GUI stuff, implementation is in PluginManagerGUI.cpp
	*/
private:
	bool isWindowOpen = false;
	bool isMinimized = false;
	std::string menuTitle = "Plugin manager";
	ImGui::FileBrowser fileDialog = ImGui::FileBrowser(ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_SingleClickDir | ImGuiFileBrowserFlags_SortIgnoreCase);
public:
	virtual void Render();
	virtual std::string GetMenuName();
	virtual std::string GetMenuTitle();
	virtual void SetImGuiContext(uintptr_t ctx);
	virtual bool ShouldBlockInput();
	virtual bool IsActiveOverlay();
	virtual void OnOpen();
	virtual void OnClose();
};