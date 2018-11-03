#include "MainWindow.h"

#include <Application.h>
#include <Message.h>
#include <StringView.h>
#include <Layout.h>
#include <Button.h>

#include "../apisrc/PluginManager.h"
#include "../apisrc/PluginAPI.h"
#include "../pluginsrc/HaikuProtocol.h"
#include "../pluginsrc/LimerickProtocol.h"


MainWindow::MainWindow(void)
	:	BWindow(BRect(100,100,500,400),"Poem Viewer",B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
	,fPluginManager() // goes and finds all protocols in the libs folder
	,fHaikuPlugins(fPluginManager.FindForProtocol(sig_haikuprotocol,"1.0"))
	,fLimerickPlugins(fPluginManager.FindForProtocol(sig_limerickprotocol,"1.0"))
{
	// Normally we'd keep the list, and send introspection messages to each
	//     For now though, we'll just fetch the first implementation of each
	
	fStringView = new BStringView("poemview","Poem goes here");
	
	// Create UI with button to fetch a haiku or limerick
	BLayoutBuilder::Group<> layout(this,B_VERTICAL,0)
		.Add(fStringView)
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
		case M_GET_RANDOM_HAIKU:
		{
			fPluginManager.SendMessage(fHaikuPlugins[0],msg);
			break;
		}
		case M_GET_RANDOM_LIMERICK:
		{
			fPluginManager.SendMessage(fLimerickPlugins[0],msg);
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
