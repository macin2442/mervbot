#include "botdll.h"

#include "datatypes.h"
#include "algorithms.h"
#include "clientprot.h"
#include "host.h"
#include "botdb.h"

typedef void (__stdcall *pfnEnsureInit)(void);
typedef void (__stdcall *pfnForceTerm)(void);

#ifdef DOTNET
inline char* StringToChar(System::String *string)
{
	__wchar_t *buffer = new __wchar_t[string->Length + 1];
	memset(buffer, 0, string->Length + 1);

	for (int i = 0; i < string->Length; i++)
		buffer[i] = string->get_Chars(i);

	char *buffer2 = new char[string->Length + 1];
	memset(buffer2, 0, string->Length + 1);

	WideCharToMultiByte(CP_ACP, 0, buffer, string->Length, buffer2, string->Length + 1, NULL, NULL);

	delete[] buffer;

	return buffer2;
}

bool DLLImports::importDotNet(char *file)
{
	System::AppDomain *domain;

	int slot;
	
	// Find open import slot
	for (slot = 0; slot < DLL_MAX_LOADED; ++slot)
		if (!DLL_TALK[slot] && !DotNetPlugins[slot])
			break;

//	System::Byte buffer[];
//
//	try
//	{
//		System::String *name_1 = new System::String(file);
//		System::String *name_2 = new System::String(".dll");
//		System::String *name = System::String::Concat(name_1, name_2);
//
//		// First, load the file.
//		System::IO::FileStream *fs = System::IO::File::Open(name, System::IO::FileMode::Open, System::IO::FileAccess::Read, System::IO::FileShare::Read);
//		buffer = __gc new System::Byte[(int)fs->Length];
//		fs->Read(buffer, 0, buffer->Length);
//		fs->Close();
//	}
//	catch (System::Exception *ex)
//	{
//		h->logEvent("%s load failed - file not found.", file);
//#ifdef _DEBUG
//		System::Console::WriteLine(ex->ToString());
//#endif
//		return false;
//	}

	DotNetPlugin *p = new DotNetPlugin();
	domain = System::AppDomain::CreateDomain(".Net plugin domain");
	p->Domain = domain;

	/*System::Reflection::Assembly *assembly;

	try
	{
		assembly = p->Domain->Load(buffer);
	}
	catch (System::Exception *ex)
	{
		h->logEvent("%s is not a .Net plugin.", file);
#ifdef _DEBUG
		System::Console::WriteLine(ex->ToString());
#endif
		System::AppDomain::Unload(p->Domain);
		return false;
	}

	*/try
	{
		System::String *name_1 = new System::String(file);
		System::String *name_2 = new System::String(".dll");
		System::String *name = System::String::Concat(name_1, name_2);

		//plugin = dynamic_cast<MervInterfaces::IPlugin*>(p->Domain->CreateInstanceAndUnwrap(assembly->GetName()->Name, "PluginLoader"));
		p->Plugin = dynamic_cast<MervInterfaces::IPlugin*>(domain->CreateInstanceFromAndUnwrap(name, "PluginLoader"));
	}
	catch (System::Exception *ex)
	{
		h->logEvent("%s is not a valid .Net plugin (could not load loader)", file);
#ifdef _DEBUG
		System::Console::WriteLine(ex->ToString());
#endif
		System::AppDomain::Unload(p->Domain);
		return false;
	}

	DotNetPlugins[slot] = p;

	strncpy(ModuleName[slot], file, DLL_NAMELEN);

	talk(slot, makeInit(&listen, &(h->playerlist), &(h->flagList), (CALL_MAP)(h->map), &(h->brickList), (CALL_PARAMS)h->creation_parameters, (void*)&h->settings));
	if (h->inArena)
	{
		talk(slot, makeArenaEnter(h->botInfo.initialArena, h->Me, h->billerOnline));
		talk(makeArenaSettings(&h->settings));

		if (h->gotMap)
			talk(makeMapLoaded());
	}

	return true;
}

void DLLImports::clearDotNet(int slot)
{
	if (slot < 0 || slot >= DLL_MAX_LOADED)
		return;

	if (DLL_TALK[slot])
		return;

	if (!DotNetPlugins[slot])
		return;

	talk(slot, makeTerm());

	System::AppDomain::Unload(DotNetPlugins[slot]->Domain);
	DotNetPlugins[slot] = NULL;
}
#endif

void DLLImports::clearImport(int slot)
{
	if (slot < 0 || slot >= DLL_MAX_LOADED)
		return;

	if (DLL_TALK[slot])
	{
		talk(slot, makeTerm());

		// ** Mixed-mode plugin shutdown **
		// NOTE: This function is not strictly required for mixed-mode DLLs but it is the approach that Microsoft recommends.
		// Any mixed-mode DLL should terminate the C runtime in this function.
		pfnForceTerm pfnDll = (pfnForceTerm)GetProcAddress(DLLhMod[slot], "DllForceTerm");
		if (pfnDll)
			pfnDll();
		// ********************************

		FreeLibrary(DLLhMod[slot]);

		DLLhMod[slot] = NULL;
		DLL_TALK[slot] = NULL;
	}
#ifdef DOTNET
	else if (DotNetPlugins[slot])
		clearDotNet(slot);
#endif
}

void DLLImports::clearImports()
{
	for (int slot = 0; slot < DLL_MAX_LOADED; ++slot)
	{
		clearImport(slot);
	}
}

void __stdcall listen(BotEvent &event)
{
	Host *h = (Host*)event.handle;

#ifndef _DEBUG
try {
#endif

	switch (event.code)
	{
	case EVENT_Echo:
		{
			char *msg = (char*)event.p[0];

			h->logEvent("Ext: %s", msg);
		}
		break;
	case EVENT_Say:
		{
			int mode = *(int*)&event.p[0];
			int sound = *(int*)&event.p[1];
			int ident = *(int*)&event.p[2];
			char *msg = (char*)event.p[3];

			// Fix team private messages, needs a player ident param, not a team number
			if (mode == MSG_TeamPrivate)
			{
				_listnode <Player> *parse = h->playerlist.head;

				while (parse)
				{
					Player *p = parse->item;

					if (p->team == ident)
					{
						ident = p->ident;

						h->tryChat(mode, sound, ident, msg);

						return;
					}

					parse = parse->next;
				}

				break;
			}

			h->tryChat(mode, sound, ident, msg);
		}
		break;
	case EVENT_Ship:
		{
			int ship = *(int*)&event.p[0];

			h->postRR(generateChangeShip((Ship_Types)ship));
		}
		break;
	case EVENT_Team:
		{
			int team = *(int*)&event.p[0];

			h->postRR(generateChangeTeam(team));
		}
		break;
	case EVENT_Die:
		{
			Player *p = (Player*)event.p[0];

			if (h->Me)
				h->postRR(generateDeath(p->ident, h->Me->bounty));
		}
		break;
	case EVENT_Attach:
		{
			Player *p = (Player*)event.p[0];

			h->postRR(generateAttachRequest(p->ident));
		}
		break;
	case EVENT_Detach:
		{
			h->postRR(generateAttachRequest(UNASSIGNED));
		}
		break;
	case EVENT_Following:
		{
			bool following = *(bool*)&event.p[0];

			h->follow = !following ? NULL : h->playerlist.head;
		}
		break;
	case EVENT_Flying:
		{
			bool flying = *(bool*)&event.p[0];

			h->DLLFlying = flying;
		}
		break;
	case EVENT_DropBrick:
		{
			h->postRU(generateBrickDrop((Uint16)h->Me->tile.x, (Uint16)h->Me->tile.y));
		}
		break;
	case EVENT_Banner:
		{
			BYTE *buffer = (BYTE*)event.p[0];

			h->postRR(generateChangeBanner(buffer));
		}
		break;
	case EVENT_GrabFlag:
		{
			int flag = *(int*)&event.p[0];

			h->postRR(generateFlagRequest(flag));
		}
		break;
	case EVENT_SendPosition:
		{
			bool reliable = *(bool*)&event.p[0];

			h->sendPosition(reliable);
		}
		break;
	case EVENT_FireWeapon:
		{
			weaponInfo *wi = (weaponInfo*)event.p[0];

			h->sendPosition(false, h->getHostTime(), wi->type, wi->level, wi->shrapBounce, wi->shrapLevel, wi->shrapCount, wi->fireType);
		}
		break;
	case EVENT_DropFlags:
		{
			h->postRR(generateFlagDrop());
		}
		break;
	case EVENT_ChangeArena:
		{
			String name = (char*)event.p[0];

			if (!invalidArena(name.msg))
				h->changeArena(name.msg);
		}
		break;
	case EVENT_MoveObjects:
		{
			lvzObject *objects = (lvzObject *)event.p[0];
			int num_objects = *(int*)&event.p[1];
			int player_id = *(int*)&event.p[2];

			if (num_objects < 0)
				break;

			h->postRR(generateObjectModify(player_id, objects, num_objects));
		}
		break;
	case EVENT_GrabBall:
		{
			int id = *(int*)&event.p[0];

			_listnode<PBall> *parse = h->ballList.head;

			while (parse)
			{
				PBall *pb = parse->item;

				if (id == pb->ident)
				{
					h->postRR(generatePowerballRequest(pb->hosttime, id));
					break;
				}

				parse = parse->next;
			}
		}
		break;
	case EVENT_FireBall:
		{
			int id = *(int*)&event.p[0];
			int x = *(int*)&event.p[1];
			int y = *(int*)&event.p[2];
			int xvel = *(int*)&event.p[3];
			int yvel = *(int*)&event.p[4];

			h->postRR(generatePowerballUpdate(h->getHostTime(), id, x, y, xvel, yvel, 0xffff));
			// Cyan~Fire noticed this didn't compile well in MinGW
		}
		break;
	case EVENT_ToggleObjects:
		{
			objectInfo *objects = (objectInfo *)event.p[0];
			int num_objects = *(int*)&event.p[1];
			int player_id = *(int*)&event.p[2];

			if (num_objects < 0)
				break;

			if (h->hasSysOp)
			{
				h->postRR(generateObjectToggle(player_id, objects, num_objects));
			}
			else
			{
				String s;

				s += "*objset ";

				for (int i = 0; i < num_objects; ++i)
				{
					if (objects[i].disabled)
					{
						s += "-";
					}
					else
					{
						s += "+";
					}

					s += objects[i].id;
					s += ",";
				}

				if (player_id == UNASSIGNED)
				{
					h->tryChat(MSG_Public, SND_None, 0, s.msg);
				}
				else
				{
					h->tryChat(MSG_Private, SND_None, player_id, s.msg);
				}
			}
		}
		break;
	case EVENT_ChangeSettings:
		{
			_linkedlist <String> *settings = (_linkedlist <String> *)event.p[0];

			h->postRR(generateChangeSettings(*settings));
		}
		break;
	case EVENT_SpawnBot:
		{
			if (h->botInfo.db->spawns.getConnections() >= h->botInfo.maxSpawns)
				break;

			BOT_INFO bi;
			bi.set(h->botInfo);
 
			String name		= (char*)event.p[0];
			String password = (char*)event.p[1];
			String staff	= (char*)event.p[2];
			String arena	= (char*)event.p[3];

			// Name
			if (invalidName(name.msg))
				break;

			// Password
			if (password.IsEmpty())
				password = bi.password;

			// Staff
			if (staff.IsEmpty())
				staff = bi.staffpw;

			// Arena
			if (arena.IsEmpty())
				arena = bi.initialArena;

			if (invalidArena(arena.msg))
				break;

			bi.setLogin(name.msg, password.msg, staff.msg);
			bi.setArena(arena.msg, bi.initialShip, bi.xres, bi.yres, bi.allowAudio);

			bi.db->spawns.connectHost(bi);
		}
		break;
	}

#ifndef _DEBUG
} catch (...)
{ h->logEvent("ERROR: Exception during EVENT %i", event.code); }
#endif

}

bool DLLImports::importLibrary(char *files)
{
	String s = files;

	String plugin = s.split(',');

	if (s.len != 0)
		importLibrary(s.msg);

	if (plugin.len == 0)
		return false;

	int slot;

	// Find open import slot
	for (slot = 0; slot < DLL_MAX_LOADED; ++slot)
#ifdef DOTNET
		if (!DLL_TALK[slot] && !DotNetPlugins[slot])
#else
		if (!DLL_TALK[slot])
#endif
			break;

	if (slot == DLL_MAX_LOADED)
		return false;

	// Avoid directory traversal exploits.
	trimString('/', plugin);
	trimString('\\', plugin);

#ifndef _DEBUG
try {
#endif

#ifdef DOTNET
	// First try to load it as .Net.
	if (importDotNet(plugin.msg))
		return true;
#endif

#if __linux__
	// TODO
	DLLhMod[slot] = dlopen(plugin.msg, RTLD_NOW);
#else
	DLLhMod[slot] = LoadLibrary(plugin.msg);
#endif

#ifndef _DEBUG
} catch (...)
{ h->logEvent("ERROR: Exception in DLLMain while loading plugin %s at slot %i", plugin.msg, slot); }
#endif

	if (!DLLhMod[slot])
		return false;

	// ** Mixed-mode plugin initialization **
	// NOTE: This function is not strictly required for mixed-mode DLLs but it is the approach that Microsoft recommends.
	// Any mixed-mode DLL should initialize the C runtime in this function.
	pfnEnsureInit pfnDll = (pfnEnsureInit)GetProcAddress(DLLhMod[slot], "DllEnsureInit");
	if (pfnDll)
		pfnDll();
	// **************************************

	strncpy(ModuleName[slot], plugin.msg, DLL_NAMELEN);

#if __linux__
	// TODO we cannot use ordinal values in Linux
#else
	DLL_TALK[slot] = (CALL_TALK)GetProcAddress(DLLhMod[slot], (LPCSTR)1);
#endif

	if (!DLL_TALK[slot])
	{
		h->logEvent("Could not find entry point in plugin.");
		return false;
	}

	talk(slot, makeInit(&listen, &(h->playerlist), &(h->flagList), (CALL_MAP)(h->map), &(h->brickList), (CALL_PARAMS)h->creation_parameters, (void*)&h->settings));
	if (h->inArena)
	{
		talk(slot, makeArenaEnter(h->botInfo.initialArena, h->Me, h->billerOnline));
		talk(makeArenaSettings(&h->settings));

		if (h->gotMap)
			talk(makeMapLoaded());
	}

	return true;
}

void DLLImports::talk(BotEvent event)
{
	for (int slot = 0; slot < DLL_MAX_LOADED; ++slot)
	{
		talk(slot, event);
	}
}

void DLLImports::talk(int slot, BotEvent event)
{
	if (slot < 0 || slot >= DLL_MAX_LOADED)
		return;

	event.handle = h;

	if (DLL_TALK[slot])
	{
#ifndef _DEBUG
try {
#endif

		DLL_TALK[slot](event);

#ifndef _DEBUG
} catch (...)
{ h->logEvent("ERROR: Exception in plugin %s at slot %i during event %i", ModuleName[slot], slot, event.code); }
#endif

	}
#ifdef DOTNET
	else if (DotNetPlugins[slot])
	{
//#ifndef _DEBUG
try {
//#endif

		DotNetPlugin *p = DotNetPlugins[slot];

		//p->Call(&event, Listener);
		p->Plugin->Talk(static_cast<System::IntPtr>(&event), Listener);

		//System::Object *params[] = __gc new System::Object*[3];
		//params[0] = __box(static_cast<System::IntPtr>(&event));
		//params[1] = Listener;
		//params[2] = ListenerMethod;
		
		//p->Talk->Invoke(p->Loader, params);

//#ifndef _DEBUG
} catch (System::Exception* ex)
{
	char *text = StringToChar(ex->Message);
	h->logEvent("ERROR: .Net exception in plugin %s at slot %i during event %i! Error: %s", ModuleName[slot], slot, event.code, text);

	delete[] text;
}
//#endif
	}
#endif
}

char *DLLImports::getPlugin(int slot)
{
	if (slot < 0 || slot >= DLL_MAX_LOADED)
		return NULL;
	if (!DLL_TALK[slot]
#ifdef DOTNET
	&& !DotNetPlugins[slot]
#endif
	) return NULL;

	return ModuleName[slot];
}

DLLImports::DLLImports(Host &host)
{
	for (int slot = 0; slot < DLL_MAX_LOADED; ++slot)
	{
		DLL_TALK[slot] = NULL;
		DLLhMod[slot] = NULL;
#ifdef DOTNET
		DotNetPlugins[slot] = NULL;
#endif
	}

#ifdef DOTNET
	Listener = new DotNetListener();
#endif

	h = &host;
}

DLLImports::~DLLImports()
{
	clearImports();
}