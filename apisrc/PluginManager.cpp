#include "PluginManager.h"

#include <Path.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Message.h>
#include <String.h>

#include "PluginAPI.h"

#include <private/shared/Thread.h> // private/shared/Thread.h

#include <cstring>
#include <string>
#include <iterator>
#include <vector>
#include <iostream>





class PluginManager::Impl
{
public:
										Impl();
										Impl(std::vector<std::string> morePaths);
	virtual								~Impl();
	
	const 	std::vector<std::string>	FindForProtocol(const char* signature,const char* version);
			void						SendMessage(const std::string pluginsig, BMessage* message);
			void						AddDescription(entry_ref path, plugin_descriptor* desc);
	const	std::vector<BEntry>			GetAllPluginPaths();
	const	std::vector<std::shared_ptr<plugin>>			GetAllPlugins();
	const	std::vector<std::string>	GetProblems();
			void						AddProblem(std::string problem);
private:
			std::vector<std::shared_ptr<plugin>>			plugins;
			std::vector<std::string>	problems;
			std::vector<BEntry> 		fPluginPaths;
};



// static thread methods

static int32
IntrospectThread(entry_ref addonRef, PluginManager::Impl* pluginManager)
{
	std::cout << "Introspect Thread" << std::endl;
	BEntry entry(&addonRef);
	BPath path;
	status_t result = entry.InitCheck();
	if (result == B_OK)
		result = entry.GetPath(&path);

	if (result == B_OK) {
		image_id addonImage = load_add_on(path.Path());
		if (addonImage >= 0) {
			plugin_descriptor (*describe_plugin)(void*);
			result = get_image_symbol(addonImage, "describe_plugin", 0,
				(void**)&describe_plugin);

			if (result >= 0) {
				// call add-on code
				plugin_descriptor description = 
					(*describe_plugin)(NULL);
				PRINT(("plugin signature: %s\n",description.signature));
				std::cout << "plugin signature: " << description.signature << std::endl;
				pluginManager->AddDescription(addonRef,&description);

				unload_add_on(addonImage);
				return B_OK;
			} else
				PRINT(("couldn't find describe_plugin\n"));

			unload_add_on(addonImage);
		} else
			result = addonImage;
	}


	BString buffer("Error %error loading Plugin %name.");
	buffer.ReplaceFirst("%error", strerror(result));
	buffer.ReplaceFirst("%name", addonRef.name);
	
	// Log the above somewhere useful in the plugin manager for later inspection
	//     (Otherwise the user would be like "Hey, why isn't that plugin listed?"
	pluginManager->AddProblem(buffer.String());
	
	return result;
}


static void
DescribePlugin(entry_ref addonRef,PluginManager::Impl* pluginManager)
{
	std::cout << "DescribePlugin" << std::endl;
	LaunchInNewThread("Add-on", B_NORMAL_PRIORITY, &IntrospectThread, addonRef, pluginManager);
}


static int32
MessagePlugin(entry_ref addonRef, PluginManager::Impl* pluginManager)
{
	// TODO implement
	return B_OK;
}

PluginManager::Impl::Impl(std::vector<std::string> additionalPaths)
	:fPluginPaths()
{
	std::cout << "Impl(paths)" << std::endl;
	for (auto ap: additionalPaths)
	{
		std::cout << "Path: " << ap << std::endl;
		BEntry entry(ap.c_str());
		// check if its a file or folder, and process accordingly
		// TODO folder later, single files for now
		if (entry.IsFile()) {
			std::cout << "Got file" << std::endl;
			fPluginPaths.emplace_back(entry);
		}
	}
	
	for (auto entry: fPluginPaths)
	{
		std::cout << "Processing path" << std::endl;
		entry_ref ref;
		entry.GetRef(&ref);
		DescribePlugin(ref,this);
	}
}

PluginManager::Impl::Impl()
	:fPluginPaths()
{	
	std::cout << "Impl()" << std::endl;
	// First add standard system paths
	BPath dataPath;
	directory_which searchDirs[] = {B_SYSTEM_ADDONS_DIRECTORY,B_USER_ADDONS_DIRECTORY,
		B_SYSTEM_NONPACKAGED_ADDONS_DIRECTORY,B_USER_NONPACKAGED_ADDONS_DIRECTORY};
	for (directory_which dir: searchDirs)
	{
		find_directory(dir,&dataPath);
		BPath p(dataPath);
		p.Append("MessagePlugins");
		BDirectory directory(p.Path());
		BEntry entry;
		while (B_OK == directory.GetNextEntry(&entry,true))
		{
			// now we have a potential library file, so try to load it and introspect it
			
			fPluginPaths.emplace_back(entry);
		}
	}
	
	// Then add specific paths
	
	for (auto entry: fPluginPaths)
	{
		entry_ref ref;
		entry.GetRef(&ref);
		DescribePlugin(ref,this);
	}
}

	
PluginManager::Impl::~Impl()
{
}

	// TODO move constructor???

const
std::vector<std::string>
PluginManager::Impl::FindForProtocol(const char* signature,const char* version)
	{
		std::vector<std::string> matches;
		// loop through
		for (auto plugin: plugins)
		{
			// add to vector if name and versions match
			// return vector (move semantics)
			// TODO for now just add everything with same protocol
			for (int prot = 0;prot < plugin->description->count;prot++) 
			{
				const protocol* protocol = &(plugin->description->protocolList[prot]);
				if (0 == strcmp(protocol->signature, signature))
				{
					matches.emplace_back(plugin->description->signature);
				}
			}
		}
		return std::move(matches);
	}
	
void
PluginManager::Impl::SendMessage(const std::string pluginsig, BMessage* message)
{
	// Call its SendMessage function
	for (auto plugin: plugins)
	{
		if (0 == strcmp(plugin->description->signature, pluginsig.c_str()))
		{
			MessagePlugin(plugin->path, this);
			return;
		}
	}
}
	
// internal methods
void
PluginManager::Impl::AddDescription(entry_ref path, plugin_descriptor* desc)
{
	std::string sig(desc->signature);
	std::string summary(desc->summary);
	std::string version(desc->version);
	/*
	char* sig = nullptr;
	char* summary = nullptr;
	char* version = nullptr;
	strcpy(sig,desc->signature);
	strcpy(summary,desc->summary);
	strcpy(version,desc->version);
	*/
	
	int size = desc->count;
	
	struct protocol protocolList[size];
	for (int i = 0;i < desc->count;i++) 
	{
		char* psig = nullptr;
		char* pver = nullptr;
		char* pminver = nullptr;
		char* pmaxver = nullptr;
		strcpy(psig,protocolList[i].signature);
		strcpy(pver,protocolList[i].version);
		strcpy(pminver,protocolList[i].minVersion);
		strcpy(pmaxver,protocolList[i].maxVersion);
	}
	
	struct plugin_descriptor* mypd = new plugin_descriptor(sig.c_str(),summary.c_str(),version.c_str(),desc->count,protocolList);
	
	std::cout << "summary: " << mypd->summary << std::endl;
/*	
	const int* count = new int(desc->count);
	std::shared_ptr<struct plugin_descriptor> d = std::make_shared<struct plugin_descriptor>(
		sig.c_str(),summary.c_str(),version.c_str(),count,protocolList);
	std::shared_ptr<struct plugin> p = std::make_shared<struct plugin>(std::move(path), std::move(d));
	plugins.emplace_back(p);
	*/
}

const
std::vector<BEntry>
PluginManager::Impl::GetAllPluginPaths()
{
	return fPluginPaths;
}

const
std::vector<std::shared_ptr<plugin>>
PluginManager::Impl::GetAllPlugins()
{
	return plugins; // forces copy constructor
}

const
std::vector<std::string>
PluginManager::Impl::GetProblems()
{
	return problems;
}

void
PluginManager::Impl::AddProblem(std::string problem)
{
	problems.emplace_back(problem);
}





PluginManager::PluginManager(std::vector<std::string> additionalPaths)
	:fImpl(std::make_unique<PluginManager::Impl>(additionalPaths))
{
}
PluginManager::PluginManager()
	:fImpl(std::make_unique<PluginManager::Impl>())
{
}

PluginManager::~PluginManager()
{
	;
}

const std::vector<BEntry>
PluginManager::GetAllPluginPaths()
{
	return fImpl->GetAllPluginPaths();
}

const std::vector<std::shared_ptr<plugin>>
PluginManager::GetAllPlugins()
{
	return fImpl->GetAllPlugins();
}

const std::vector<std::string>
PluginManager::FindForProtocol(const char* signature,const char* version)
{
	return fImpl->FindForProtocol(signature,version);
}

void
PluginManager::SendMessage(const std::string pluginid,BMessage* message)
{
	fImpl->SendMessage(pluginid,message);
}

const std::vector<std::string>
PluginManager::GetProblems()
{
	return fImpl->GetProblems();
}




