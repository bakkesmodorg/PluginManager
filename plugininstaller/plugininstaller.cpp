#include "pch.h"
#include <iostream>
#include "SettingsManager.h"
#include <vector>
#include <fstream>
#include <sstream>
#include "easywsclient/easywsclient.hpp"
#include <filesystem>
static inline unsigned int split(const std::string &txt, std::vector<std::string> &strs, char ch);

int main(int argc, char *argv[])
{
	RegisterySettingsManager settings;
	std::wstring registryString_ = settings.GetStringSetting(L"BakkesModPath", RegisterySettingsManager::REGISTRY_DIR_APPPATH);
	if (registryString_.empty())
	{
		std::cout << "Could not find BakkesMod path. Are you sure it's installed?" << std::endl;
		getchar();
		return EXIT_FAILURE;
	}

	std::filesystem::path registryStringP = std::filesystem::path(registryString_).make_preferred();
	if (argc < 2)
	{
		std::cout << "Missing args" << std::endl;
		getchar();
		return EXIT_FAILURE;
	}

	int rcon_port = 9002;
	std::string rcon_password = "password";


	std::filesystem::path config_path = registryStringP / L"cfg/config.cfg";
	std::cout << "Checking config path " << config_path << "\n";
	if (std::filesystem::exists(config_path)) //file exists
	{
		std::ifstream readConfig(config_path);
		std::string line;
		
		while (std::getline(readConfig, line))
		{
			//std::cout << line << "\n";
			std::istringstream iss(line);
			std::string name, val;
			if (!(iss >> name >> val)) { break; } // error

			if (name.compare("rcon_port") == 0)
			{
				if (val.front() == '"')
				{
					val = val.substr(1);
				}
				if (val.back() == '"')
				{
					val.pop_back();
				}
				rcon_port = std::stoi(val);
			}
			if (name.compare("rcon_password") == 0)
			{
				if (val.front() == '"')
				{
					val = val.substr(1);
				}
				if (val.back() == '"')
				{
					val.pop_back();
				}
				rcon_password = val;
			}
		}
	}

	std::string param = argv[1];
	if (param.find("bakkesmod://") == 0)
	{
		param = param.substr(std::string("bakkesmod://").size());
	}
	//std::cout << "Param: " << param << "\n";
	/*else
	{
		param = argv[1];
	}*/
	std::vector<std::string> params;
	//std::cout << "Before split";
	split(param, params, '/');
	//std::cout << "after split";
	std::string command = params.at(0);
	std::cout << param << ", command: " << command << std::endl;
	if (command.compare("install") == 0) 
	{
		if (params.size() < 2)
		{
			std::cout << "Usage: install/id" << std::endl;
			getchar();
			return EXIT_FAILURE;
		}
		std::vector<std::string> ids;
		split(params.at(1), ids, ',');
		for (std::string id : ids)
		{
#ifdef _WIN32
			INT rc;
			WSADATA wsaData;

			rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
			if (rc) {
				std::cout << "WSAStartup Failed." << std::endl;
				getchar();
				return 1;
			}
#endif
			std::filesystem::path allowedFile = registryStringP / "data" / "rcon_commands.cfg";
			std::filesystem::path allowedFileMoved = registryStringP / "data" / "rcon_commands.cfg.backup";

			bool success = false;
			bool auth_success = false;
			std::string ws_url = "ws://localhost:" + std::to_string(rcon_port) + "/";
			std::cout << "Trying to connect to " << ws_url << std::endl;
			try {
				std::unique_ptr<easywsclient::WebSocket> ws(easywsclient::WebSocket::from_url(ws_url));
				if (ws.get() != NULL) {
					ws->send("rcon_password " + rcon_password);
					while (ws->getReadyState() != easywsclient::WebSocket::CLOSED) {
						//easywsclient::WebSocket::pointer wsp = &*ws;
						ws->poll();
						//succ = &success, wsp, id, registryString, 
						ws->dispatch([&](const std::string & message) {
							printf(">>> %s\n", message.c_str());
							if (message == "authyes") {

								auth_success = true;
								std::cout << "Auth success!\n";
							}
							else if (message == "authyes") {
								std::cout << "Plugin install through websockets failed, invalid password!" << std::endl;
								
							}
							else if(message == "ERR:illegal_command")
							{
								success = false;
								std::cout << "Executed illegal command apparently!" << std::endl;
								ws->close();
							}
							
						});

						//std::cout << (int)ws->getReadyState() << std::endl;
						ws->poll(300);
						//std::cout << (int)ws->getReadyState() << std::endl;
						std::cout << "Checking auth success\n";
						if (auth_success && (ws->getReadyState() != easywsclient::WebSocket::CLOSED))
						{
							if (std::filesystem::exists(allowedFile))
							{
								if (std::filesystem::exists(allowedFileMoved))
								{
									std::filesystem::remove(allowedFileMoved);
								}
								std::cout << "Writing to file\n";
								std::filesystem::rename(allowedFile, allowedFileMoved);
								std::ofstream out(allowedFile);
								out << "bpm_install" << std::endl << "rcon_refresh_allowed" << std::endl << "sleep" << std::endl;
							}
							Sleep(1000);
							{
								success = true;
								//std::cout << (int)ws->getReadyState() << std::endl;
								ws->poll();
								//std::cout << (int)ws->getReadyState() << std::endl;
								ws->send("rcon_refresh_allowed;");
								ws->poll(300);
								Sleep(200);
								ws->send("bpm_install " + id + "; ");
								//std::cout << (int)ws->getReadyState() << std::endl;
								ws->poll(300);
								/*ws->dispatch([&](const std::string& message)
									{
										std::cout << "MSG: " << message << "\n";
										if (message == "ERR:illegal_command")
										{
											success = false;
											std::cout << "Executed illegal command apparently!" << std::endl;
										}
									});*/
								ws->poll(100);
								std::cout << "Plugin install called through websockets!" << std::endl;
								ws->close();
							}
						}
						
					}
				}
				else
				{
					std::cout << "Websocket server not running. (AKA RL is not running or BM is not injected)" << std::endl;
				}
			}
			catch (...) {
				std::cout << "Websockets failed for some reason (RL not running?), using newfeatures.apply file instead" << std::endl;
			}
#ifdef _WIN32
			WSACleanup();
#endif
			if (std::filesystem::exists(allowedFileMoved))
			{
				if (std::filesystem::exists(allowedFile))
				{
					std::filesystem::remove(allowedFile);
				}
				std::filesystem::rename(allowedFileMoved, allowedFile);
			}
			std::cout << "Success: " << success << std::endl;
			for (int i = 0; i < 25; ++i)
			{
				std::cout << std::endl;
			}
			if (!success)
			{
				std::ofstream outfile;

				outfile.open(registryStringP / L"/data/newfeatures.apply", std::ios_base::app);
				outfile << "bpm_install " << id << std::endl;
				
				std::cout << "Added line \"bpm_install " << id << "\" to /data/newfeatures.apply" << std::endl;
				std::cout << "Plugin will be installed next time you launch the game!" << std::endl;
			}
			else
			{
				std::cout << "Plugin has been installed. " << std::endl;
			}

		}
	}
	getchar();
	
}



static inline unsigned int split(const std::string &txt, std::vector<std::string> &strs, char ch)
{
	size_t pos = txt.find(ch);
	size_t initialPos = 0;
	strs.clear();

	// Decompose statement
	while (pos != std::string::npos) {
		strs.push_back(txt.substr(initialPos, pos - initialPos));
		initialPos = pos + 1;
		pos = txt.find(ch, initialPos);
	}

	// Add the last one
	strs.push_back(txt.substr(initialPos, min(pos, txt.size()) - initialPos));
	return strs.size();
}
