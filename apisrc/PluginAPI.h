#ifndef PLUGINAPI_H
#define PLUGINAPI_H

struct protocol {
	const char* signature;
	const char* version;
	const char* minVersion;
	const char* maxVersion;
};
struct plugin_descriptor {
	const char* signature;
	const char* summary;
	const char* version;
	const int count;
	const protocol* protocolList;
};
class BMessage;

#endif
