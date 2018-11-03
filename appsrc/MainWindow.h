#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <vector>
#include <string>
#include <Window.h>
#include "../apisrc/PluginManager.h"

class BStringView;

class MainWindow : public BWindow
{
public:
						MainWindow(void);
			void		MessageReceived(BMessage *msg);
			bool		QuitRequested(void);
			
private:
			PluginManager	fPluginManager;
			const std::vector<std::string>	fHaikuPlugins;
			const std::vector<std::string>	fLimerickPlugins;
			BStringView*	fStringView;
};

#endif
