#include "DataValidation.h"
#include "ServiceLocator.h"
#include "Data.h"
#include "Utility.h"
#include "Limits.h"
#include "Profile.h"
#include "ItemDataStore.h"
#include "AlternateCurrencyManager.h"
#include "NPCFactory.h"
#include "ShopDataStore.h"
#include "ZoneData.h"

const bool validateShopData();
const bool validateTransmutationData();
const bool validateAlternateCurrencies();
const bool validateNPCTypes();
const bool validateZoneData();

const bool validateData() {
	Profile p("validateData");
	Log::info("[Data Validation] Validation starting.");

	EXPECTED_BOOL(validateShopData());
	EXPECTED_BOOL(validateTransmutationData());
	EXPECTED_BOOL(validateAlternateCurrencies());
	EXPECTED_BOOL(validateNPCTypes());
	EXPECTED_BOOL(validateZoneData());
	
	Log::info("[Data Validation] Validation finished.");
	return true;
}

const bool validateShopData() {
	Profile p("validateShopData");

	auto shopData = ServiceLocator::getShopDataStore()->getShopData();
	for (auto i : shopData) {
		for (auto j : i->mItems) {
			// Check: Item exists.
			EXPECTED_BOOL(ServiceLocator::getItemDataStore()->get(j.first));
			// Check: Quantity is valid.
			EXPECTED_BOOL(Limits::Shop::quantityValid(j.second));
		}
	}

	return true;
}

const bool validateTransmutationData() {
	Profile p("validateTransmutationData");

	return true;
}

const bool validateAlternateCurrencies() {
	Profile p("validateAlternateCurrencies");

	auto alternateCurrencies = ServiceLocator::getAlternateCurrencyManager()->getCurrencies();
	for (auto i : alternateCurrencies) {
		// Check: Item exists.
		EXPECTED_BOOL(ServiceLocator::getItemDataStore()->get(i->mItemID));

		// Do I care about other attributes? Duplicate IDs?
	}

	return true;
}

const bool validateNPCTypes() {
	Profile p("validateNPCTypes");

	auto npcTypes = ServiceLocator::getNPCFactory()->getNPCTypes();
	for (auto i : npcTypes) {
		// Check: Appearance exists.
		EXPECTED_BOOL(ServiceLocator::getNPCFactory()->findAppearance(i->mAppearanceID));

		// Special checks for merchant type.
		if (i->mClass == ClassID::Merchant) {
			// Check: Has shop id set.
			EXPECTED_BOOL(i->mShopID != 0);
			// Check: Shop data exists.
			EXPECTED_BOOL(ServiceLocator::getShopDataStore()->getShopData(i->mShopID));
		}
	}

	return true;
}

const bool validateZoneData() {
	Profile p("validateZoneData");

	auto zoneData = ServiceLocator::getZoneDataManager()->getData();
	for (auto i : zoneData) {
		// Spawn Groups.
		for (auto j : i->mSpawnGroups) {
			for (auto k : j->mEntries) {
				// Check: NPCType exists.
				EXPECTED_BOOL(ServiceLocator::getNPCFactory()->findType(k->mNPCType));
			}
		}

		// Spawn Points.
		for (auto j : i->mSpawnPoints) {
			// TODO: Find SpawnGroup associated with SpawnPoint.
		}
	}
	return true;
}