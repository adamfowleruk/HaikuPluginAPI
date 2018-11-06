#include "PluginManager.h"

#include <Path.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Message.h>
#include <String.h>
#include <Handler.h>

#include "PluginAPI.h"

#include <private/shared/Thread.h> // private/shared/Thread.h

#include <cstring>
#include <string>
#include <iterator>
#include <vector>
#include <iostream>
#include <functional>





class PluginManager::Impl
{
public:
										Impl();
										Impl(std::vector<std::string> morePaths);
	virtual								~Impl();
	
	const 	std::vector<std::string>	FindForProtocol(const char* signature,const char* version);
			void						SendMessage(const std::string pluginsig, const char* protocolSig, BMessage* message);
			void						AddDescription(entry_ref path, plugin_descriptor* desc);
	const	std::vector<BEntry>			GetAllPluginPaths();
	const	std::vector<plugin>			GetAllPlugins();
	const	std::vector<std::string>	GetProblems();
			void						AddProblem(std::string problem);
            void                        SetReplyHandler(BHandler* replyHandler);
            void                        ReceiveReply(const char* flattenedMessage);
private:
			std::vector<plugin>			plugins;
			std::vector<std::string>	problems;
			std::vector<BEntry> 		fPluginPaths;
            BHandler*                   fReplyHandler;
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
				PRINT(("IntrospectThread: plugin signature: %s\n",
                    description.signature));
				std::cout << "IntrospectThread: plugin signature: " 
                    << description.signature << std::endl;
				
				// print out first protocol too
				std::cout << "IntrospectThread:   first protocol sig: " 
                    << description.protocolList[0].signature << std::endl;
				
				pluginManager->AddDescription(addonRef,&description);
				

				unload_add_on(addonImage);
				return B_OK;
			} else
				PRINT(("IntrospectThread: couldn't find describe_plugin\n"));

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

// TODO FIXME: horrible hack for now
static PluginManager::Impl* replyToManager = nullptr;

static void
handle_reply(const char* flattenedMessage)
{
    replyToManager->ReceiveReply(flattenedMessage);
}

static int32
SendThread(entry_ref addonRef, PluginManager::Impl* pluginManager,const char* protocolSig, const char* flattenedMessage)
{
	std::cout << "Send Thread" << std::endl;
    if (nullptr == replyToManager) 
    {
        replyToManager = pluginManager;
    }
    
    BEntry entry(&addonRef);
	BPath path;
	status_t result = entry.InitCheck();
	if (result == B_OK)
		result = entry.GetPath(&path);

	if (result == B_OK) {
		image_id addonImage = load_add_on(path.Path());
		if (addonImage >= 0) {
			void (*receive_message_raw)(const char*, const char*, 
                void (const char*));

            void (*replyTo)(const char*) = &handle_reply;
            //void (*replyTo)(const char*) = &(PluginManager::Impl::ReceiveReply);
            
            /*
            void (*replyTo)(const char*) = [pluginManager] -> (
                const char* flattenedMessage) {
                    pluginManager->ReceiveReply(flattenedMessage);
                };
            */
            /*
            std::function<void(const char*)> replyTo(PluginManager::Impl* pm) {
                return [&](const char* fm) {
                    pm->ReceiveReply(fm);
                };
            };
            */
            
			result = get_image_symbol(addonImage, "receive_message_raw", 
                3,
				(void**)&receive_message_raw);

			if (result >= 0) {
				//std::cout << "SendThread: message what: " << message->what << std::endl;
				// call add-on code
					(*receive_message_raw)(protocolSig, flattenedMessage, 
                        replyTo);
					
				std::cout << "SendThread: Message Sent" << std::endl;

				//unload_add_on(addonImage); // TODO ensure it is safe to remove already (and each time?)
				return B_OK;
			} else
				PRINT(("SendThread: couldn't find describe_plugin\n"));

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
MessagePlugin(entry_ref addonRef, PluginManager::Impl* pluginManager, const char* protocolSig, BMessage* message)
{
	std::cout << "MessagePlugin: what: " << message->what << std::endl;
	char* flattenedMessage = new char[message->FlattenedSize()];
	message->Flatten(flattenedMessage,message->FlattenedSize());
	std::string cc(flattenedMessage);
	LaunchInNewThread("MessagePlugin::SendMessage",B_NORMAL_PRIORITY, 
		&SendThread, addonRef, pluginManager, protocolSig, 
		cc.c_str());
}

PluginManager::Impl::Impl(std::vector<std::string> additionalPaths)
	: fPluginPaths(),
      fReplyHandler(nullptr)
{
	std::cout << "Impl(paths)" << std::endl;
	for (auto ap: additionalPaths)
	{
		std::cout << "Impl(paths): Path: " << ap << std::endl;
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
		std::cout << "Impl(paths): Processing path" << std::endl;
		entry_ref ref;
		entry.GetRef(&ref);
		DescribePlugin(ref,this);
	}
}

PluginManager::Impl::Impl()
	: fPluginPaths(),
      fReplyHandler(nullptr)
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
			for (int prot = 0;prot < plugin.description.count;prot++) 
			{
				const protocol* protocol = &(plugin.description.protocolList[prot]);
				if (0 == strcmp(protocol->signature, signature))
				{
					matches.emplace_back(plugin.description.signature);
				}
			}
		}
		return std::move(matches);
	}
	
void
PluginManager::Impl::SendMessage(const std::string pluginsig, const char* protocolSig, BMessage* message)
{
	std::cout << "Impl::SendMessage(): what: " << message->what << std::endl;
	// Call its SendMessage function
	for (auto plugin: plugins)
	{
		if (0 == strcmp(plugin.description.signature, pluginsig.c_str()))
		{
			std::cout << "Impl::SendMessage():   matching plugin for protocol" << std::endl;
			MessagePlugin(plugin.path, this, protocolSig, message);
		}
	}
}

void
PluginManager::Impl::SetReplyHandler(BHandler* replyHandler)
{
    fReplyHandler = replyHandler;
}

void PluginManager::Impl::ReceiveReply(const char* flattenedMessage)
{
    
    std::cout << "PluginManager::Impl::ReceiveReply: reply received "
        << std::endl;
    BMessage* msg = new BMessage();
    msg->Unflatten(flattenedMessage);
    
    // now handle it somewhere...
    if (nullptr != fReplyHandler)
    {
        fReplyHandler->Looper()->Lock();
        fReplyHandler->MessageReceived(msg);
        fReplyHandler->Looper()->Unlock();
    }
}
	
// internal methods
void
PluginManager::Impl::AddDescription(entry_ref path, plugin_descriptor* desc)
{
	std::cout << "Impl::AddDescription()" << std::endl;
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
	
	const int size = desc->count;
	
	struct protocol* protocolList = new protocol[size];
	for (int i = 0;i < desc->count;i++) 
	{
	/*
		char* psig = nullptr;
		char* pver = nullptr;
		char* pminver = nullptr;
		char* pmaxver = nullptr;
		strcpy(psig,protocolList[i].signature);
		strcpy(pver,protocolList[i].version);
		strcpy(pminver,protocolList[i].minVersion);
		strcpy(pmaxver,protocolList[i].maxVersion);
		*/
		protocolList[i].signature = strdup(desc->protocolList[i].signature);
		protocolList[i].version = strdup(desc->protocolList[i].version);
		protocolList[i].minVersion = strdup(desc->protocolList[i].minVersion);
		protocolList[i].maxVersion = strdup(desc->protocolList[i].maxVersion);
		std::cout << "Impl::AddDescription:   protocol sig: " << protocolList[i].signature << std::endl;
	}
	
	/*
	struct plugin_descriptor* mypd = new plugin_descriptor(sig.c_str(),summary.c_str(),version.c_str(),
		&(const int)(size),&protocolList);
		*/
	// done a deep copy, so a shallow copy now will work find
	struct plugin_descriptor mypd = {sig.c_str(),summary.c_str(),version.c_str(),
		size,protocolList};
	std::cout << "Impl::AddDescription: summary: " << mypd.summary << std::endl;
	struct plugin plug = {path,mypd};
	plugins.emplace_back(plug); // shallow copy now fine, as we've allocated everything in our memory space
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
std::vector<plugin>
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
PluginManager::SetReplyHandler(BHandler* replyHandler)
{
    fImpl->SetReplyHandler(replyHandler);
}

void
PluginManager::SendMessage(const std::string pluginid,const char* protocolSig,BMessage* message)
{
	std::cout << "PluginManager::SendMessage: what: " << message->what << std::endl;
	fImpl->SendMessage(pluginid,protocolSig,message);
}

const std::vector<std::string>
PluginManager::GetProblems()
{
	return fImpl->GetProblems();
}




