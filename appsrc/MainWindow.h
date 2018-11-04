#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <vector>
#include <string>
#include <Window.h>
#include "../apisrc/PluginManager.h"

class BTextView;

const uint32 M_LIST = 'list';
const uint32 M_PROBLEMS = 'prob';
const uint32 M_PATHS = 'path';

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
			BTextView*	fStringView;
};

#endif
