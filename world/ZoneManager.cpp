#include "ZoneManager.h"
#include "Zone.h"
#include "GroupManager.h"
#include "GuildManager.h"
#include "RaidManager.h"
#include "World.h"
#include "DataStore.h"
#include "Character.h"
#include "ZoneClientConnection.h"
#include "LogSystem.h"
#include "Utility.h"

ZoneManager::ZoneManager() { }

ZoneManager::~ZoneManager() {
	ZoneClientConnection::deinitialise();
}


void ZoneManager::update() {
	for (auto& i : mZones) {
		i->update();
	}
}

uint16 ZoneManager::getZonePort(ZoneID pZoneID, uint32 pInstanceID) {
	// Search existing zones
	for (auto i : mZones) {
		if (i->getID() == pZoneID && i->getInstanceID() == pInstanceID) {
			return i->getPort();
		}
	}

	// Zone was not found, create it.
	Zone* zone = _makeZone(pZoneID, pInstanceID);
	return zone->getPort();
}

bool ZoneManager::initialise() {
	Log::status("[Zone Manager] Initialising.");

	for (int i = 0; i < 200; i++)
		mAvailableZonePorts.push_back(7000+i);
	ZoneClientConnection::initalise();

	return true;
}

uint32 ZoneManager::_getNextZonePort() {
	uint32 port = *mAvailableZonePorts.begin();
	mAvailableZonePorts.pop_front();
	return port;
	// TODO: Error check this ;)
}

void ZoneManager::addAuthentication(ClientAuthentication& pAuthentication, String pCharacterName, ZoneID pZoneID, uint32 pInstanceID) {
	bool zoneFound = false;
	for (auto i : mZones) {
		if (i->getID() == pZoneID && i->getInstanceID() == pInstanceID) {
			i->addAuthentication(pAuthentication, pCharacterName);
			return;
		}
	}

	// Zone was not found, create it.
	Zone* zone = _makeZone(pZoneID, pInstanceID);
	zone->addAuthentication(pAuthentication, pCharacterName);
}

Zone* ZoneManager::_makeZone(ZoneID pZoneID, uint32 pInstanceID) {
	const uint32 zonePort = _getNextZonePort();
	StringStream ss; ss << "[Zone Manager] Starting new Zone on port " << zonePort << ", ZoneID: " << pZoneID << " InstanceID: " << pInstanceID;
	Log::info(ss.str());
	Zone* zone = new Zone(zonePort, pZoneID, pInstanceID);
	zone->initialise();
	mZones.push_back(zone);
	return zone;
}

void ZoneManager::whoAllRequest(Character* pCharacter, WhoFilter& pFilter) {
	std::list<Character*> matches;
	for (auto i : mZones) {
		i->getWhoMatches(matches, pFilter);
	}
	pCharacter->getConnection()->sendWhoResults(matches);
}

void ZoneManager::notifyCharacterChatTell(Character* pCharacter, const String& pTargetName, const String& pMessage) {
	// Search Zones
	for (auto i : mZones) {
		if (i->trySendTell(pCharacter->getName(), pTargetName, pMessage)) {
			// Send echo "You told Player, 'Message'
			pCharacter->getConnection()->sendSimpleMessage(MessageType::TellEcho, StringID::TELL_ECHO, pTargetName, pMessage);
			return;
		}
	}

	// Check zoning Characters
	for (auto i : mZoningCharacters) {
		if (i->getName() == pTargetName) {
			// Send queued echo "You told Player, '[queued] Message'
			pCharacter->getConnection()->sendSimpleMessage(MessageType::TellEcho, StringID::TELL_QUEUED, pTargetName, Utility::StringIDString(StringID::QUEUED), pMessage);
			// Store queued message.
			i->addQueuedMessage(ChannelID::CH_TELL, pCharacter->getName(), pMessage);
		}
	}

	// pTargetName is not online at this time.
	pCharacter->getConnection()->sendSimpleMessage(MessageType::White, StringID::PLAYER_NOT_ONLINE, pTargetName);
}

Character* ZoneManager::findCharacter(const String pCharacterName, bool pIncludeZoning, Zone* pExcludeZone) {
	Character* character = nullptr;

	// Search Zones.
	for (auto i : mZones) {
		if (i != pExcludeZone) {
			character = i->findCharacter(pCharacterName);
			if (character)
				return character;
		}
	}

	// Search zoning characters.
	if (pIncludeZoning) {
		for (auto i : mZoningCharacters) {
			if (i->getName() == pCharacterName)
				return i;
		}
	}

	return nullptr;
}

void ZoneManager::notifyCharacterZoneOut(Character* pCharacter) {
	mZoningCharacters.push_back(pCharacter);
}

void ZoneManager::registerZoneTransfer(Character* pCharacter, ZoneID pZoneID, uint16 pInstanceID) {
	ARG_PTR_CHECK(pCharacter);

	ZoneTransfer zoneTransfer;
	zoneTransfer.mCharacterName = pCharacter->getName();
	zoneTransfer.mFromZoneID = pCharacter->getZone()->getID();
	zoneTransfer.mFromInstanceID = pCharacter->getZone()->getInstanceID();
	zoneTransfer.mToZoneID = pZoneID;
	zoneTransfer.mToInstanceID = pInstanceID;

	World::getInstance().addCharacterZoneTransfer(zoneTransfer);
}

Character* ZoneManager::getZoningCharacter(String pCharacterName) {
	for (auto i : mZoningCharacters) {
		if (i->getName() == pCharacterName)
			return i;
	}

	return nullptr;
}

ZoneSearchResult ZoneManager::getAllZones() {
	ZoneSearchResult result;
	for (auto i : mZones) {
		ZoneSearchEntry entry;
		entry.mName = i->getLongName();
		entry.mID = i->getID();
		entry.mInstanceID = i->getInstanceID();
		entry.mNumCharacters = i->getNumCharacters();
		result.push_back(entry);
	}

	return result;
}
