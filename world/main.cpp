#include "../common/timer.h"
#include "../common/timeoutmgr.h"
#include "../common/platform.h"
#include "../common/crash.h"

#include "DataValidation.h"
#include "LogSystem.h"
#include "World.h"
#include "ZoneData.h"
#include "GuildManager.h"
#include "AccountManager.h"
#include "ZoneManager.h"
#include "TitleManager.h"
#include "XMLDataStore.h"
#include "SpellDataStore.h"
#include "StaticItemData.h"
#include "ItemDataStore.h"
#include "LogSystem.h"
#include "UCS.h"
#include "Random.h"
#include "NPCFactory.h"
#include "Timer.h"
#include "TimeUtility.h"
#include "Transmutation.h"
#include "AlternateCurrencyManager.h"
#include "ShopDataStore.h"
#include "Settings.h"
#include "HateControllerFactory.h"
#include "ServiceLocator.h"
#include "GroupManager.h"
#include "RaidManager.h"
#include "ItemFactory.h"
#include "CommandHandler.h"

#include "Testing.h"
#include "gtest/gtest.h"
#pragma comment(lib, "../../dependencies/gtest/gtestd.lib")

#include <memory>

/*
Google Test Notes
- Changed CRT to static
- Changed 'Debug Information Format' to 'Program Database'
- Addition Includes + gtest
*/

uint64 TTimer::mCurrentTime = 0;

TimeoutManager timeout_manager;

int main(int argc, char** argv)  {
	system("pause");
	::testing::InitGoogleTest(&argc, argv);
	RUN_ALL_TESTS();

	EXPECTED_MAIN(Log::start("logs/" + std::to_string(Time::nowMilliseconds()) + ".log"));
	//system("pause");

	RegisterExecutablePlatform(ExePlatformWorld);
	set_exception_handler();

	Random::initialise();

	ServiceLocator::reset();

	ILogFactory* logFactory = new DefaultLogFactory();

	IDataStore* dataStore = new XMLDataStore();
	ServiceLocator::setDataStore(dataStore);
	EXPECTED_MAIN(dataStore->loadSettings());
	EXPECTED_MAIN(dataStore->initialise());

	ShopDataStore* shopDataStore = new ShopDataStore();
	ServiceLocator::setShopDataStore(shopDataStore);
	EXPECTED_MAIN(shopDataStore->initialise(dataStore));

	AccountManager* accountManager = new AccountManager();
	ServiceLocator::setAccountManager(accountManager);
	EXPECTED_MAIN(accountManager->initialise(dataStore, logFactory));

	ZoneDataManager* zoneDataManager = new ZoneDataManager();
	ServiceLocator::setZoneDataManager(zoneDataManager);
	EXPECTED_MAIN(zoneDataManager->initialise(dataStore));

	TitleManager* titleManager = new TitleManager();
	ServiceLocator::setTitleManager(titleManager);
	EXPECTED_MAIN(titleManager->initialise(dataStore));

	SpellDataStore* spellDataStore = new SpellDataStore();
	ServiceLocator::setSpellDataStore(spellDataStore);
	EXPECTED_MAIN(spellDataStore->initialise(dataStore));

	AlternateCurrencyManager* alternateCurrencyManager = new AlternateCurrencyManager();
	ServiceLocator::setAlternateCurrencyManager(alternateCurrencyManager);
	EXPECTED_MAIN(alternateCurrencyManager->initialise(dataStore));

	ItemDataStore* itemDataStore = new ItemDataStore();
	ServiceLocator::setItemDataStore(itemDataStore);
	EXPECTED_MAIN(itemDataStore->initialise(dataStore));

	ItemFactory* itemFactory = new ItemFactory();
	ServiceLocator::setItemFactory(itemFactory);
	EXPECTED_MAIN(itemFactory->initialise(itemDataStore));

	ZoneManager* zoneManager = new ZoneManager();
	ServiceLocator::setZoneManager(zoneManager);

	GroupManager* groupManager = new GroupManager();
	ServiceLocator::setGroupManager(groupManager);
	
	RaidManager* raidManager = new RaidManager();
	ServiceLocator::setRaidManager(raidManager);

	GuildManager* guildManager = new GuildManager();
	ServiceLocator::setGuildManager(guildManager);

	CommandHandler* commandHandler = new CommandHandler();
	EXPECTED_MAIN(commandHandler->initialise(dataStore));

	EXPECTED_MAIN(zoneManager->initialise(zoneDataManager, groupManager, raidManager, guildManager, commandHandler, itemFactory));
	EXPECTED_MAIN(groupManager->initialise(zoneManager));
	EXPECTED_MAIN(raidManager->initialise(zoneManager));
	EXPECTED_MAIN(guildManager->initialise(dataStore, zoneManager));

	World* world = new World();
	ServiceLocator::setWorld(world);
	EXPECTED_MAIN(world->initialise(dataStore, logFactory, zoneManager, accountManager));

	UCS* ucs = new UCS();
	ServiceLocator::setUCS(ucs);
	EXPECTED_MAIN(ucs->initialise());

	NPCFactory* npcFactory = new NPCFactory();
	ServiceLocator::setNPCFactory(npcFactory);
	EXPECTED_MAIN(npcFactory->initialise(dataStore, itemFactory, shopDataStore));

	// Validate Data
	if (Settings::getValidationEnabled()) {
		if (!validateData()) {
			Log::error("Data Validation has failed!");
			system("pause");
			return 0;
		}
	}

	while(true) {
		Timer::SetCurrentTime();
		TTimer::setCurrentTime(Time::nowMilliseconds());
		world->update();

		//check for timeouts in other threads
		timeout_manager.CheckTimeouts();
		Sleep(20);
	}
	Log::status("World : Shutting down");

	Log::end();
	return 0;
}