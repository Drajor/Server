#pragma once

#include "Constants.h"
#include "ClientAuthentication.h"
#include <list>
#include <map>
#include "../common/types.h"

class EQStreamFactory;
class EQStreamIdentifier;
class DataStore;
class ZoneClientConnection;

class World;
class Character;

class Zone {
public:
	Zone(World* pWorld, DataStore* pDataStore, uint32 pPort, uint32 pZoneID, uint32 pInstanceID = 0);
	~Zone();
	bool initialise();
	void addAuthentication(ClientAuthentication& pAuthentication, std::string pCharacterName);
	void removeAuthentication(std::string pCharacterName);
	bool checkAuthentication(std::string pCharacterName);
	void addCharacter(Character* pCharacter);

	void shutdown();
	void update();

	uint32 getID() { return mID; }
	uint32 getInstanceID() { return mInstanceID; }
	uint16 getPort() { return mPort; }

	void notifyCharacterZoneIn(Character* pCharacter);
	void notifyCharacterZoneOut(Character* pCharacter);
	void notifyCharacterPositionChanged(Character* pCharacter);
	void notifyCharacterLogOut(Character* pCharacter);
	void notifyCharacterLinkDead(Character* pCharacter);
	void notifyCharacterAFK(Character* pCharacter);
	void notifyCharacterShowHelm(Character* pCharacter);
	void notifyCharacterAnonymous(Character* pCharacter);
	void notifyCharacterStanding(Character* pCharacter);
	void notifyCharacterSitting(Character* pCharacter);
	void notifyCharacterCrouching(Character* pCharacter);
	void notifyCharacterChatSay(Character* pCharacter, const std::string pMessage);
	void notifyCharacterChatShout(Character* pCharacter, const std::string pMessage);
	void notifyCharacterChatOOC(Character* pCharacter, const std::string pMessage);
	void notifyCharacterEmote(Character* pCharacter, const std::string pMessage);
	void notifyCharacterChatAuction(Character* pCharacter, const std::string pMessage);
	void notifyCharacterChatTell(Character* pCharacter, const std::string pTargetName, const std::string pMessage);
	void notifyCharacterAnimation(Character* pCharacter, uint8 pAction, uint8 pAnimationID, bool pIncludeSender = false);
	void notifyCharacterLevelIncrease(Character* pCharacter);
	void notifyCharacterGM(Character* pCharacter);

	void moveCharacter(Character* pCharacter, float pX, float pY, float pZ);
	uint16 getNextSpawnID() { return mNextSpawnID++; }
	
private:
	void _sendChat(Character* pCharacter, ChannelID pChannel, const std::string pMessage);
	void _sendTell(Character* pCharacter, const std::string pFromName, const std::string pMessage);
	void _sendSpawnAppearance(Character* pCharacter, SpawnAppearanceType pType, uint32 pParameter, bool pIncludeSender = false);
	void _handleIncomingConnections();
	











	std::map<std::string, ClientAuthentication> mAuthenticatedCharacters;


	uint16 mNextSpawnID;
	uint32 mID; // Zone ID
	uint32 mInstanceID;
	uint32 mPort;

	bool mInitialised; // Flag indicating whether the Zone has been initialised.
	EQStreamFactory* mStreamFactory;
	EQStreamIdentifier* mStreamIdentifier;
	World* mWorld;
	DataStore* mDataStore;
	std::list<Character*> mCharacters; // List of Player Characters in Zone.
	std::list<ZoneClientConnection*> mZoneClientConnections;
};