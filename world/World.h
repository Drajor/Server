#pragma once

#include "../common/types.h"
#include <list>
#include <map>
#include <string>

class EmuTCPServer;
class LoginServerConnection;
class UCSConnection;
class EQStreamFactory;
class EQStreamIdentifier;
class ZoneManager;
class AccountManager;
class DataStore;
class WorldClientConnection;
struct CharacterSelect_Struct;
struct CharCreate_Struct;

class World {
public:
	World(DataStore* pDataStore);
	~World();
	bool initialise();
	void update();

	void notifyIncomingClient(uint32 pLoginServerID, std::string pLoginServerAccountName, std::string pLoginServerKey, int16 pWorldAdmin = 0, uint32 pIP = 0, uint8 pLocal = 0);
	bool tryIdentify(WorldClientConnection* pConnection, uint32 pLoginServerAccountID, std::string pLoginServerKey);

	// Return whether World is connected to the Login Server.
	bool isLoginServerConnected();

	bool getLocked() { return mLocked; }
	void setLocked(bool pLocked);

	// Login Server requests response for Client who would like to join the World.
	int16 getUserToWorldResponse(uint32 pLoginServerAccountID);

	// Character Select Screen
	bool getCharacterSelectInfo(uint32 pWorldAccountID, CharacterSelect_Struct* pCharacterSelectData);
	bool isCharacterNameUnique(std::string pCharacterName);
	bool isCharacterNameReserved(std::string pCharacterName);
	void reserveCharacterName(uint32 pWorldAccountID, std::string pCharacterName);
	bool deleteCharacter(uint32 pWorldAccountID, std::string pCharacterName);
	bool createCharacter(uint32 pWorldAccountID, std::string pCharacterName, CharCreate_Struct* pData);
	bool isWorldEntryAllowed(uint32 pWorldAccountID, std::string pCharacterName);

	uint16 getZonePort(uint16 pZoneID, uint16 pInstanceID = 0);
private:
	struct IncomingClient {
		uint32 mAccountID; // Login Server Account
		std::string mAccountName; // Login Server Account
		std::string mKey;
		int16 mWorldAdmin;
		uint32 mIP;
		uint8 mLocal;
	};
	std::list<IncomingClient*> mIncomingClients; // These are Clients the Login Server has told us about but have not yet fully connected to the World.
	std::map<uint32, std::string> mReservedCharacterNames;

	void _checkUCSConnection();
	void _handleIncomingClientConnections();
	






	bool mInitialised;
	bool mLocked;
	EmuTCPServer* mTCPServer;
	LoginServerConnection* mLoginServerConnection;
	UCSConnection* mUCSConnection;
	EQStreamFactory* mStreamFactory;
	EQStreamIdentifier* mStreamIdentifier;
	ZoneManager* mZoneManager;
	AccountManager* mAccountManager;
	DataStore* mDataStore;

	std::list<WorldClientConnection*> mClients;
};