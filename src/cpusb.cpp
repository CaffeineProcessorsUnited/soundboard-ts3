/*
* TeamSpeak 3 demo plugin
*
* Copyright (c) 2008-2015 TeamSpeak Systems GmbH
*/

#ifdef _WIN32
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <assert.h>

#include <string>
#include <iostream>
#include <sstream>

// not using cmake...
//#include "CPUSBConfig.h"

#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "teamspeak/clientlib_publicdefinitions.h"
#include "ts3_functions.h"
#include "cpusb.hpp"

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 20

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

static char* pluginID = NULL;

char appPath[PATH_BUFSIZE];
char resourcesPath[PATH_BUFSIZE];
char configPath[PATH_BUFSIZE];
char pluginPath[PATH_BUFSIZE];

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if(WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif

/*********************************** Required functions ************************************/
/*
* If any of these required functions is not implemented, TS3 will refuse to load the plugin
*/

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
	#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if(!result) {
        const wchar_t* name = L"CPU Soundboard";
		if(wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
            result = "CPU Soundboard";  /* Conversion failed, fallback here */
		}
	}
	return result;
	#else
    return "CPU Soundboard";
	#endif
}

/* Plugin version */
const char* ts3plugin_version() {
	return "1.0";
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "CaffeineProcessorsUnited";
}

/* Plugin description */
const char* ts3plugin_description() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "The TS3 integration for the CPU Soundboard";
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
	ts3Functions = funcs;
}

/*
* Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
* If the function returns 1 on failure, the plugin will be unloaded again.
*/
int ts3plugin_init() {
	ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
	ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
	ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE);

	return 0;
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}

}

/****************************** Optional functions ********************************/
/*
* Following functions are optional, if not needed you don't need to implement them.
*/

/* Tell client if plugin offers a configuration window. If this function is not implemented, it's an assumed "does not offer" (PLUGIN_OFFERS_NO_CONFIGURE). */
int ts3plugin_offersConfigure() {
	return PLUGIN_OFFERS_NO_CONFIGURE;  /* In this case ts3plugin_configure does not need to be implemented */
}

/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData and ts3plugin_initMenus */
void ts3plugin_freeMemory(void* data) {
	free(data);
}

/*
* Plugin requests to be always automatically loaded by the TeamSpeak 3 client unless
* the user manually disabled it in the plugin dialog.
* This function is optional. If missing, no autoload is assumed.
*/
int ts3plugin_requestAutoload() {
    return 1; /* 1 = request autoloaded, 0 = do not request autoload */
}


int ts3plugin_onTextMessageEvent(uint64 serverConnectionHandlerID, anyID targetMode, anyID toID, anyID fromID, const char* fromName, const char* fromUniqueIdentifier, const char* message, int ffIgnored) {
    printf("CPUSB: onTextMessageEvent %llu %d %d %s %s %d\n", (long long unsigned int)serverConnectionHandlerID, targetMode, fromID, fromName, message, ffIgnored);

	/* Friend/Foe manager has ignored the message, so ignore here as well. */
	if(ffIgnored) {
		return 0; /* Client will ignore the message anyways, so return value here doesn't matter */
	}
	anyID myID;
	if(ts3Functions.getClientID(serverConnectionHandlerID, &myID) != ERROR_ok) {
        ts3Functions.logMessage("Error querying own client id", LogLevel_ERROR, "CPUSB", serverConnectionHandlerID);
		return 0;
	}
	if(fromID != myID && toID == myID) {  /* Don't reply when source is own client */
		std::stringstream encoded;
		encoded << "{\"client\":{\"id\":\"" << fromID << "\",\"unique\":\"" << fromUniqueIdentifier << "\",\"name\":\"" << html_encode(fromName) << "\"},\"data\":\"" << html_encode(message) << "\"}";
		std::string json = encoded.str();
		ts3Functions.logMessage(json.c_str(), LogLevel_INFO, "CPUSB", serverConnectionHandlerID);
		std::stringstream caller;
		caller << "python " << pluginPath << "cpusb_plugin/command.py \"" << pluginPath << "cpusb_plugin/\" \'" + json + "\'";
		int result = system(caller.str().c_str());
		if (result != 0) {
			// Couln't execute it :(
			ts3Functions.logMessage("Error executing python script", LogLevel_ERROR, "CPUSB", serverConnectionHandlerID);
		} else {
			ts3Functions.logMessage("Sent to server", LogLevel_ERROR, "CPUSB", serverConnectionHandlerID);
		}
		return 1; // Hide the message in the client
	}
	return 0;  /* 0 = handle normally, 1 = client will ignore the text message */
}

int ts3plugin_onClientPokeEvent(uint64 serverConnectionHandlerID, anyID fromClientID, const char* pokerName, const char* pokerUniqueIdentity, const char* message, int ffIgnored) {
	anyID myID;

    printf("CPUSB onClientPokeEvent: %llu %d %s %s %d\n", (long long unsigned int)serverConnectionHandlerID, fromClientID, pokerName, message, ffIgnored);

	/* Check if the Friend/Foe manager has already blocked this poke */
	if(ffIgnored) {
		return 0;  /* Client will block anyways, doesn't matter what we return */
	}

	return 1;

	/* Example code: Send text message back to poking client */
	if(ts3Functions.getClientID(serverConnectionHandlerID, &myID) != ERROR_ok) {  /* Get own client ID */
        ts3Functions.logMessage("Error querying own client id", LogLevel_ERROR, "CPUSB", serverConnectionHandlerID);
		return 0;
	}
	if(fromClientID != myID) {  /* Don't reply when source is own client */
		std::string usage = "Usage: ";
		if(ts3Functions.requestSendPrivateTextMsg(serverConnectionHandlerID, usage.c_str(), fromClientID, NULL) != ERROR_ok) {
            ts3Functions.logMessage("Error requesting send text message", LogLevel_ERROR, "CPUSB", serverConnectionHandlerID);
		}
	}
	return 1;  /* 0 = handle normally, 1 = client will ignore the poke */
}

std::string html_encode(std::string str) {
	std::stringstream encoded;
	for(std::string::size_type i = 0; i < str.size(); ++i) {
		if (str[i] == '"') {
			encoded << "%22";
		} else if (str[i] == '\'') {
			encoded << "%27";
		} else {
			encoded << str[i];
		}
	}
	return encoded.str();
}

std::string html_encode(char* str) {
	std::stringstream _str;
	_str << str;
	return html_encode(_str.str());
}
