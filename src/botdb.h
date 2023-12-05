/*
	Various global databases by cat02e@fsu.edu
*/


#ifndef BOTDB_H
#define BOTDB_H

#include "hostlist.h"
#include "player.h"
#include "clientprot.h"
#include "botinfo.h"

class opEntry
{
	Uint32 loginCount;		// Overall count of logins, this instantiation
	Uint32 failedAttempts;	// Count of failed logins, this instantiation
	Operator_Level access;
	String name, pass;

	friend struct BOT_DATABASE;

public:
	opEntry(const char *nname, const char *ppass, Operator_Level aaccess);

	void setPassword(const char *ppass);
	void setName(const char *nname);
	void setAccess(Operator_Level aaccess);

	Operator_Level getAccess();

	void addCounter();
	void addFailure();
	Uint32 getOverallCount();
	Uint32 getFailureCount();
	char *getName();

	bool validateName(const char *nname);
	bool validatePass(const char *ppass);
};

class cmdAlias
{
	String cmd, alias;

public:
	cmdAlias(const char *ccmd, const char *aalias);

	bool isCmd(const char *ccmd);
	bool isAlias(const char *aalias);

	bool test(char *&ccmd);	// change command to alias on match

	String &getAlias();
	String &getCommand();
};

struct BOT_DATABASE
{
	// Database
	char path[532];

	BOT_DATABASE();

	Uint32 lastSave;			// in hundredths of a second

	// Parameters
	BOT_INFO botInfo;

	// Spawns
	hostList spawns;

	void loadSpawns();

	// Operators
	_linkedlist <opEntry> opList;
	bool operatorsUpdated;

	opEntry *findOperator(const char *name);
	opEntry *addOperator(const char *name, Operator_Level level);
	opEntry *addOperator(const char *name, const char *pass, Operator_Level level);
	bool removeOperator(const char *name);

	void loadOperators();
	void saveOperators();

	//OmegaFirebolt added loadOperators2
	void loadOperators2();

	// Aliasing
	_linkedlist <cmdAlias> aliasList;
	bool aliasesUpdated;

	void aliasCommand(char *&command);
	void addAlias(const char *command, const char *alias);
	bool killAlias(const char *alias);

	cmdAlias *findAlias(const char *alias);

	void loadAliases();
	void saveAliases();

	String getAliasList(const char *command);

	// INI
	char regName[40];
	char regEMail[40];
	char regState[40];
	BYTE regAge;
	Uint32 loginIP;
	Uint16 loginPort;
	bool recordLogins;
	Uint16 resX;
	Uint16 resY;
	bool chatterLog;
	bool encryptMode;
	char windowCaption[64];
	bool maskBan;
	bool remoteInterpreter;
	bool remoteOperator;
	bool playerVoices;
	Uint32 saveInterval;			// in hundredths of a second
	char spawnsFile[64];
	char commandsFile[64];
	char operatorsFile[64];
	Uint32 maxSpawns;
	bool noTerminal;
	bool forceContinuum;	// hidden option
	bool runSilent;			// hidden option
	bool disablePub;
	bool noisySpectator;
	Uint32 maxTries;
	char initialChatChannels[64];

	void reloadINI(bool doLogin);	// doLogin: Ignore [Login] section to avoid DoS attempts
};

#endif	// BOTDB_H
