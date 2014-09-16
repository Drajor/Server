#pragma once

#include "Constants.h"
#include "Singleton.h"
#include "ClientAuthentication.h"

class Zone;
class Character;
struct WhoFilter;

struct ZoneSearchEntry {
	String mName = "";
	ZoneID mID = ZoneIDs::NoZone;
	InstanceID mInstanceID = 0;
	uint32 mNumCharacters = 0;
};
typedef std::list<ZoneSearchEntry> ZoneSearchResult;

class ZoneManager : public Singleton<ZoneManager> {
private:
	friend class Singleton<ZoneManager>;
	ZoneManager() {};
	~ZoneManager();
	ZoneManager(ZoneManager const&); // Do not implement.
	void operator=(ZoneManager const&); // Do not implement.
public:
	bool initialise();
	const bool isZoneAvailable(const ZoneID pZoneID, const InstanceID pInstanceID);

	ZoneSearchResult getAllZones();

	const bool addAuthentication(ClientAuthentication& pAuthentication, String pCharacterName, ZoneID pZoneID, uint32 pInstanceID = 0);
	
	void registerZoneTransfer(Character* pCharacter, ZoneID pZoneID, uint16 pInstanceID);
	
	void addZoningCharacter(Character* pCharacter);
	const bool removeZoningCharacter(const String& pCharacterName);
	Character* getZoningCharacter(const String& pCharacterName);

	
	void update();
	uint16 getZonePort(ZoneID pZoneID, uint32 pInstanceID = 0);
	void notifyCharacterChatTell(Character* pCharacter, const String& pTargetName, const String& pMessage);
	void whoAllRequest(Character* pCharacter, WhoFilter& pFilter);
	Character* findCharacter(const String pCharacterName, bool pIncludeZoning = false, Zone* pExcludeZone = nullptr);
private:
	Zone* _search(const ZoneID pZoneID, const uint32 pInstanceID);
	const bool _makeZone(Zone* pZone, const ZoneID pZoneID, const uint32 pInstanceID = 0);
	const uint32 _getNextZonePort();
	std::list<uint32> mAvailableZonePorts;
	std::list<Zone*> mZones;
	std::list<Character*> mZoningCharacters;
};