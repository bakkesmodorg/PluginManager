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

#define BPM_JSON_FILE  "./bakkesmod/data/bpm.json"
#define BP_ENDPOINT "https://bakkesplugins.com/"



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

	void OnBpmCommand(std::vector<std::string> params);
	void RegisterURIHandler();
	/*
	Installing related functions
	*/
private:
	std::vector<int> plugins_to_install;
public:
	/*
	Extracts the plugin, trusts the developer packaged correctly and doesn't overwrite existing file.
	Returns the contents of plugin.json
	*/
	std::string InstallZip(std::filesystem::path path);

	/*
	Overwrites/creates bpm.json with default data
	*/
	void CreateBPMJson();

	void CheckForPluginUpdates();

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