#include "LootAllocator.h"
#include "ServiceLocator.h"
#include "NPC.h"
#include "Item.h"
#include "ItemGenerator.h"
#include "Random.h"

void LootAllocator::allocate(NPC* pNPC) {
	EXPECTED(pNPC);

	_allocateItems(pNPC);
	_allocateCurrency(pNPC);
}

void LootAllocator::_allocateItems(NPC* pNPC) {
	uint32 numItemsAllocated = 0;
	
	// Roll for max allocations.
	for (uint32 i = 0; i < mMaximumItems; i++) {
		if (Random::roll(mDropRate)) {
			numItemsAllocated++;
			Item* item = ServiceLocator::getItemGenerator()->makeRandom(pNPC->getLevel(), Rarity::Common);
			pNPC->addLootItem(item);
		}
	}

	// Bring allocation up to the minimum if required.
	while (numItemsAllocated < mMinimumItems) {
		Item* item = ServiceLocator::getItemGenerator()->makeRandom(pNPC->getLevel(), Rarity::Common);
		pNPC->addLootItem(item);
		numItemsAllocated++;
	}

	EXPECTED(numItemsAllocated >= mMinimumItems && numItemsAllocated <= mMinimumItems);
}

void LootAllocator::_allocateCurrency(NPC* pNPC) {

}
