#pragma once

#include "Constants.h"
#include "Singleton.h"

class Item;
struct ItemData;
class ItemGenerator : public Singleton<ItemGenerator> {
private:
	friend class Singleton<ItemGenerator>;
	ItemGenerator() {};
	~ItemGenerator() {};
	ItemGenerator(ItemGenerator const&); // Do not implement.
	void operator=(ItemGenerator const&); // Do not implement.
public:
	static Item* makeRandom(const uint8 pLevel);
private:
	Item* _makeRandom(const uint8 pLevel);
	Item* _makeBaseItem();
};