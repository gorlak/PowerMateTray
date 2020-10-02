#define VERSION_MAJOR			3
#define VERSION_MINOR			12
#define VERSION_PATCH			0

#define VERSION_STRINGIFY(major, minor, patch) wxT(#major) wxT(".") wxT(#minor) wxT(".") wxT(#patch)
#define VERSION_TOSTRING(major, minor, patch) VERSION_STRINGIFY(major,minor,patch)
#define VERSION_STRING VERSION_TOSTRING( VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH )

#define RESOURCE_VERSION VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, 0
#define RESOURCE_VERSION_STRINGIFY(major, minor, patch) #major "," #minor "," #patch ",0"
#define RESOURCE_VERSION_TOSTRING(major, minor, patch) RESOURCE_VERSION_STRINGIFY(major, minor, patch)
#define RESOURCE_VERSION_STRING RESOURCE_VERSION_TOSTRING( VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH )

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        101
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1000
#define _APS_NEXT_SYMED_VALUE           105
#endif
#endif
