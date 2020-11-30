#pragma once
#include "./imgui/imgui.h"
//#include "./imgui/CustomImguiModifications.h"
#include "imguifilebrowser/imfilebrowser.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "./bakkesmod/wrappers/PluginManagerWrapper.h"
#include "bakkesmod\plugin\pluginwindow.h"
#include "bakkesmod\wrappers\UniqueIDWrapper.h"

struct PluginEntry
{
	std::string dllName;
	std::string pluginName;
	std::string version;
	bool loaded = false;
};

#define BPM_JSON_FILE  "data/bpm.json"
#define BP_ENDPOINT "https://bakkesplugins.com/"

static inline std::string ws2s(const std::wstring& wstr)
{
	/* warning STL4017: std::wbuffer_convert, std::wstring_convert, and the <codecvt> header (containing std::codecvt_mode, std::codecvt_utf8, std::codecvt_utf16, and std::codecvt_utf8_utf16) are deprecated in C++17.
	 * Fix from: https://stackoverflow.com/a/3999597, this fix should be backward compatible. */
	 // using convert_typeX = std::codecvt_utf8<wchar_t>;
	 // std::wstring_convert<convert_typeX, wchar_t> converterX;
	 // return converterX.to_bytes(wstr);
	if (wstr.empty()) return std::string();

	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);

	return strTo;
}

static inline std::wstring s2ws(const std::string& str)
{
	/* warning STL4017: std::wbuffer_convert, std::wstring_convert, and the <codecvt> header (containing std::codecvt_mode, std::codecvt_utf8, std::codecvt_utf16, and std::codecvt_utf8_utf16) are deprecated in C++17.
	 * Fix from: https://stackoverflow.com/a/3999597, this fix should be backward compatible. */
	 //using convert_typeX = std::codecvt_utf8<wchar_t>;
	 //std::wstring_convert<convert_typeX, wchar_t> converterX;
	 //return converterX.from_bytes(str);
	if (str.empty()) return std::wstring();

	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);

	return wstrTo;
}



class PluginManager : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginWindow
{
private:
	std::mutex allPluginsVectorMutex;
	std::map<std::string, PluginEntry> allPlugins;
public:
	virtual void onLoad();
	virtual void onUnload();
	std::filesystem::path GetAbsolutePath(std::filesystem::path relative);

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
	//UniqueIDWrapper myUniqueID;
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