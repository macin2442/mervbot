/*
	Import bot behavior from DLL's by cat02e@fsu.edu

	Cyan~Fire fixed MinGW compilation for DLLImports::talk()
*/


#ifndef BOT_H
#define BOT_H

class DLLImports;

#include "dllcore.h"

#if __linux__
#include "windows_compat.h"
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

typedef void (__stdcall *CALL_TALK)(BotEvent &event);

#define DLL_MAX_LOADED	32
#define DLL_NAMELEN		256

#ifdef DOTNET
#include <vcclr.h>

void __stdcall listen(BotEvent &event);

__gc class DotNetListener : public System::MarshalByRefObject, public MervInterfaces::IMerv
{
public:
	void Listen(System::IntPtr pEvent)
	{
		listen(*(BotEvent*)pEvent.ToPointer());
	}
};

__gc class DotNetPlugin
{
public:
	System::AppDomain *Domain;
	//System::Reflection::MethodInfo *Talk;
	//System::Object *Loader;
	MervInterfaces::IPlugin *Plugin;
};
#endif

class DLLImports
{
	CALL_TALK DLL_TALK[DLL_MAX_LOADED];
	HMODULE DLLhMod[DLL_MAX_LOADED];
	char ModuleName[DLL_MAX_LOADED][DLL_NAMELEN];

#ifdef DOTNET
	// Only one (normal vs .Net) will be loaded per each slot.
	gcroot<DotNetPlugin*> DotNetPlugins[DLL_MAX_LOADED];

	gcroot<DotNetListener*> Listener;
#endif

	class Host *h;

	void talk(int slot, BotEvent event);

#ifdef DOTNET
	bool importDotNet(char *file);
	void clearDotNet(int slot);

	void dotNetListen(BotEvent &event);
#endif

public:
	void talk(BotEvent event);			// Talk to the DLL
	bool importLibrary(char *file);		// Load callbacks from DLL
	void clearImports();				// Remove all references
	void clearImport(int slot);			// Remove reference
	char *getPlugin(int slot);			// Get plugin name, returns NULL if no plugin loaded

	DLLImports(Host &);					// Initialize all callbacks
	~DLLImports();						// Disable all callbacks
};

#endif	// BOT_H
