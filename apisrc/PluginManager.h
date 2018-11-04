#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <vector>
#include <string>
#include <memory>
#include <Path.h>
#include <Entry.h>
#include "PluginAPI.h"

class BMessage;
class BEntry;

struct plugin
{
	entry_ref path;
	plugin_descriptor description;
};

// NB Uses the PIMPL idiom to maximise compatibility across API versions
class PluginManager
{
public:
										PluginManager();
										PluginManager(std::vector<std::string> additionalPaths);
	virtual								~PluginManager();
	// TODO move contructor???
	const 	std::vector<std::string>	FindForProtocol(const char* signature,const char* version);
			status_t					SendMessage(const std::string pluginid,BMessage* message);
	const	std::vector<BEntry>			GetAllPluginPaths();
	const	std::vector<plugin>			GetAllPlugins();
	const	std::vector<std::string>	GetProblems();
	
	class Impl;
private:
			std::unique_ptr<Impl>		fImpl;
};

#endif
