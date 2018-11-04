#include "MainWindow.h"

#include <Application.h>
#include <Message.h>
#include <StringView.h>
#include <Layout.h>
#include <Button.h>
#include <String.h>

#include "../apisrc/PluginManager.h"
#include "../apisrc/PluginAPI.h"
#include "../pluginsrc/HaikuProtocol.h"
#include "../pluginsrc/LimerickProtocol.h"

#include <LayoutBuilder.h>
#include <Layout.h>
#include <Message.h>

#include <iostream>

MainWindow::MainWindow(void)
	:	BWindow(BRect(100,100,500,400),"Poem Viewer",B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
	,fPluginManager(std::vector<std::string> {
		"/boot/system/non-packaged/add-ons/MessagePlugins/GetHaikuPlugin"
	} ) // goes and finds all protocols in the libs folder
	,fHaikuPlugins(fPluginManager.FindForProtocol(sig_haikuprotocol,"1.0"))
	,fLimerickPlugins(fPluginManager.FindForProtocol(sig_limerickprotocol,"1.0"))
{
	// Normally we'd keep the list, and send introspection messages to each
	//     For now though, we'll just fetch the first implementation of each
	std::cout << "Loaded PoemViewer main window" << std::endl;
	
	fStringView = new BTextView("poemview");
	fStringView->SetText("Poem goes here");
	fStringView->MakeEditable(false);
	
	// Create UI with button to fetch a haiku or limerick
	BLayoutBuilder::Group<>(this,B_VERTICAL,0)
		.Add(fStringView)
		.Add(new BButton("paths","List Plugin Paths",new BMessage(M_PATHS)))
		.Add(new BButton("list","List Plugins",new BMessage(M_LIST)))
		.Add(new BButton("problems","List Problems",new BMessage(M_PROBLEMS)))
		.Add(new BButton("gethaiku","Get Haiku",new BMessage(M_GET_RANDOM_HAIKU)))
		.Add(new BButton("getlimerick","Get Limerick",new BMessage(M_GET_RANDOM_LIMERICK)))
		.AddGlue()
		.SetInsets(B_USE_DEFAULT_SPACING)
		.End();
	
}


void
MainWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		case M_PATHS:
		{
			BString pathInfo("Plugin Paths Found:-\n");
			BPath bp;
			for (auto entry: fPluginManager.GetAllPluginPaths())
			{
				std::cout << "PoemViewer processing path" << std::endl;
				if (B_OK == entry.GetPath(&bp))
					pathInfo << bp.Path() << "\n";
			}
			fStringView->SetText(pathInfo);
			std::cout << pathInfo << std::endl;
			break;
		}
		case M_LIST:
		{
			BString pluginInfo;
			auto plugins = fPluginManager.GetAllPlugins();
			pluginInfo << "Total: " << plugins.size() << "\n";
			for (auto plugin: plugins)
			{
				pluginInfo << "Plugin:-\n";
				pluginInfo << plugin.description.signature << "\n";
				pluginInfo << plugin.description.summary << "\n";
				pluginInfo << "    version: " << plugin.description.version << "\n";
				pluginInfo << "    protocols: " << plugin.description.count << "\n";
				for (int32 prot = 0;prot < plugin.description.count;prot++)
				{
					protocol p = plugin.description.protocolList[prot];
					pluginInfo << "        " << p.signature << "\n";
				}
			}
			fStringView->SetText(pluginInfo);
			std::cout << pluginInfo << std::endl;
			break;
		}
		case M_GET_RANDOM_HAIKU:
		{
			if (0 == fPluginManager.GetAllPlugins().size()) {
				fStringView->SetText("No plugins in folder!");
			} else {
				fPluginManager.SendMessage(fHaikuPlugins[0],msg);
			}
			break;
		}
		case M_GET_RANDOM_LIMERICK:
		{
			if (0 == fPluginManager.GetAllPlugins().size()) {
				fStringView->SetText("No plugins in folder!");
			} else {
				fPluginManager.SendMessage(fLimerickPlugins[0],msg);
			}
			break;
		}
		case M_PROBLEMS:
		{
			BString probs("Plugin Problems:-\n");
			for (auto prob: fPluginManager.GetProblems())
			{
				probs << prob.c_str() << "\n";
			}
			fStringView->SetText(probs);
			break;
		}
		case M_RECEIVE:
		{
			fStringView->SetText(msg->GetString("poem",0));
			break;
		}
		default:
		{
			BWindow::MessageReceived(msg);
			break;
		}
	}
}


bool
MainWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
