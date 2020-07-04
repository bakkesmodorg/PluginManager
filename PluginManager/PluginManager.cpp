#include "PluginManager.h"
#include <fstream>
#include "utils/io.h"
//Need to undef these for zip_file
#undef min
#undef max
#include "zip_file.hpp"
#ifdef _WIN32
#include <windows.h>
#include <Shlobj_core.h>
#include <sstream>
#include <memory>
#include <curl/curl.h>
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "libcurl_a.lib")
#pragma comment(lib, "curlpp.lib")
//#include "curlpp/curlpp.cpp"
#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"
#include "curlpp/Exception.hpp"

#include <thread>
#include <future>
#include "SettingsManager.h"
#endif

#include "nlohmann/json.hpp"
using json = nlohmann::json;
BAKKESMOD_PLUGIN(PluginManager, "Plugin Manager", "2", 0)

std::string random_string(size_t length)
{
	auto randchar = []() -> char
	{
		const char charset[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";
		const size_t max_index = (sizeof(charset) - 1);
		return charset[rand() % max_index];
	};
	std::string str(length, 0);
	std::generate_n(str.begin(), length, randchar);
	return str;
}

size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void *f)
{
	FILE *file = (FILE *)f;
	return fwrite(ptr, size, nmemb, file);
};
unsigned long long steamID = 0;

std::future<std::string> downloadZip(std::string const& url) {
	return std::async(std::launch::async, [](std::string const& url) mutable {
		try
		{
			curlpp::Cleanup cleaner;
			curlpp::Easy request;

			/// Set the writer callback to enable cURL to write result in a memory area
			curlpp::options::WriteFunctionCurlFunction
				myFunction(WriteCallback);
			char tmp_path[MAX_PATH];
			GetTempPath(MAX_PATH, tmp_path);
			const std::string download_folder = std::string(tmp_path) + "/bakkesmod/";
			const std::string filename = download_folder + random_string(12) + ".zip";
			if (CreateDirectory(download_folder.c_str(), NULL) ||
				ERROR_ALREADY_EXISTS == GetLastError())
			{
				

				FILE *file = stdout;

				file = fopen(filename.c_str(), "wb");
				if (file == NULL)
				{
					fprintf(stderr, "%s/n", strerror(errno));
					return std::string("ERROR: " + std::string(strerror(errno)));
				}
				

				curlpp::OptionTrait<void *, CURLOPT_WRITEDATA>
					myData(file);


				std::list<std::string> header;
				header.push_back("User-Agent: BPM;2;" + std::to_string(BAKKESMOD_PLUGIN_API_VERSION) + ";steam;" + std::to_string(steamID) + ";");
				request.setOpt(new curlpp::options::HttpHeader(header));

				request.setOpt(myFunction);
				request.setOpt(myData);

				
				request.setOpt(new curlpp::options::Url(url));
				//request.setOpt(new curlpp::options::Verbose(true));
				request.perform();
				fclose(file);
				return filename;
			}
			else
			{
				return std::string("ERROR: Dont have access to " + download_folder);
			}
		}

		catch (curlpp::LogicError & e)
		{
			std::cout << e.what() << std::endl;
			return std::string("ERROR: " + std::string(e.what()));
		}

		catch (curlpp::RuntimeError & e)
		{
			std::cout << e.what() << std::endl;
			return std::string("ERROR: " + std::string(e.what()));
		}
		catch (...) {}
		return std::string("ERROR: idk");
	}, url);
}


std::future<std::string> checkupdate(std::string const& url, std::string const& body) {
	return std::async(std::launch::async,
		[](std::string const& url, std::string const& body) mutable {
		try
		{

			std::list<std::string> header;
			header.push_back("Content-Type: application/json");
			header.push_back("User-Agent: BPM;2;" + std::to_string(BAKKESMOD_PLUGIN_API_VERSION) + ";steam;" + std::to_string(steamID) + ";");
			curlpp::Cleanup clean;
			curlpp::Easy r;
			r.setOpt(new curlpp::options::Url(url));
			r.setOpt(new curlpp::options::HttpHeader(header));
			if (body.size() > 0) {
				r.setOpt(new curlpp::options::PostFields(body));
				r.setOpt(new curlpp::options::PostFieldSize(body.length()));

			}
			std::ostringstream response;
			r.setOpt(new curlpp::options::WriteStream(&response));

			r.perform();

			return std::string(response.str());
		}

		catch (curlpp::LogicError & e)
		{
			return std::string(e.what());
		}

		catch (curlpp::RuntimeError & e)
		{
			return std::string(e.what());// << std::endl;
		}
		catch (...)
		{

		}
		return std::string("Err");
	}, url, body);
}

void PluginManager::onLoad()
{
	srand(time(0));
	RegisterURIHandler();
	cvarManager->registerNotifier("plugin", std::bind(&PluginManager::OnPluginListUpdated, this, std::placeholders::_1), "plugin command hook for plugin manager", PERMISSION_ALL);
	cvarManager->registerNotifier("bpm_install", std::bind(&PluginManager::OnBpmCommand, this, std::placeholders::_1), "Install BakkesMod plugin by id", PERMISSION_ALL);
	//93
	OnPluginListUpdated(std::vector<std::string>());
	fileDialog.SetAcceptableFileTypes("zip");
	steamID = gameWrapper->GetSteamID();
	cvarManager->log("Got steam id " + std::to_string(steamID));
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

	
	CheckForPluginUpdates();
	

	
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

void PluginManager::OnBpmCommand(std::vector<std::string> params)
{
	std::string command = params.at(0);
	if (command.compare("bpm_install") == 0)
	{
		if (params.size() < 2)
		{
			cvarManager->log("Usage: " + command + " id");
			return;
		}
		for (int i = 1; i < params.size(); i++) {
			try {
				int parse_id = std::stoi(params.at(i));
				plugins_to_install.push_back(parse_id);
			}
			catch (...) {}
		}
		CheckForPluginUpdates();
	}
}

void PluginManager::RegisterURIHandler()
{
	RegisterySettingsManager settings;

	std::wstring registryString = settings.GetStringSetting(L"BakkesModPath", RegisterySettingsManager::REGISTRY_DIR_APPPATH);
	registryString += L"plugininstaller.exe";
	settings.SaveSetting(L"", L"URL:bakkesmod protocol", L"Software\\Classes\\bakkesmod", HKEY_CURRENT_USER);
	settings.SaveSetting(L"URL Protocol", L"bakkesmod",  L"Software\\Classes\\bakkesmod", HKEY_CURRENT_USER);
	settings.SaveSetting(L"", (registryString + L",1").c_str(), L"Software\\Classes\\bakkesmod\\DefaultIcon", HKEY_CURRENT_USER);
	
	settings.SaveSetting(L"", (L"\"" + registryString + L"\" \"%1\"") , L"Software\\Classes\\bakkesmod\\shell\\open\\command", HKEY_CURRENT_USER);

}

bool has_suffix(const std::string &str, const std::string &suffix)
{
	return str.size() >= suffix.size() &&
		str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string PluginManager::InstallZip(std::filesystem::path path)
{
	std::basic_ifstream<BYTE> fileStr(path, std::ios::binary);
	std::string jsonResult = "";
	auto data = std::vector<BYTE>((std::istreambuf_iterator<BYTE>(fileStr)),
		std::istreambuf_iterator<BYTE>());

	bool pluginJsonFound = false;
	std::string dllName = "";
	miniz_cpp::zip_file file(data);
	std::string extractDir = "bakkesmod/";
	for (auto &member : file.infolist())
	{
		std::string fullPath = extractDir + member.filename;
		if (member.filename.compare("plugin.json") == 0)
		{
			pluginJsonFound = true;
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
				dllName = member.filename.substr(0, member.filename.rfind('.'));
				dllName = dllName.substr(dllName.rfind('/') + 1);
				cvarManager->executeCommand("plugin unload " + dllName); //Plugin might already be installed and user is installing new version
			}
			file.extract(member, extractDir);
		}
	}
	if (!pluginJsonFound)
	{
		if (!has_suffix(dllName, ".dll"))
		{
			dllName = dllName + ".dll";
		}
		return "{ \"dll\": \"" + dllName + "\" }";
	}
	return jsonResult;
}

void PluginManager::CreateBPMJson()
{
	json bpmdata;

	bpmdata["plugins"] = json::object();

	std::ofstream file(BPM_JSON_FILE);
	file << bpmdata;
}

void PluginManager::CheckForPluginUpdates()
{
	if (!file_exists(BPM_JSON_FILE))
	{
		this->CreateBPMJson();
	}
	json bpmj;
	try {
		std::ifstream ifs(BPM_JSON_FILE);
		bpmj = json::parse(ifs);
	}
	catch (...) {
		bpmj["plugins"] = json::object();
		cvarManager->log("bpm file corrupt, recreating");
	}
	//if cannot parse json, recreate default file
	if (bpmj.is_discarded())
	{
		this->CreateBPMJson();
		std::ifstream ifs(BPM_JSON_FILE);
		bpmj = json::parse(ifs);
		if (bpmj.is_discarded())
		{
			cvarManager->log("Somethings very wrong with json loading");
		}
	}
	if (bpmj.find("plugins") != bpmj.end() && bpmj["plugins"].is_object())
	{

		std::string plugin_list;
		std::map<int, int> plugin_versions;
		std::string separator = "";
		for (auto plugin = bpmj["plugins"].begin(); plugin != bpmj["plugins"].end(); plugin++)
		{
			int plugin_id = std::stoi(plugin.key());
			int version = plugin.value()["version"];
			plugin_versions[plugin_id] = version;
			plugin_list += separator + std::to_string(plugin_id);
			separator = ",";
		}

		/*
		New plugin to install
		*/
		for (int to_install : plugins_to_install)
		{
			int plugin_id = to_install;
			int version = -1;
			plugin_versions[plugin_id] = version;
			plugin_list += separator + std::to_string(plugin_id);
			separator = ",";
		}

		
		std::thread([cv = &cvarManager, gw = &gameWrapper, plugin_list, plugin_versions, this, bpmj]() {
			std::shared_ptr<std::future<std::string>> fut = std::make_shared< std::future<std::string>>(checkupdate("https://bakkesplugins.com/api/v1/plugins/get/short?pluginids=" + plugin_list, ""));
			while (fut->wait_for(std::chrono::seconds(0)) == std::future_status::ready);
			std::string response = fut->get();
			(*gw)->Execute([cv, fut, plugin_versions, bpmj, response, this](GameWrapper* game) mutable {
				
				(*cv)->log(response);
				std::string resp = response;
				nlohmann::json response_json;
				try {
					response_json = json::parse(resp);
				}
				catch (json::parse_error &e)
				{
					(*cv)->log("JSON parse error: " + std::string(e.what()));
					return;
				}
				catch (...)
				{
					(*cv)->log("Unexpected JSON parse failure!");
					return;
				}
				if (response_json.is_discarded())
				{
					(*cv)->log("JSON is not parseable!");
					return;
				}
				if (response_json.find("success") != response_json.end() && response_json["success"] == true)
				{
					if (response_json.find("response") != response_json.end() &&
						response_json["response"].find("plugins") != response_json["response"].end())
					{
						auto plugin_list = response_json["response"]["plugins"];
						for (auto it = plugin_list.begin(); it != plugin_list.end(); it++)
						{
							const int plugin_id_key = std::stoi(it.key());
							int plugin_version = it.value()["version"];
							auto found = plugin_versions.find(plugin_id_key);
							if (found != plugin_versions.end())
							{
								if (plugin_version > found->second)
								{
									//Download and install new plugin
									std::future<std::string> zip_dl;
									{
										zip_dl = downloadZip(std::string((string)it.value()["download_url"]));
									}
									std::string file_location = zip_dl.get();
									if (file_location.find("ERROR") != 0)
									{
										try {
											cvarManager->executeCommand("writeconfig;");
											cvarManager->log("Installing zip " + file_location);
											std::string installResult = InstallZip(file_location);
											cvarManager->log("Install done, deleting zip");
											int result = remove(file_location.c_str());
											cvarManager->log("Install zip delete result = " + std::to_string(result));

											
											std::string err = "";
											auto jsonVal = json::parse(installResult);
											if (err.size() == 0)
											{
												if (jsonVal.is_object())
												{
													if (jsonVal.find("dll") != jsonVal.end())
													{
														std::string dllName = jsonVal["dll"];
														if (dllName.substr(dllName.size() - std::string(".dll").size()).compare(".dll") == 0)
														{
															dllName = dllName.substr(0, dllName.rfind('.'));
														}
														cvarManager->executeCommand("sleep 1; plugin load " + dllName + "; writeplugins;cl_settings_refreshplugins;");
													}
												}
											}
											else
											{
												cvarManager->log("Error " + err);
											}
											cvarManager->executeCommand("sleep 1;exec config;");
											plugin_versions[plugin_id_key] = plugin_version;
										}
										catch (const std::exception& ex) {
											// ...
											cvarManager->log("Ex: " + std::string(ex.what()));
										}
										catch (const std::string& ex) {
											cvarManager->log("Exs: " + ex);
										}
										catch (...) {
											cvarManager->log("Catch all exception");
										}

									}
									else
									{
										cvarManager->log("Got error when downloading plugin: " + file_location);
									}
								}
								//plugin_versions.erase(plugin_id_key);
							}
						}

						bpmj["plugins"] = {};
						for (auto it : plugin_versions)
						{
							bpmj["plugins"][std::to_string(it.first)]["version"] = it.second;
						}
						std::ofstream file(BPM_JSON_FILE);
						file << bpmj;
					}

					if (response_json.find("errors") != response_json.end() &&
						response_json["errors"].find("pluginids") != response_json["errors"].end())
					{
						//remove old plugins lata
					}
				}
				else
				{
					(*cv)->log("Response returned success = false");
				}
			});
			OnPluginListUpdated(std::vector<std::string>());
		}).detach();
	}


	else
	{
		cvarManager->log("Could not find plugins list");
	}
}
