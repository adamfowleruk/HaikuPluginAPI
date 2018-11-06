#include "../apisrc/Plugin.h"

#include <Message.h>

#include "HaikuProtocol.h"
#include "LimerickProtocol.h"
#include "GetHaikuPlugin.h"

#include <vector>
#include <cstring>
#include <string>
#include <iterator>
#include <iostream>

// IMPLEMENT OUR HAIKU MANAGER CLASS FROM OUR HEADER FILE

PoemManager::PoemManager(std::vector<std::string> poemlist)
	: poems(std::move(poemlist)) // TODO std:move this and take ownership?
{
}

PoemManager::~PoemManager()
{
	// nothing to do - handled by default (C++11)
}

void
PoemManager::MessageReceived(BMessage* message)
{
	std::cout << "GetHaikuPlugin::MessageReceived: what: " << 
        message->what << std::endl;
	std::cout << "GetHaikuPlugin::MessageReceived: M_GET_RANDOM_HAIKU: " 
        << M_GET_RANDOM_HAIKU << std::endl;
	std::cout << "GetHaikuPlugin::MessageReceived: M_GET_RANDOM_LIMERICK: " 
        << M_GET_RANDOM_LIMERICK << std::endl;
	switch(message->what) {
	case M_GET_RANDOM_HAIKU:
	case M_GET_RANDOM_LIMERICK:
	{
		// do something
		const char* poem = GetRandom();
		BMessage* reply = new BMessage(M_RECEIVE);
		reply->AddString("poem",poem);
		std::cout << "GetHaikuPlugin::MessageReceived: Sending Poem: " 
            << poem << std::endl;
        std::cout << "BMessage reply output:-" << std::endl;
        reply->PrintToStream();
		message->SendReply(reply);
		std::cout << "GetHaikuPlugin::MessageReceived: Message reply sent" 
            << std::endl;
		
		break;
	}
	case M_ADD_HAIKU:
	{
		// TODO implement the other function to add a haiku to our list 
        //      and respond with index
		;
		break;
	}
	}
}

const char*	
PoemManager::GetRandom()
{
	return (*select_randomly(poems.begin(),poems.end())).c_str();
}

const char*	
PoemManager::AtIndex(uint32 idx)
{
	if (idx > poems.size() - 1)
		return nullptr;
		
	return poems.at(idx).c_str();
}

const uint32	
PoemManager::Count()
{
	return poems.size();
}

const uint32	
PoemManager::AddPoem(const char* poem)
{
	poems.emplace_back(poem);
	return poems.size() - 1;
}





// NOW DEFINE THE PLUGIN'S HOOKS AND SUPPORTED PROTOCOLS
// The poem managers would likely be completely different classes in 
//     real life. I've used a single class purely as a convenience for 
//     myself in this example.

const char* signature = "x.vnd/GetHaikuPlugin";

PoemManager* haikus = new PoemManager(
	std::vector<std::string> {
		"Haikus are easy.\n"
		"But sometimes they don't make sense.\n"
		"Refrigerator."
		,
		"I'm much funnier.\n"
		"when i am drunk off my butt.\n"
		"sadly, i'm sober."
	}
);
PoemManager* limericks = new PoemManager(
	std::vector<std::string> {
		"A fellow jumped off a high wall,\n"
		"And had a most terrible fall.\n" 
		"He went back to bed, \n"
		"With a bump on his head,\n" 
		"That's why you don't jump off a wall."
		,		
		"There once was a young man called Kyle,\n"
		"who worked at the circus a while.\n"
		"He flew through the air,\n"
		"with hardly a care,\n"
		"and that's why his body's in a pile."
		,
		"There was an Old Man with a beard,\n" 
		"Who said, It is just as I feared!\n"
		"Two Owls and a Hen,\n"
		"Four Larks and a Wren,\n"
		"Have all built their nests in my beard!"
	}
);

// This single plugin library supports multiple protocols
// Each protocol has a suite of messages it will support

extern "C"
plugin_descriptor
describe_plugin()
{
	//const struct protocol hp = {sig_haikuprotocol, "1.1","1.0","1.1"};
	//const struct protocol lp = {sig_limerickprotocol, "1.0","1.0","1.0"};
	static const struct protocol protocols[] = {
		{sig_haikuprotocol, "1.1","1.0","1.1"},{sig_limerickprotocol, 
           "1.0","1.0","1.0"}
	};
	const struct plugin_descriptor description = {
		signature,"Operations on Poems","1.1",
		2,
		protocols
	};
	std::cout << "GetHaikuPlugin::describe_plugin: first protocol from prot array: " << protocols[0].signature 
        << std::endl;
	std::cout << "GetHaikuPlugin::describe_plugin: first protocol from description: " 
        << description.protocolList[0].signature << std::endl;
	return description;
}

void
receive_message(const char* protocol,BMessage* message) 
{
	// This is where the plugin would hand off to
	//    one or more classes, depending on the list of protocols supported 
    //    and their handlers
	
	if (0 == strcmp(sig_haikuprotocol,protocol))
	{
		std::cout << "GetHaikuPlugin::message_received: Got a haiku request" << std::endl;
		// we know that the PoemManager instance supports to get random 
        // protocol
		haikus->MessageReceived(message);
	} else {
		if (0 == strcmp(sig_limerickprotocol,protocol))
		{
			std::cout << "GetHaikuPlugin::message_received: Got a limerick request" 
                << std::endl;
			// we know that the LlimerickManager instance supports the add 
            //      protocol
			limericks->MessageReceived(message);
		}
	}
}