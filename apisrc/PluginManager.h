#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <vector>
#include <string>


class PluginManager
{
public:
										PluginManager();
	virtual								~PluginManager();
	// TODO move contructor???
	const 	std::vector<std::string>	FindForProtocol(const char* signature,const char* version);
			void						SendMessage(const std::string pluginid,BMessage* message);
private:
	class Impl;
			std::unique_ptr<Impl>		fImpl;
};

#endif
