#include "Inventory.h"
#include "Item.h"
#include "ItemData.h"
#include "Data.h"
#include "Character.h"
#include "Utility.h"
#include "ItemGenerator.h"
#include "ItemFactory.h"
#include "Limits.h"

Inventoryy::Inventoryy() {
	for (auto& i : mItems) i = nullptr;
	for (auto& i : mBank) i = nullptr;
	for (auto& i : mSharedBank) i = nullptr;
	for (auto& i : mTrade) i = nullptr;

	memset(mCurrency, 0, sizeof(mCurrency));

	//put(ItemGenerator::makeDice("Yera"), SlotID::POWER_SOURCE);

	//auto container = ItemFactory::make(ItemID::StartContainer);
	//container->setContents(ItemFactory::make(ItemID::StartFood, 10000), 8);
	//container->setContents(ItemFactory::make(ItemID::StartDrink, 10000), 9);
	//container->setContents(ItemGenerator::makeTwoHandBlunt(1, Rarity::Common), 0);
	//put(container, SlotID::MAIN_7);
	//container->updateContentsSlots();

	//pushCursor(ItemGenerator::makeTwoHandBlunt(1, Rarity::Common));
	//pushCursor(ItemGenerator::makeOneHandBlunt(1, Rarity::Common));
}

const unsigned char* Inventoryy::getData(uint32& pSize) {
	unsigned char * data = nullptr;
	uint32 numItems = 0;

	// Calculate the number of Items and amount of memory required to send.

	// Calc: Iterates a slot range, counting number of items and data size.
	auto calc = [this, &numItems, &pSize](const uint32 pMinSlotID, const uint32 pMaxSlotID) {
		for (auto i = pMinSlotID; i <= pMaxSlotID; i++) {
			auto item = getItem(i);
			if (item) {
				numItems++;
				pSize += item->getDataSize(Payload::ItemPacketCharInventory);
			}
		}
	};

	// Worn Items.
	calc(SlotID::CHARM, SlotID::AMMO);
	
	// Main Items.
	calc(SlotID::MAIN_0, SlotID::MAIN_7);

	// Cursor.
	calc(SlotID::CURSOR, SlotID::CURSOR); // NOTE: Only the first Item on cursor is sent here.

	// Bank Items
	calc(SlotID::BANK_0, SlotID::BANK_23);

	// Shared Bank Items.
	calc(SlotID::SHARED_BANK_0, SlotID::SHARED_BANK_1);

	// Allocate the required memory.
	pSize += sizeof(uint32); // Item Count
	data = new unsigned char[pSize];

	Utility::DynamicStructure ds(data, pSize);
	ds.write<uint32>(numItems);

	// Copy: Iterates a slot range and copies Item data.
	auto copy = [this, &ds](const uint32 pMinSlotID, const uint32 pMaxSlotID) {
		for (auto i = pMinSlotID; i <= pMaxSlotID; i++) {
			auto item = getItem(i);
			if (item)
				item->copyData(ds, Payload::ItemPacketCharInventory);
		}
	};

	// Worn Items.
	copy(SlotID::CHARM, SlotID::AMMO);

	// Main Items.
	copy(SlotID::MAIN_0, SlotID::MAIN_7);

	// Cursor.
	copy(SlotID::CURSOR, SlotID::CURSOR);

	// Bank Items
	copy(SlotID::BANK_0, SlotID::BANK_23);

	// Shared Bank Items.
	copy(SlotID::SHARED_BANK_0, SlotID::SHARED_BANK_1);

	// Check: The amount of data written matches what was calculated.
	if (ds.check() == false) {
		Log::error("[Inventory] Bad Write: Written: " + std::to_string(ds.getBytesWritten()) + " Size: " + std::to_string(ds.getSize()));
	}

	return data;
}

Item* Inventoryy::getItem(const uint32 pSlot) const {
	// Getting: Cursor
	if (SlotID::isCursor(pSlot)) {
		Item* item = mCursor.empty() ? nullptr : mCursor.front();
		if (item) EXPECTED_BOOL(item->getSlot() == pSlot); // Testing / Debug.
		return item;
	}

	// Getting: Main Inventory or Worn.
	if (SlotID::isMainInventory(pSlot) || SlotID::isWorn(pSlot)) {
		Item* item = mItems[pSlot];
		if (item) EXPECTED_BOOL(item->getSlot() == pSlot); // Testing / Debug.
		return item;
	}

	// Getting: Main Contents.
	if (SlotID::isMainContents(pSlot)) {
		const uint32 parentSlotID = SlotID::getParentSlot(pSlot);
		Item* parentContainer = getItem(parentSlotID);
		EXPECTED_PTR(parentContainer);
		const uint32 subIndex = SlotID::getSubIndex(pSlot);
		Item* item = parentContainer->getContents(subIndex);
		if (item) EXPECTED_BOOL(item->getSlot() == pSlot); // Testing / Debug.
		return item;
	}

	// Getting: Bank.
	if (SlotID::isBank(pSlot)) {
		const uint32 index = SlotID::getIndex(pSlot);
		EXPECTED_PTR(SlotID::isValidBankIndex(index));
		Item* item = mBank[index];
		if (item) EXPECTED_BOOL(item->getSlot() == pSlot); // Testing / Debug.
		return item;
	}

	// Getting: Bank Contents.
	if (SlotID::isBankContents(pSlot)) {
		const uint32 parentSlotID = SlotID::getParentSlot(pSlot);
		Item* parentContainer = getItem(parentSlotID);
		EXPECTED_PTR(parentContainer);
		const uint32 subIndex = SlotID::getSubIndex(pSlot);
		Item* item = parentContainer->getContents(subIndex);
		if (item) EXPECTED_BOOL(item->getSlot() == pSlot); // Testing / Debug.
		return item;
	}

	// Getting: Shared Bank.
	if (SlotID::isSharedBank(pSlot)) {
		const uint32 index = SlotID::getIndex(pSlot);
		EXPECTED_PTR(SlotID::isValidSharedBankIndex(index));
		Item* item = mSharedBank[index];
		if (item) EXPECTED_BOOL(item->getSlot() == pSlot); // Testing / Debug.
		return item;
	}

	// Getting: Shared Bank Contents.
	if (SlotID::isSharedBankContents(pSlot)) {
		const uint32 parentSlotID = SlotID::getParentSlot(pSlot);
		Item* parentContainer = getItem(parentSlotID);
		EXPECTED_PTR(parentContainer);
		const uint32 subIndex = SlotID::getSubIndex(pSlot);
		Item* item = parentContainer->getContents(subIndex);
		if (item) EXPECTED_BOOL(item->getSlot() == pSlot); // Testing / Debug.
		return item;
	}

	// Getting: Trade.
	if (SlotID::isTrade(pSlot)) {
		const uint32 index = SlotID::getIndex(pSlot);
		EXPECTED_PTR(SlotID::isValidTradeIndex(index));
		Item* item = mTrade[index];
		if (item) EXPECTED_BOOL(item->getSlot() == pSlot); // Testing / Debug.
		return item;
	}

	return nullptr;
}

const bool Inventoryy::put(Item* pItem, const uint32 pSlot) {
	EXPECTED_BOOL(pItem);
	Log::info("[Inventory] PUT " + pItem->getName() + " to " + std::to_string(pSlot));

	// Putting item on cursor.
	if (SlotID::isCursor(pSlot)) {
		mCursor.push_front(pItem);
		pItem->setSlot(pSlot);
		return true;
	}

	// Putting item in Main Inventory or Worn.
	if (SlotID::isMainInventory(pSlot) || SlotID::isWorn(pSlot)) {
		Item* existingItem = getItem(pSlot);
		EXPECTED_BOOL(existingItem == nullptr); // Failure = desync.
		pItem->setSlot(pSlot);
		mItems[pSlot] = pItem;
		return true;
	}

	// Putting: Bank.
	if (SlotID::isBank(pSlot)) {
		Item* existingItem = getItem(pSlot);
		EXPECTED_BOOL(existingItem == nullptr); // Failure = desync.
		pItem->setSlot(pSlot);

		const uint32 index = SlotID::getIndex(pSlot);
		EXPECTED_BOOL(SlotID::isValidBankIndex(index));
		mBank[index] = pItem;
		return true;
	}

	// Putting: Shared Bank.
	if (SlotID::isSharedBank(pSlot)) {
		// Check: Only tradeable Items can be stored in the Shared bank.
		EXPECTED_BOOL(pItem->isTradeable());
		Item* existingItem = getItem(pSlot);
		EXPECTED_BOOL(existingItem == nullptr); // Failure = desync.
		pItem->setSlot(pSlot);

		const uint32 index = SlotID::getIndex(pSlot);
		EXPECTED_BOOL(SlotID::isValidSharedBankIndex(index));
		mSharedBank[index] = pItem;
		return true;
	}

	// Putting: Trade.
	if (SlotID::isTrade(pSlot)) {
		// TODO: Not sure whether to check that the Item is tradeable here or not. I would need to be exposed to the trading context ><
		Item* existingItem = getItem(pSlot);
		EXPECTED_BOOL(existingItem == nullptr); // Failure = desync.
		pItem->setSlot(pSlot);

		const uint32 index = SlotID::getIndex(pSlot);
		EXPECTED_BOOL(SlotID::isValidTradeIndex(index));
		mTrade[index] = pItem;
		return true;
	}

	// Putting: Main Contents.
	if (SlotID::isMainContents(pSlot)) {
		EXPECTED_BOOL(putContainer(pItem, pSlot));
		EXPECTED_BOOL(pItem->getSlot() == pSlot); // Testing / Debug.
		return true;
	}

	// Putting: Bank Contents.
	if (SlotID::isBankContents(pSlot)) {
		EXPECTED_BOOL(putContainer(pItem, pSlot));
		EXPECTED_BOOL(pItem->getSlot() == pSlot); // Testing / Debug.
		return true;
	}

	// Putting: Shared Bank Contents.
	if (SlotID::isSharedBankContents(pSlot)) {
		EXPECTED_BOOL(putContainer(pItem, pSlot));
		EXPECTED_BOOL(pItem->getSlot() == pSlot); // Testing / Debug.
		return true;
	}

	return false;
}

const bool Inventoryy::putContainer(Item* pItem, const uint32 pSlot) {
	EXPECTED_BOOL(pItem);

	// Determine the parent slot.
	const uint32 parentSlotID = SlotID::getParentSlot(pSlot);
	// Check: Parent is a valid Item.
	Item* parentContainer = getItem(parentSlotID);
	EXPECTED_BOOL(parentContainer);
	// Check: Parent is a valid container.
	EXPECTED_BOOL(parentContainer->isContainer());
	const uint32 subIndex = SlotID::getSubIndex(pSlot);
	// Check: Index is valid for container size.
	EXPECTED_BOOL(parentContainer->getContainerSlots() > subIndex);
	// Check: No existing Item where pItem is being put
	EXPECTED_BOOL(parentContainer->getContents(subIndex) == nullptr);
	// Finally try and put the Item inside the container.
	EXPECTED_BOOL(parentContainer->setContents(pItem, subIndex));

	return true;
}

const bool Inventoryy::moveItem(const uint32 pFromSlot, const uint32 pToSlot, const uint32 pStackSize) {

	// NOTE: The client sends this when an item is summoned to their cursor.
	if (pFromSlot == pToSlot) {
		// Debug logging until I understand any other circumstances this can occur in.
		Log::info("MoveItem: From: " + std::to_string(pFromSlot) + " To: " + std::to_string(pToSlot) + " Stack: " + std::to_string(pStackSize));
		return true;
	}

	// Deleting an Item.
	if (SlotID::isDelete(pToSlot)) {
		Log::info("[Inventory] DELETE");
		// NOTE: As far as I know the only possible slot to delete from is cursor.
		EXPECTED_BOOL(SlotID::isCursor(pFromSlot));
		Item* itemFrom = _popCursor();
		EXPECTED_BOOL(itemFrom); // Failure = desync
		EXPECTED_BOOL(SlotID::isCursor(itemFrom->getSlot())); // Failure = bug

		delete itemFrom;
		return true;
	}


	// Putting an Item down.
	if (SlotID::isCursor(pFromSlot)) {
		EXPECTED_BOOL(_putDown(pToSlot, pStackSize));
		return true;
	}

	// Picking an Item up.
	if (SlotID::isCursor(pToSlot)) {
		EXPECTED_BOOL(_pickUp(pFromSlot, pStackSize));
		return true;
	}

	return false;
}

Item* Inventoryy::_popCursor() {
	EXPECTED_PTR(!mCursor.empty());
	Item* item = mCursor.front();
	mCursor.pop_front();
	return item;
}

Item* Inventoryy::_peekCursor() const {
	if (isCursorEmpty()) return nullptr;
	return mCursor.front();
}

const bool Inventoryy::pushCursor(Item* pItem) {
	EXPECTED_BOOL(pItem);
	mCursor.push_back(pItem);
	pItem->setSlot(SlotID::CURSOR);
	return true;
}

const bool Inventoryy::consume(const uint32 pSlot, const uint32 pStacks) {
	// NOTE: UF will only consume from the Main Inventory

	auto item = getItem(pSlot);
	EXPECTED_BOOL(item);

	// Consuming the whole Item.
	if (item->getStacks() == pStacks || !item->isStackable()) {
		EXPECTED_BOOL(_clear(pSlot));
		delete item;
		item = nullptr;
	}
	// Consuming stacks.
	else {
		EXPECTED_BOOL(item->removeStacks(pStacks));
	}

	return true;
}

void Inventoryy::_calculateAdd(Item* pItem) {
	EXPECTED(pItem);

	_addStrength(pItem->_getStrength());
	_addStamina(pItem->_getStamina());
	_addIntelligence(pItem->_getIntelligence());
	_addWisdom(pItem->_getWisdom());
	_addAgility(pItem->_getAgility());
	_addDexterity(pItem->_getDexterity());
	_addCharisma(pItem->_getCharisma());

	_addHeroicStrength(pItem->_getHeroicStrength());
	_addHeroicStamina(pItem->_getHeroicStamina());
	_addHeroicIntelligence(pItem->_getHeroicIntelligence());
	_addHeroicWisdom(pItem->_getHeroicWisdom());
	_addHeroicAgility(pItem->_getHeroicAgility());
	_addHeroicDexterity(pItem->_getHeroicDexterity());
	_addHeroicCharisma(pItem->_getHeroicCharisma());

	_addMagicResist(pItem->_getMagicResist());
	_addFireResist(pItem->_getFireResist());
	_addColdResist(pItem->_getColdResist());
	_addDiseaseResist(pItem->_getDiseaseResist());
	_addPoisonResist(pItem->_getPoisonResist());
	_addCorruptionResist(pItem->getCorruptionResist());

	_addHeroicMagicResist(pItem->_getHeroicMagicResist());
	_addHeroicFireResist(pItem->_getHeroicFireResist());
	_addHeroicColdResist(pItem->_getHeroicColdResist());
	_addHeroicDiseaseResist(pItem->_getHeroicDiseaseResist());
	_addHeroicPoisonResist(pItem->_getHeroicPoisonResist());
	_addHeroicCorruptionResist(pItem->_getHeroicCorruptionResist());

	_addHealth(pItem->_getHealth());
	_addMana(pItem->_getMana());
	_addEndurance(pItem->_getEndurance());

	_addHealthRegen(pItem->_getHealthRegen());
	_addManaRegen(pItem->_getManaRegen());
	_addEnduranceRegen(pItem->_getEnduranceRegen());

	// TODO: MOD2
}

void Inventoryy::_calculateRemove(Item* pItem) {
	EXPECTED(pItem);

	_removeStrength(pItem->_getStrength());
	_removeStamina(pItem->_getStamina());
	_removeIntelligence(pItem->_getIntelligence());
	_removeWisdom(pItem->_getWisdom());
	_removeAgility(pItem->_getAgility());
	_removeDexterity(pItem->_getDexterity());
	_removeCharisma(pItem->_getCharisma());

	_removeHeroicStrength(pItem->_getHeroicStrength());
	_removeHeroicStamina(pItem->_getHeroicStamina());
	_removeHeroicIntelligence(pItem->_getHeroicIntelligence());
	_removeHeroicWisdom(pItem->_getHeroicWisdom());
	_removeHeroicAgility(pItem->_getHeroicAgility());
	_removeHeroicDexterity(pItem->_getHeroicDexterity());
	_removeHeroicCharisma(pItem->_getHeroicCharisma());

	_removeMagicResist(pItem->_getMagicResist());
	_removeFireResist(pItem->_getFireResist());
	_removeColdResist(pItem->_getColdResist());
	_removeDiseaseResist(pItem->_getDiseaseResist());
	_removePoisonResist(pItem->_getPoisonResist());
	_removeCorruptionResist(pItem->getCorruptionResist());

	_removeHeroicMagicResist(pItem->_getHeroicMagicResist());
	_removeHeroicFireResist(pItem->_getHeroicFireResist());
	_removeHeroicColdResist(pItem->_getHeroicColdResist());
	_removeHeroicDiseaseResist(pItem->_getHeroicDiseaseResist());
	_removeHeroicPoisonResist(pItem->_getHeroicPoisonResist());
	_removeHeroicCorruptionResist(pItem->_getHeroicCorruptionResist());

	_removeHealth(pItem->_getHealth());
	_removeMana(pItem->_getMana());
	_removeEndurance(pItem->_getEndurance());

	_removeHealthRegen(pItem->_getHealthRegen());
	_removeManaRegen(pItem->_getManaRegen());
	_removeEnduranceRegen(pItem->_getEnduranceRegen());
}

void Inventoryy::updateConsumables() {
	Item* food = findFirst(ItemType::Food);
	
	// There was already food.
	if (mAutoFood) {
		// Now there is no food.
		if (!food) {
			_calculateRemove(mAutoFood);
			mAutoFood = nullptr;
		}
		// There is new food.
		else if (mAutoFood != food) {
			_calculateRemove(mAutoFood);
			mAutoFood = food;
			_calculateAdd(mAutoFood);
		}
	}
	// There was no food, now there is.
	else if(food) {
		mAutoFood = food;
		_calculateAdd(mAutoFood);
	}

	Item* drink = findFirst(ItemType::Drink);
	
	// There was already drink.
	if (mAutoDrink) {
		// Now there is no drink.
		if (!drink) {
			_calculateRemove(mAutoDrink);
			mAutoDrink = nullptr;
		}
		// There is new drink.
		else if (mAutoDrink != drink) {
			_calculateRemove(mAutoDrink);
			mAutoDrink = drink;
			_calculateAdd(mAutoDrink);
		}
	}
	// There was no drink, now there is.
	else if (drink) {
		mAutoDrink = drink;
		_calculateAdd(mAutoDrink);
	}
}

Item* Inventoryy::findFirst(const uint8 pItemType) const {
	// Search: Main Inventory slots first.
	for (int i = SlotID::MAIN_0; i <= SlotID::MAIN_7; i++) {
		if (mItems[i] && mItems[i]->getItemType() == pItemType)
			return mItems[i];
	}

	// Search: Slots inside each primary slot.
	for (int i = SlotID::MAIN_0; i <= SlotID::MAIN_7; i++) {
		if (mItems[i] && mItems[i]->isContainer()) {
			Item* item = mItems[i]->findFirst(pItemType);
			if (item) {
				return item;
			}
		}
	}
	return nullptr;
}

Item* Inventoryy::findStackable(const uint32 pItemID) const {
	// Search: Main slots.
	for (int i = SlotID::MAIN_0; i <= SlotID::MAIN_7; i++) {
		auto item = mItems[i];
		if (item && item->getID() == pItemID && item->getStacks() < item->getMaxStacks())
			return item;
	}

	// Search: Containers in main slots.
	for (int i = SlotID::MAIN_0; i <= SlotID::MAIN_7; i++) {
		auto container = mItems[i];
		if (container && container->isContainer()) {
			Item* item = container->findStackable(pItemID);
			if (item) {
				return item;
			}
		}
	}
	return nullptr;
}

const bool Inventoryy::_clear(const uint32 pSlot) {
	// Clearing: Cursor
	// NOTE: Only the first Item is removed.
	if (SlotID::isCursor(pSlot)) {
		EXPECTED_BOOL(_popCursor());
		return true;
	}
	if (SlotID::isMainInventory(pSlot) || SlotID::isWorn(pSlot)) {
		mItems[pSlot] = nullptr;
		return true;
	}
	// Clearing: Bank slot.
	if (SlotID::isBank(pSlot)) {
		uint32 index = SlotID::getIndex(pSlot);
		EXPECTED_BOOL(SlotID::isValidBankIndex(index));
		mBank[index] = nullptr;
		return true;
	}
	// Clearing: Shared Bank slot.
	if (SlotID::isSharedBank(pSlot)) {
		uint32 index = SlotID::getIndex(pSlot);
		EXPECTED_BOOL(SlotID::isValidSharedBankIndex(index));
		mSharedBank[index] = nullptr;
		return true;
	}
	// Clearing: Trade slot.
	if (SlotID::isTrade(pSlot)) {
		uint32 index = SlotID::getIndex(pSlot);
		EXPECTED_BOOL(SlotID::isValidTradeIndex(index));
		mTrade[index] = nullptr;
		return true;
	}

	// Clearing: Main Contents.
	if (SlotID::isMainContents(pSlot)) {
		EXPECTED_BOOL(_clearContainerSlot(pSlot));
		return true;
	}

	// Clearing: Bank Contents.
	if (SlotID::isBankContents(pSlot)) {
		EXPECTED_BOOL(_clearContainerSlot(pSlot));
		return true;
	}

	// Clearing: Shared Bank Contents.
	if (SlotID::isSharedBankContents(pSlot)) {
		EXPECTED_BOOL(_clearContainerSlot(pSlot));
		return true;
	}

	return false;
}

const bool Inventoryy::_clearContainerSlot(const uint32 pSlot) {
	const uint32 parentSlot = SlotID::getParentSlot(pSlot);
	const uint32 subIndex = SlotID::getSubIndex(pSlot);

	if (SlotID::isMainContents(pSlot)) EXPECTED_BOOL(SlotID::isMainInventory(parentSlot));
	if (SlotID::isBankContents(pSlot)) EXPECTED_BOOL(SlotID::isBank(parentSlot));
	if (SlotID::isSharedBankContents(pSlot)) EXPECTED_BOOL(SlotID::isSharedBank(parentSlot));

	EXPECTED_BOOL(SlotID::subIndexValid(subIndex));

	Item* container = getItem(parentSlot);
	EXPECTED_BOOL(container);
	EXPECTED_BOOL(container->isContainer());
	EXPECTED_BOOL(container->getContainerSlots() > subIndex);
	EXPECTED_BOOL(container->clearContents(subIndex));

	return true;
}

const bool Inventoryy::_putDown(const uint32 pToSlot, const uint32 pStackSize) {

	// Check: Existing Item in pToSlot.
	Item* existing = getItem(pToSlot);
	if (existing) {
		// Potential stack merge.
		if (pStackSize > 0) {
			Item* cursorItem = _peekCursor();
			EXPECTED_BOOL(cursorItem); // Failure = desync.
			EXPECTED_BOOL(cursorItem->isStackable());

			// Check: Both Items are the same.
			if (cursorItem->getID() == existing->getID()) {
				EXPECTED_BOOL(_stackMergeCursor(pToSlot, pStackSize));
				return true;
			}
		}
	}
	
	// Pop cursor head into limbo.
	Item* cursorItem = _popCursor();
	EXPECTED_BOOL(cursorItem); // Failure = desync.

	// Pick up the existing Item. (Push cursor)
	if (existing)
		EXPECTED_BOOL(_pickUp(pToSlot, 0)); // pStackSize is set to 0 because 0 means the whole Item.

	EXPECTED_BOOL(put(cursorItem, pToSlot));

	// Check: Worn Item being equipped.
	if (SlotID::isWorn(pToSlot)) {
		// Set Item to Attuned where it is Attunable.
		if (cursorItem->isAttunable())
			// TODO: Attuned augments in cursorItem.
			cursorItem->setIsAttuned(true);

		_calculateAdd(cursorItem);
	}

	// Check: Container being put down.
	if (cursorItem->isContainer())
		cursorItem->updateContentsSlots();

	return true;
}

const bool Inventoryy::_stackMergeCursor(const uint32 pToSlot, const uint32 pStackSize) {

	Item* cursorItem = _peekCursor();
	EXPECTED_BOOL(cursorItem); // Failure = desync.
	EXPECTED_BOOL(cursorItem->isStackable());

	Item* existing = getItem(pToSlot);
	EXPECTED_BOOL(existing);
	EXPECTED_BOOL(cursorItem->getID() == existing->getID());

	const uint32 preMergeStacks = cursorItem->getStacks() + existing->getStacks();

	// Full merge.
	if (pStackSize == cursorItem->getStacks()) {

		// Add stacks from cursor to the existing Item.
		EXPECTED_BOOL(existing->addStacks(pStackSize));

		// Delete cursor Item as it has been consumed in the full stack merge.
		_popCursor();
		delete cursorItem;
		cursorItem = nullptr;

		// Check: The total number of stacks remains the same.
		EXPECTED_BOOL(existing->getStacks() == preMergeStacks);

		return true;
	}
	// Partial merge.
	else {
		// Remove stacks from cursor.
		EXPECTED_BOOL(cursorItem->removeStacks(pStackSize));
		// Add stacks from cursor to the existing Item.
		EXPECTED_BOOL(existing->addStacks(pStackSize));

		// Check: The total number of stacks remains the same.
		EXPECTED_BOOL(existing->getStacks() + cursorItem->getStacks() == preMergeStacks);

		return true;
	}
}

const bool Inventoryy::_pickUp(const uint32 pFromSlot, const uint32 pStackSize) {
	Item* pickUp = getItem(pFromSlot);
	EXPECTED_BOOL(pickUp);

	// Splitting the stack.
	if (pStackSize > 0) {
		EXPECTED_BOOL(pickUp->isStackable());
		const uint32 preSplitStacks = pickUp->getStacks();
		EXPECTED_BOOL(pickUp->getStacks() > pStackSize);
		EXPECTED_BOOL(pickUp->removeStacks(pStackSize));

		Item* split = pickUp->copy();
		split->setStacks(pStackSize);

		// Check: The total number of stacks remains the same.
		EXPECTED_BOOL(preSplitStacks == (pickUp->getStacks() + split->getStacks()));

		EXPECTED_BOOL(pushCursor(split));
		return true;
	}

	// Regular pick up.
	EXPECTED_BOOL(pushCursor(pickUp));
	EXPECTED_BOOL(_clear(pFromSlot));
	pickUp->clearParent();
	pickUp->clearSubIndex();

	// Check: Worn Item being un-equipped.
	if (SlotID::isWorn(pFromSlot))
		_calculateRemove(pickUp);

	return true;
}

Item* Inventoryy::find(const uint32 pItemID, const uint32 pInstanceID) const {

	// Search Worn Slots
	for (int i = SlotID::CHARM; i <= SlotID::AMMO; i++) {
		if (mItems[i] && mItems[i]->getID() == pItemID && mItems[i]->getInstanceID() == pInstanceID)
			return mItems[i];
	}

	// Search Primary Main.
	for (int i = SlotID::MAIN_0; i <= SlotID::MAIN_7; i++) {
		if (mItems[i] && mItems[i]->getID() == pItemID && mItems[i]->getInstanceID() == pInstanceID) {
			return mItems[i];
		}
		// Search Primary Contents.
		else if (mItems[i] && mItems[i]->isContainer()) {
			for (int j = 0; j < SlotID::MAX_CONTENTS; j++) {
				if (mItems[j] && mItems[j]->getID() == pItemID && mItems[j]->getInstanceID() == pInstanceID) {
					return mItems[j];
				}
			}
		}
	}

	return nullptr;
}

const bool Inventoryy::removeRadiantCrystals(const uint32 pCrystals) {
	EXPECTED_BOOL(mRadiantCrystals >= pCrystals);

	mRadiantCrystals -= pCrystals;
	return true;
}

const bool Inventoryy::removeEbonCrystals(const uint32 pCrystals) {
	EXPECTED_BOOL(mEbonCrystals >= pCrystals);

	mEbonCrystals -= pCrystals;
	return true;
}

const bool Inventoryy::moveCurrency(const uint32 pFromSlot, const uint32 pToSlot, const uint32 pFromType, const uint32 pToType, const int32 pAmount) {
	EXPECTED_BOOL(Limits::General::moneySlotIDValid(pFromSlot));
	EXPECTED_BOOL(Limits::General::moneySlotIDValid(pToSlot));
	EXPECTED_BOOL(Limits::General::moneyTypeValid(pFromType));
	EXPECTED_BOOL(Limits::General::moneyTypeValid(pToType));
	EXPECTED_BOOL(pAmount > 0);

	const int32 currencyAtFrom = _getCurrency(pFromSlot, pFromType);
	EXPECTED_BOOL(currencyAtFrom >= pAmount);

	int32 addAmount = 0;
	int32 removeAmount = 0;

	// Trivial move (e.g. Gold to Gold)
	if (pFromType == pToType) {
		addAmount = pAmount;
		removeAmount = pAmount;
	}
	// (Conversion) Moving larger currency to smaller currency
	else if (pFromType > pToType) {
		uint32 diff = pFromType - pToType;
		addAmount = static_cast<int32>(pAmount * std::pow(10, diff)); // Convert large to small
		removeAmount = pAmount;
	}
	// (Conversion) Moving smaller currency to larger currency
	else {
		uint32 diff = pToType - pFromType;
		uint32 denominator = static_cast<uint32>(std::pow(10, diff));
		addAmount = pAmount / denominator; // Convert small to large
		// NOTE: The remainder of the above division is ignored, the Client will keep it on their cursor.
		removeAmount = pAmount - (pAmount % denominator);
	}

	EXPECTED_BOOL(removeCurrency(pFromSlot, pFromType, removeAmount));
	EXPECTED_BOOL(addCurrency(pToSlot, pToType, addAmount));

	return true;
}

const bool Inventoryy::addCurrency(const uint32 pSlot, const int32 pPlatinum, const int32 pGold, const int32 pSilver, const int32 pCopper) {
	EXPECTED_BOOL(addCurrency(pSlot, CurrencyType::Platinum, pPlatinum));
	EXPECTED_BOOL(addCurrency(pSlot, CurrencyType::Gold, pGold));
	EXPECTED_BOOL(addCurrency(pSlot, CurrencyType::Silver, pSilver));
	EXPECTED_BOOL(addCurrency(pSlot, CurrencyType::Copper, pCopper));
	return true;
}

const uint64 Inventoryy::getTotalCurrency() const {
	uint64 total = 0;
	for (auto i = 0; i < CurrencySlot::MAX; i++) {
		for (auto j = 0; j < CurrencyType::MAX; j++) {
			total += _getCurrency(i, j) * static_cast<uint64>(std::pow(10, j));
		}
	}
	return total;
}

const bool Inventoryy::currencyValid() const {
	for (auto i = 0; i < CurrencySlot::MAX; i++) {
		for (auto j = 0; j < CurrencyType::MAX; j++) {
			if (_getCurrency(i, j) < 0)
				return false;
		}
	}

	return true;
}

const bool Inventoryy::onTradeAccept() {
	// Clear trade currency.
	setCurrency(CurrencySlot::Trade, CurrencyType::Platinum, 0);
	setCurrency(CurrencySlot::Trade, CurrencyType::Gold, 0);
	setCurrency(CurrencySlot::Trade, CurrencyType::Silver, 0);
	setCurrency(CurrencySlot::Trade, CurrencyType::Copper, 0);

	return true;
}

const bool Inventoryy::onTradeCancel() {
	// Record total current before moving anything.
	const uint64 preMoveCurrency = getTotalCurrency();

	const auto platinum = getTradePlatinum();
	const auto gold = getTradeGold();
	const auto silver = getTradeSilver();
	const auto copper = getTradeCopper();

	// Move trade slot currency to personal.
	if (platinum > 0) { EXPECTED_BOOL(moveCurrency(CurrencySlot::Trade, CurrencySlot::Personal, CurrencyType::Platinum, CurrencyType::Platinum, platinum)); }
	if (gold > 0) { EXPECTED_BOOL(moveCurrency(CurrencySlot::Trade, CurrencySlot::Personal, CurrencyType::Gold, CurrencyType::Gold, gold)); }
	if (silver > 0) { EXPECTED_BOOL(moveCurrency(CurrencySlot::Trade, CurrencySlot::Personal, CurrencyType::Silver, CurrencyType::Silver, silver)); }
	if (copper > 0) { EXPECTED_BOOL(moveCurrency(CurrencySlot::Trade, CurrencySlot::Personal, CurrencyType::Copper, CurrencyType::Copper, copper)); }

	// Check: Total currency has not changed.
	EXPECTED_BOOL(preMoveCurrency == getTotalCurrency());
	
	// Check: Currency is still in a valid state.
	EXPECTED_BOOL(currencyValid());


	return true;
}

const uint32 Inventoryy::findEmptySlot(Item* pItem) const {
	EXPECTED_VAR(pItem, SlotID::None); // This probably not wise :/

	// Check: Main
	for (uint32 i = SlotID::MAIN_0; i <= SlotID::MAIN_7; i++) {
		auto item = getItem(i);
		if (!item) return i;
	}
	
	// Check: Main Contents
	for (uint32 i = SlotID::MAIN_0; i <= SlotID::MAIN_7; i++) {
		auto item = getItem(i);
		if (item && item->isContainer() && item->getContainerSize() >= pItem->getSize()) {
			const uint32 slotID = item->findEmptySlot();
			if (SlotID::isNone(slotID) == false)
				return slotID;
		}
	}

	return SlotID::None;
}

const uint32 Inventoryy::findEmptySlot(const bool pContainer, const uint8 pItemSize) const {
	// Check: Main
	for (uint32 i = SlotID::MAIN_0; i <= SlotID::MAIN_7; i++) {
		auto item = getItem(i);
		if (!item) return i;
	}

	if (pContainer) return SlotID::None;

	// Check: Main Contents
	for (uint32 i = SlotID::MAIN_0; i <= SlotID::MAIN_7; i++) {
		auto item = getItem(i);
		if (item && item->isContainer() && item->getContainerSize() >= pItemSize) {
			const uint32 slotID = item->findEmptySlot();
			if (SlotID::isNone(slotID) == false)
				return slotID;
		}
	}

	return SlotID::None;
}

void Inventoryy::getTradeItems(std::list<Item*>& pItems) const {
	for (auto i : mTrade) {
		if (i) pItems.push_back(i);
	}
}

const bool Inventoryy::clearTradeItems() {
	for (uint32 i = SlotID::TRADE_0; i <= SlotID::TRADE_7; i++)
		EXPECTED_BOOL(_clear(i));

	return true;
}

const bool Inventoryy::addCurrency(const int32 pPlatinum, const int32 pGold, const int32 pSilver, const int32 pCopper) {
	
	// Get current currency as copper.
	int64 currentCurrency = 0;
	EXPECTED_BOOL(Utility::convertCurrency(currentCurrency, getPersonalPlatinum(), getPersonalGold(), getPersonalSilver(), getPersonalCopper()));

	// Get amount being added as copper.
	int64 addedCurrency = 0;
	EXPECTED_BOOL(Utility::convertCurrency(addedCurrency, pPlatinum, pGold, pSilver, pCopper));

	int64 newCurrency = currentCurrency + addedCurrency;
	int32 newPlatinum = 0, newGold = 0, newSilver = 0, newCopper = 0;
	Utility::convertFromCopper<uint64>(newCurrency, newPlatinum, newGold, newSilver, newCopper);

	// Set new currency amounts.
	setCurrency(CurrencySlot::Personal, CurrencyType::Platinum, newPlatinum);
	setCurrency(CurrencySlot::Personal, CurrencyType::Gold, newGold);
	setCurrency(CurrencySlot::Personal, CurrencyType::Silver, newSilver);
	setCurrency(CurrencySlot::Personal, CurrencyType::Copper, newCopper);

	return true;
}

const bool Inventoryy::addCurrency(const uint32 pSlot, const uint32 pType, const int32 pAmount) {
	EXPECTED_BOOL(CurrencySlot::isValid(pSlot));
	EXPECTED_BOOL(CurrencyType::isValid(pType));
	EXPECTED_BOOL(pAmount >= 0);

	mCurrency[pSlot][pType] += pAmount;
	return true;
}

const bool Inventoryy::removeCurrency(const int32 pPlatinum, const int32 pGold, const int32 pSilver, const int32 pCopper) {
	
	// Get current currency as copper.
	int64 currentCurrency = 0;
	EXPECTED_BOOL(Utility::convertCurrency(currentCurrency, getPersonalPlatinum(), getPersonalGold(), getPersonalSilver(), getPersonalCopper()));

	// Get amount being removed as copper.
	int64 removedCurrency = 0;
	EXPECTED_BOOL(Utility::convertCurrency(removedCurrency, pPlatinum, pGold, pSilver, pCopper));

	EXPECTED_BOOL(currentCurrency >= removedCurrency);

	int64 newCurrency = currentCurrency - removedCurrency;
	int32 newPlatinum = 0, newGold = 0, newSilver = 0, newCopper = 0;
	Utility::convertFromCopper<uint64>(newCurrency, newPlatinum, newGold, newSilver, newCopper);

	// Set new currency amounts.
	setCurrency(CurrencySlot::Personal, CurrencyType::Platinum, newPlatinum);
	setCurrency(CurrencySlot::Personal, CurrencyType::Gold, newGold);
	setCurrency(CurrencySlot::Personal, CurrencyType::Silver, newSilver);
	setCurrency(CurrencySlot::Personal, CurrencyType::Copper, newCopper);

	return true;
}

const uint64 Inventoryy::getTotalCursorCurrency() const {
	int64 value = 0;
	EXPECTED_VAR(Utility::convertCurrency(value, getCursorPlatinum(), getCursorGold(), getCursorSilver(), getCursorCopper()), 0);
	return value;
}

const uint64 Inventoryy::getTotalPersonalCurrency() const {
	int64 value = 0;
	EXPECTED_VAR(Utility::convertCurrency(value, getPersonalPlatinum(), getPersonalGold(), getPersonalSilver(), getPersonalCopper()), 0);
	return value;
}

const uint64 Inventoryy::getTotalBankCurrency() const {
	int64 value = 0;
	EXPECTED_VAR(Utility::convertCurrency(value, getBankPlatinum(), getBankGold(), getBankSilver(), getBankCopper()), 0);
	return value;
}

const uint64 Inventoryy::getTotalTradeCurrency() const {
	int64 value = 0;
	EXPECTED_VAR(Utility::convertCurrency(value, getTradePlatinum(), getTradeGold(), getTradeSilver(), getTradeCopper()), 0);
	return value;
}

const uint64 Inventoryy::getTotalSharedBankCurrency() const {
	int64 value = 0;
	// NOTE: The shared bank only supports storing platinum.
	EXPECTED_VAR(Utility::convertCurrency(value, getSharedBankPlatinum(), 0, 0, 0), 0);
	return value;
}

const uint32 Inventoryy::getAlternateCurrencyQuantity(const uint32 pCurrencyID) const {
	auto search = mAlternateCurrency.find(pCurrencyID);
	if (search == mAlternateCurrency.end()) return 0;
	return search->second;
}

void Inventoryy::addAlternateCurrency(const uint32 pCurrencyID, const uint32 pQuantity) {
	setAlternateCurrencyQuantity(pCurrencyID, getAlternateCurrencyQuantity(pCurrencyID) + pQuantity);
}

void Inventoryy::removeAlternateCurrency(const uint32 pCurrencyID, const uint32 pQuantity) {
	const uint32 currentQuantity = getAlternateCurrencyQuantity(pCurrencyID);
	if (currentQuantity >= pQuantity)
		setAlternateCurrencyQuantity(pCurrencyID, currentQuantity - pQuantity);
	else {
		setAlternateCurrencyQuantity(pCurrencyID, 0);
	}
}

const bool save(Item* pItem, Data::Item* pDataItem) {
	return true;
}

const bool saveItem(Item* pItem, std::list<Data::Item>& pList) {
	EXPECTED_BOOL(pItem);

	// Copy data from Item to Data::Item.
	Data::Item item;
	item.mItemID = pItem->getID();
	item.mSlot = pItem->hasParent() ? pItem->getSubIndex() : pItem->getSlot();
	item.mStacks = pItem->getStacks();
	item.mCharges = pItem->getCharges();
	item.mAttuned = pItem->isAttuned();
	item.mLastCastTime = pItem->getLastCastTime();

	// Copy container contents.
	if (pItem->isContainer()) {
		std::list<Item*> contents;
		pItem->getContents(contents);
		
		for (auto i : contents)
			EXPECTED_BOOL(saveItem(i, item.mSubItems));
	}
	// Copy Item augmentations.
	else if (pItem->hasAugmentations()) {
		std::list<Item*> augmentations;
		pItem->getAugmentations(augmentations);

		for (auto i : augmentations)
			EXPECTED_BOOL(saveItem(i, item.mSubItems));
	}

	pList.push_back(item);
	return true;
}

const bool Inventoryy::updateForSave(Data::Inventory& pInventoryData) {
	// Worn / Primary Inventory.
	for (auto i : mItems) {
		if (i) {
			EXPECTED_BOOL(saveItem(i, pInventoryData.mItems));
		}
	}

	// Cursor
	for (auto i : mCursor)
		EXPECTED_BOOL(saveItem(i, pInventoryData.mItems));

	// Bank
	for (auto i : mBank) {
		if (i) {
			EXPECTED_BOOL(saveItem(i, pInventoryData.mItems));
		}
	}
	
	// Shared Bank
	for (auto i : mSharedBank) {
		if (i) {
			EXPECTED_BOOL(saveItem(i, pInventoryData.mItems));
		}
	}

	// Trade
	for (auto i : mTrade) {
		if (i) {
			EXPECTED_BOOL(saveItem(i, pInventoryData.mItems));
		}
	}

	return true;
}

Item* loadItem(Data::Item& pItem) {
	auto item = ItemFactory::make(pItem.mItemID, pItem.mStacks);
	EXPECTED_PTR(item);
	item->setIsAttuned(pItem.mAttuned);
	item->setLastCastTime(pItem.mLastCastTime);
	item->setCharges(pItem.mCharges);

	// Container contents.
	if (item->isContainer()) {
		for (auto& i : pItem.mSubItems) {
			auto subItem = loadItem(i);
			EXPECTED_PTR(subItem);
			EXPECTED_PTR(item->setContents(subItem, i.mSlot));
		}
	}
	// Augmentations.
	else if (pItem.mSubItems.size() > 0) {
		for (auto& i : pItem.mSubItems) {
			auto subItem = loadItem(i);
			EXPECTED_PTR(subItem);
			item->setAugmentation(i.mSlot, subItem);
		}
	}

	return item;
}

const bool Inventoryy::loadFromSave(Data::Inventory& pInventoryData) {
	for (auto i : pInventoryData.mItems) {
		auto item = loadItem(i);
		EXPECTED_BOOL(item);
		EXPECTED_BOOL(put(item, i.mSlot));
	}

	return true;
}
