#include "World.h"
#include "Utility.h"
#include "ZoneManager.h"
#include "AccountManager.h"
#include "DataStore.h"
#include "LoginServerConnection.h"
#include "UCSConnection.h"

#include "../common/EmuTCPServer.h"
#include "../common/EQStreamFactory.h"
#include "../common/EQStreamIdent.h"
#include "../common/patches/patches.h"
#include "../common/eq_packet_structs.h"
#include "../common/extprofile.h"

#include "WorldClientConnection.h"
#include "Constants.h"
#include "LogSystem.h"

World::World(DataStore* pDataStore) :
	mInitialised(false),
	mLocked(false),
	mStreamIdentifier(0),
	mStreamFactory(0),
	mZoneManager(0),
	mAccountManager(0),
	mTCPServer(0),
	mLoginServerConnection(0),
	mUCSConnection(0),
	mDataStore(pDataStore)
{

}

World::~World() {
	if (mStreamFactory) mStreamFactory->Close();
	if (mTCPServer) mTCPServer->Close();
	if (mUCSConnection) mUCSConnection->disconnect();
	// TODO: Close LoginServerConnection?

	safe_delete(mTCPServer);
	safe_delete(mLoginServerConnection);
	safe_delete(mStreamFactory);
	safe_delete(mStreamIdentifier);
	safe_delete(mZoneManager);
	safe_delete(mAccountManager);
	safe_delete(mUCSConnection);
}

bool World::initialise()
{
	// Prevent multiple initialisation.
	if (mInitialised) return false;

	// Create our connection to the Login Server
	Log::status("[WORLD] Login Server Connection : Initialising");
	mLoginServerConnection = new LoginServerConnection(this, "127.0.0.1", 5998, "Admin", "Password");
	if (!mLoginServerConnection->initialise()) {
		Log::error("[WORLD] Login Server Connection : Failed to initialise");
		return false;
	}
	Log::status("[WORLD] Login Server Connection : Initialised");

	// Create and initialise EQStreamFactory.
	mStreamFactory = new EQStreamFactory(WorldStream, 9000); // [Client Limitation] World must use port 9000
	if (!mStreamFactory->Open()) {
		Utility::criticalError("Unable to open World Stream");
		return false;
	}

	mStreamIdentifier = new EQStreamIdentifier;
	RegisterAllPatches(*mStreamIdentifier);

	mZoneManager = new ZoneManager(mDataStore);
	mZoneManager->initialise();

	mAccountManager = new AccountManager(mDataStore);
	if (!mAccountManager->initialise()) {
		Utility::criticalError("Unable to initialise Account Manager");
		return false;
	}

	mTCPServer = new EmuTCPServer();
	char errbuf[TCPConnection_ErrorBufferSize];
	mTCPServer->Open(9000, errbuf);

	mUCSConnection = new UCSConnection();

	mInitialised = true;
	return true;
	/*
	Tasks which previously occurred at startup.
	- Load World Config
	- Load Log Settings
	- Load Variables
	- Load Zone Names
	- Clear groups from DB
	- Clear raids
	- Clear raid details
	- Load Items
	- Load Guilds
	- Load Ruleset
	- Delete stale player corpses.
	- Purge expired instances.
	*/
}

void World::update()
{
	_checkUCSConnection();

	mUCSConnection->update();
	mLoginServerConnection->update();
	mDataStore->update();
	mZoneManager->update();

	// Check if any new clients are connecting.
	_handleIncomingClientConnections();

	// Update World Clients.
	for (auto i : mClients) {
		i->update();
	}
}

void World::_handleIncomingClientConnections() {
	// Check for incoming connections.
	EQStream* incomingStream = nullptr;
	while (incomingStream = mStreamFactory->Pop()) {
		// Hand over to the EQStreamIdentifier. (Determine which client to user has)
		mStreamIdentifier->AddStream(incomingStream);
	}

	mStreamIdentifier->Process();

	// Check for identified streams.
	EQStreamInterface* incomingStreamInterface = nullptr;
	while (incomingStreamInterface = mStreamIdentifier->PopIdentified()) {
		// TODO: Add Banned IPs check.
		Utility::print("World Incoming Connection");
		mClients.push_back(new WorldClientConnection(incomingStreamInterface, this));
	}
}

bool World::isLoginServerConnected() {
	return mLoginServerConnection->isConnected();
}

void World::_checkUCSConnection() {
	EmuTCPConnection* tcpConnection = 0;
	while ((tcpConnection = mTCPServer->NewQueuePop())) {
		if (tcpConnection->GetPacketMode() == EmuTCPConnection::packetModeUCS) {
			mUCSConnection->setConnection(tcpConnection);
		}
	}
}

void World::notifyIncomingClient(uint32 pLoginServerID, std::string pLoginServerAccountName, std::string pLoginServerKey, int16 pWorldAdmin, uint32 pIP, uint8 pLocal) {
	IncomingClient* client = new IncomingClient();
	client->mAccountID = pLoginServerID;
	client->mAccountName = pLoginServerAccountName;
	client->mKey = pLoginServerKey;
	client->mWorldAdmin = pWorldAdmin;
	client->mIP = pIP;
	client->mLocal = pLocal;
	mIncomingClients.push_back(client);
}

bool World::tryIdentify(WorldClientConnection* pConnection, uint32 pLoginServerAccountID, std::string pLoginServerKey) {
	// Check Incoming Clients that match Account ID / Key
	for (auto i = mIncomingClients.begin(); i != mIncomingClients.end(); i++) {
		IncomingClient* incClient = *i;
		if (pLoginServerAccountID == incClient->mAccountID && pLoginServerKey == incClient->mKey && !pConnection->getIdentified()) {
			// Configure the WorldClientConnection with details from the IncomingClient.
			pConnection->_setIdentified(true);
			pConnection->setLoginServerAccountID(incClient->mAccountID);
			pConnection->setLoginServerAccountName(incClient->mAccountName);
			pConnection->setLoginServerKey(incClient->mKey);
			pConnection->setWorldAccountID(mAccountManager->getWorldAccountID(pLoginServerAccountID));
			// TODO: Do I need to set IP or Local here?
			delete incClient;
			mIncomingClients.erase(i);
			return true;
		}
	}
	return false;
}

void World::setLocked(bool pLocked) {
	mLocked = pLocked;
	// Notify Login Server!
	mLoginServerConnection->sendWorldStatus();
}

int16 World::getUserToWorldResponse(uint32 pLoginServerAccountID) {
	static const int16 ACCOUNT_STATUS_SUSPENDED = -1;
	static const int16 ACCOUNT_STATUS_BANNED = -2;

	// Fetch the Account Status.
	uint32 accountStatus = mAccountManager->getStatusFromLoginServerID(pLoginServerAccountID);

	// Account Suspended.
	if (accountStatus == ACCOUNT_STATUS_SUSPENDED) return -1;
	// Account Banned.
	if (accountStatus == ACCOUNT_STATUS_BANNED) return -2;

	// Server is Locked (Only GM/Admin may enter)
	if (mLocked && accountStatus >= 100) return 1;
	// Server is Locked and user is not a GM/Admin.
	else if( mLocked && accountStatus < 100) return 0;

	return 1; // Speak friend and enter.
}

bool World::getCharacterSelectInfo(uint32 pWorldAccountID, CharacterSelect_Struct* pCharacterSelectData) {
	return mDataStore->getCharacterSelectInfo(pWorldAccountID, pCharacterSelectData);
}

bool World::isCharacterNameUnique(std::string pCharacterName) { return mDataStore->isCharacterNameUnique(pCharacterName); }

bool World::isCharacterNameReserved(std::string pCharacterName) {
	for (auto i : mReservedCharacterNames)
		if (i.second == pCharacterName)
			return true;
	return false;
}

void World::reserveCharacterName(uint32 pWorldAccountID, std::string pCharacterName) {
	mReservedCharacterNames.insert(std::make_pair(pWorldAccountID, pCharacterName));
}

bool World::deleteCharacter(uint32 pWorldAccountID, std::string pCharacterName) {
	// Verify that character with pCharacterName belongs to pWorldAccountID.
	bool isOwner = mDataStore->checkOwnership(pWorldAccountID, pCharacterName);
	if (!isOwner) {
		Log::error("Attempt made to delete a character that does not belong to the account owner."); // TODO: More information.. this is haxxors.
		return false;
	}
	return mDataStore->deleteCharacter(pCharacterName);
}

bool World::createCharacter(uint32 pWorldAccountID, std::string pCharacterName, CharCreate_Struct* pData) {
	// Find reserved name.
	auto i = mReservedCharacterNames.find(pWorldAccountID);
	if (i == mReservedCharacterNames.end()) {
		Log::error("Reserved character name not found during character creation!");
		return false;
	} else {
		mReservedCharacterNames.erase(pWorldAccountID);
	}

	// Create and configure profile.
	PlayerProfile_Struct profile;
	memset(&profile, 0, sizeof(PlayerProfile_Struct));
	strcpy(profile.name, pCharacterName.c_str());

	ExtendedProfile_Struct extendedProfile;
	memset(&extendedProfile, 0, sizeof(ExtendedProfile_Struct));
	// TODO: Check Validity!
	
	profile.race = pData->race;
	profile.class_ = pData->class_;
	profile.gender = pData->gender;
	profile.deity = pData->deity;
	profile.STR = pData->STR;
	profile.STA = pData->STA;
	profile.AGI = pData->AGI;
	profile.DEX = pData->DEX;
	profile.WIS = pData->WIS;
	profile.INT = pData->INT;
	profile.CHA = pData->CHA;
	profile.face = pData->face;
	profile.eyecolor1 = pData->eyecolor1;
	profile.eyecolor2 = pData->eyecolor2;
	profile.hairstyle = pData->hairstyle;
	profile.haircolor = pData->haircolor;
	profile.beard = pData->beard;
	profile.beardcolor = pData->beardcolor;
	profile.drakkin_heritage = pData->drakkin_heritage;
	profile.drakkin_tattoo = pData->drakkin_tattoo;
	profile.drakkin_details = pData->drakkin_details;
	profile.birthday = 0; // TODO:
	profile.lastlogin = 0; // TODO:
	profile.level = 1;
	profile.points = 5;
	profile.cur_hp = 1000;
	profile.hunger_level = 6000;
	profile.thirst_level = 6000;
	profile.zone_id = ZoneIDs::NorthQeynos;
	profile.x = 0;
	profile.y = 0;
	profile.z = 0;

	for (int i = 0; i < MAX_PP_SPELLBOOK; i++)
		profile.spell_book[i] = 0xFFFFFFFF;

	for (int i = 0; i < MAX_PP_MEMSPELL; i++)
		profile.mem_spells[i] = 0xFFFFFFFF;

	for (int i = 0; i < BUFF_COUNT; i++)
		profile.buffs[i].spellid = 0xFFFF;

	profile.binds[0].zoneId = profile.zone_id;
	profile.binds[0].x = profile.x;
	profile.binds[0].y = profile.y;
	profile.binds[0].z = profile.z;
	profile.binds[0].heading = profile.heading;

	if (!mDataStore->createCharacter(pWorldAccountID, pCharacterName, &profile, &extendedProfile)) {
		Log::error("Could not create character!"); // pCharacterName
		return false;
	}

	return true;
}

bool World::isWorldEntryAllowed(uint32 pWorldAccountID, std::string pCharacterName)
{
	return mDataStore->checkOwnership(pWorldAccountID, pCharacterName);
}

uint16 World::getZonePort(uint16 pZoneID, uint16 pInstanceID) {
	return mZoneManager->getZonePort(pZoneID, pInstanceID);
}