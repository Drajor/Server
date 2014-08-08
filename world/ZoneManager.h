#pragma once

#include "ClientAuthentication.h"
#include <list>
#include "../common/types.h"
#include <cstdint>

class WorldClientConnection;
class Zone;
class GroupManager;
class GuildManager;
class RaidManager;
class DataStore;
class World;
class Character;
struct WhoFilter;

struct ZoneSearchEntry {
	ZoneSearchEntry() : mName(""), mID(0), mInstanceID(0), mNumCharacters(0) {};
	std::string mName;
	std::uint32_t mID;
	std::uint32_t mInstanceID;
	std::uint32_t mNumCharacters;
};
typedef std::list<ZoneSearchEntry> ZoneSearchResult;

class ZoneManager {
public:
	ZoneManager(World* pWorld, DataStore* pDataStore);
	~ZoneManager();
	ZoneSearchResult getAllZones();

	void addAuthentication(ClientAuthentication& pAuthentication, std::string pCharacterName, uint32 pZoneID, uint32 pInstanceID = 0);
	
	void registerZoneTransfer(Character* pCharacter, uint16 pZoneID, uint16 pInstanceID);
	Character* getZoningCharacter(std::string pCharacterName);

	void initialise();
	void update();
	uint16 getZonePort(uint32 pZoneID, uint32 pInstanceID = 0);
	void notifyCharacterChatTell(Character* pCharacter, const std::string& pTargetName, const std::string& pMessage);
	void notifyCharacterZoneOut(Character* pCharacter);
	void whoAllRequest(Character* pCharacter, WhoFilter& pFilter);
	Character* findCharacter(const std::string pCharacterName, bool pIncludeZoning = false, Zone* pExcludeZone = nullptr);
private:
	Zone* _makeZone(uint32 pZoneID, uint32 pInstanceID = 0);
	uint32 _getNextZonePort();
	

	World* mWorld;
	GroupManager* mGroupManager;
	GuildManager* mGuildManager;
	RaidManager* mRaidManager;
	DataStore* mDataStore;
	std::list<uint32> mAvailableZonePorts;
	std::list<Zone*> mZones;
	std::list<Character*> mZoningCharacters;
};