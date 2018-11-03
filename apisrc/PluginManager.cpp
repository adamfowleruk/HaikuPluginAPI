#include "PluginManager.h"

#include "PluginAPI.h"

#include <cstring>
#include <string>
#include <iterator>
#include <vector>

struct plugin
{
	entry_ref path;
	plugin_descriptor description;
};

class PluginManager::Impl
{
public:
										Impl()
	{
	}
	virtual								~Impl()
	{
	}
	// TODO move contructor???
const
std::vector<std::string>
FindForProtocol(const char* signature,const char* version)
	{
		std::vector<std::string> matches;
		// loop through
		for (auto plugin: plugins)
		{
			// add to vector if name and versions match
			// return vector (move semantics)
			// TODO for now just add everything with same protocol
			for (int prot = 0;prot < plugin.description.count;prot++) 
			{
				protocol protocol = plugin.description.protocolList[prot];
				if (0 == strcmp(protocol.signature, signature))
				{
					matches.emplace_back(plugin);
				}
			}
		}
		return std::move(matches);
	}
	
void
SendMessage(const std::string pluginsig, BMessage* message)
{
	// Call its SendMessage function
	for (auto plugin: plugins)
	{
		if (0 == strcmp(plugin.description.signature, pluginsig))
		{
			MessagePlugin(plugin.path, &this);
			return;
		}
	}
}
	
// internal methods
void
AddDescription(entry_ref path, plugin_descriptor desc)
{
	plugins.emplace_back(struct plugin {path,desc} );
}

const
std::vector<plugin>
GetAllPlugins()
{
	return plugins; // forces copy constructor
}

private:
			std::vector<plugin>			plugins;
};






PluginManager::PluginManager()
	:fImpl(std::make_unique<Impl>())
{
	BPath dataPath;
	directory_which[] searchDirs = {B_SYSTEM_ADDONS_DIRECTORY,B_USER_ADDONS_DIRECTORY,
		B_SYSTEM_NONPACKAGED_ADDONS_DIRECTORY,B_USERS_NONPACKAGED_ADDONS_DIRECTORY};
	for (auto dir: searchDirs)
	{
		find_directory(dir,&dataPath);
		BPath p(dataPath);
		p.append("add-ons"
		p.Append("MessagePlugins");
		BDirectory directory(p.Path());
		BEntry entry;
		while (B_OK == directory.GetNextEntry(&entry,true))
		{
			// now we have a potential library file, so try to load it and introspect it
			entry_ref ref;
			entry->GetRef(&ref);
			DescribePlugin(ref,&this);
		}
	}
}

PluginManager::~PluginManager()
{
	;
}

const std::vector<plugin>
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

// static thread methods

static void
DescribePlugin(entry_ref addonRef,PluginManager::Impl* pluginManager)
{
	LaunchInNewThread("Add-on", B_NORMAL_PRIORITY, &IntrospectThread, addonRef, pluginManager);
}

static int32
IntrospectThread(entry_ref addonRef, PluginManager::Impl* pluginManager)
{
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
				
				pluginManager->AddDescription(description);

				unload_add_on(addonImage);
				return B_OK;
			} else
				PRINT(("couldn't find process_refs\n"));

			unload_add_on(addonImage);
		} else
			result = addonImage;
	}

	BString buffer(B_TRANSLATE("Error %error loading Plugin %name."));
	buffer.ReplaceFirst("%error", strerror(result));
	buffer.ReplaceFirst("%name", addonRef.name);
	
	// TODO log the above somewhere useful in the plugin manager for later inspection
	//     (Otherwise the user would be like "Hey, why isn't that plugin listed?"

	return result;
}
