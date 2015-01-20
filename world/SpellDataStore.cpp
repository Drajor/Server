#include "SpellDataStore.h"
#include "IDataStore.h"
#include "Data.h"
#include "Utility.h"
#include "Limits.h"

SpellDataStore::~SpellDataStore() {
	mDataStore = nullptr;

	if (mData) {
		delete[] mData;
		mData = nullptr;
	}

	if (mLog) {
		delete mLog;
		mLog = nullptr;
	}
}

const bool SpellDataStore::initialise(IDataStore* pDataStore, ILogFactory* pLogFactory) {
	if (mInitialised) return false;
	if (!pDataStore) return false;
	if (!pLogFactory) return false;

	mDataStore = pDataStore;
	mLog = pLogFactory->make();

	mLog->setContext("[SpellDataStore]");
	mLog->status("Initialising.");

	// Allocate block of memory for spells.
	mData = new Data::Spell[Limits::Spells::MAX_SPELL_ID];

	// Load data.
	u32 numSpellsLoaded = 0;
	if (!mDataStore->loadSpells(mData, numSpellsLoaded)){
		mLog->error("Failed to load data.");
		return false;
	}
	mLog->info("Loaded data for " + toString(numSpellsLoaded) + " Spells.");

	mLog->status("Finished initialising.");
	mInitialised = true;
	return true;
}

const Data::Spell* SpellDataStore::getData(const u16 pSpellID) {
	// Guard.
	if (!Limits::Spells::spellIDValid(pSpellID)) {
		mLog->error("Out of range SpellID " + toString(pSpellID) + " requested");
		return nullptr;
	}
	
	return &mData[pSpellID];
}

namespace Spell {
	const bool canClassUse(const Data::Spell* pSpell, const u8 pClassID, const u8 pLevel) {
		EXPECTED_BOOL(pSpell);
		EXPECTED_BOOL(Limits::Character::classID(pClassID));
		return pSpell->mRequiredClassLevels[pClassID] <= pLevel;
	}

	const bool zoneAllowed(const Data::Spell* pSpell, const Zone* pZone) {
		EXPECTED_BOOL(pSpell);
		EXPECTED_BOOL(pZone);
		// TODO:
		return true;
	}

}