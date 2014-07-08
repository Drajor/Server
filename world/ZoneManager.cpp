#include "ZoneManager.h"
#include "Zone.h"
#include "DataStore.h"

ZoneManager::ZoneManager(DataStore* pDataStore) :mDataStore(pDataStore) { }

void ZoneManager::update() {
	for (auto& i : mZones) {
		i->update();
	}
}

void ZoneManager::clientConnect(WorldClientConnection* pClient)
{
}
