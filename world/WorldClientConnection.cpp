#include "WorldClientConnection.h"
#include "ServiceLocator.h"
#include "GuildManager.h"
#include "ZoneManager.h"

#include "../common/EQPacket.h"
#include "../common/EQStreamIntf.h"

#include "World.h"
#include "Utility.h"
#include "Limits.h"

#include "LogSystem.h"
#include "AccountManager.h"
#include "Data.h"
#include "Payload.h"
#include "Settings.h"


WorldConnection::WorldConnection(World* pWorld, EQStreamInterface* pStreamInterface) :
	mStreamInterface(pStreamInterface),
	mWorld(pWorld)
{
	mIP = mStreamInterface->GetRemoteIP();
	mPort = ntohs(mStreamInterface->GetRemotePort());

	ClientVersionBit = 1 << (mStreamInterface->ClientVersion() - 1);
}

WorldConnection::~WorldConnection() {
	//let the stream factory know were done with this stream
	mStreamInterface->Close();
	mStreamInterface->ReleaseFromUse();
}

bool WorldConnection::update() {
	// Check our connection.
	if (mConnectionDropped || !mStreamInterface->CheckState(ESTABLISHED)) {
		Utility::print("WorldClientConnection Lost.");
		return false;
	}

	bool ret = true;

	// Handle any incoming packets.
	EQApplicationPacket* packet = 0;
	while (ret && (packet = (EQApplicationPacket *)mStreamInterface->PopPacket())) {
		ret = _handlePacket(packet);
		delete packet;
	}

	return ret;
}

bool WorldConnection::_handlePacket(const EQApplicationPacket* pPacket) {
	EXPECTED_BOOL(pPacket);

	// Check if unidentified and sending something other than OP_SendLoginInfo
	// NOTE: Many functions called below assume getAuthenticated is checked here so do not remove it.
	if (!getAuthenticated() && pPacket->GetOpcode() != OP_SendLoginInfo) {
		Log::error("Unidentified Client sent %s, expected OP_SendLoginInfo"); //OpcodeNames[opcode]
		return false;
	}

	EmuOpcode opcode = pPacket->GetOpcode();
	switch (opcode) {
	case OP_AckPacket:
	case OP_World_Client_CRC1:
	case OP_World_Client_CRC2:
		return true;
	case OP_SendLoginInfo:
		// NOTE: Sent when Client initially connects (Moving from Server Selection to Character Selection).
		// NOTE: Sent when Client is moving from one zone to another.
		return _handleConnect(pPacket);
	case OP_ApproveName:
		// NOTE: This occurs when the user clicks the 'Create Character' button.
		return _handleNameApprovalPacket(pPacket);
	case OP_RandomNameGenerator:
		// NOTE: This occurs when the user clicks the 'Get Name' button.
		return _handleGenerateRandomNamePacket(pPacket);
	case OP_CharacterCreateRequest:
		// NOTE: This occurs when the user clicks the 'New a New Character' button.
		return _handleCharacterCreateRequestPacket(pPacket);
	case OP_CharacterCreate:
		// NOTE: This occurs when the client receives OP_ApproveName.
		return _handleCharacterCreatePacket(pPacket);
	case OP_EnterWorld:
		return _handleEnterWorld(pPacket);
	case OP_DeleteCharacter:
		// NOTE: This occurs when the user clicks the 'Delete Character' button.
		return _handleDeleteCharacterPacket(pPacket);
	case OP_WorldComplete:
		// NOTE: This occurs after OP_EnterWorld is replied to.
		mStreamInterface->Close();
		return true;
	case OP_LoginUnknown1:
	case OP_LoginUnknown2:
	case OP_CrashDump:
	case OP_WearChange:
	case OP_LoginComplete:
	case OP_ApproveWorld:
	case OP_WorldClientReady:
		return true;
	default:
		// Ignore.
		//Log::error("[World Client Connection] Got unexpected packet, ignoring.");
		return true;
	}
	return true;
}

void WorldConnection::_sendLogServer() {
	using namespace Payload::World;
	auto outPacket = new EQApplicationPacket(OP_LogServer, LogServer::size());
	auto payload = LogServer::convert(outPacket->pBuffer);

	strcpy(payload->mWorldShortName, Settings::getServerShortName().c_str());	
	payload->mEnableEmail = 0;
	payload->mEnabledVoiceMacros = 0;
	payload->mPVPEnabled = 0;
	payload->mEnablePetitionWindow = 1;
	payload->mFVEnabled = 0;

	mStreamInterface->QueuePacket(outPacket);
	safe_delete(outPacket);
}

void WorldConnection::_sendEnterWorld(String pCharacterName) {
	auto outPacket = new EQApplicationPacket(OP_EnterWorld, pCharacterName.length() + 1);
	strcpy(reinterpret_cast<char*>(outPacket->pBuffer), pCharacterName.c_str());
	
	mStreamInterface->QueuePacket(outPacket);
	safe_delete(outPacket);
}

void WorldConnection::_sendExpansionInfo() {
	using namespace Payload::World;

	auto packet = ExpansionInfo::construct(16383); // TODO: Magic.
	sendPacket(packet);
	delete packet;
}

void WorldConnection::_sendCharacterSelectInfo() {
	using namespace Payload::World;
	auto outPacket = new EQApplicationPacket(OP_SendCharInfo, CharacterSelect::size());
	auto payload = CharacterSelect::convert(outPacket->pBuffer);
	auto accountData = ServiceLocator::getAccountManager()->getAccount(mAccountID);
	EXPECTED(accountData);

	int charSlot = 0;
	for (auto i : accountData->mCharacterData) {
		strcpy(payload->mNames[charSlot], i->mName.c_str());
		payload->mRaces[charSlot] = i->mRace;
		payload->mClasses[charSlot] = i->mClass;
		payload->mZoneIDs[charSlot] = i->mZoneID;
		payload->mLevels[charSlot] = i->mLevel;

		payload->mFaceStyles[charSlot] = i->mFaceStyle;
		payload->mGenders[charSlot] = i->mGender;
		payload->mBeardStyles[charSlot] = i->mBeardStyle;
		payload->mBeardColours[charSlot] = i->mBeardColour;
		payload->mHairStyles[charSlot] = i->mHairStyle;
		payload->mHairColours[charSlot] = i->mHairColour;
		payload->mDrakkinHeritages[charSlot] = i->mDrakkinHeritage;
		payload->mDrakkinTattoos[charSlot] = i->mDrakkinTattoo;
		payload->mDrakkinDetails[charSlot] = i->mDrakkinDetails;
		payload->mDeities[charSlot] = i->mDeity;
		payload->mDrakkinTattoos[charSlot] = i->mDrakkinTattoo;
		payload->eyecolor1[charSlot] = i->mEyeColourLeft;
		payload->eyecolor2[charSlot] = i->mEyeColourRight;

		payload->mTutorialAvailable[charSlot] = 0;
		payload->mGoHomeAvailable[charSlot] = 0;

		// Equipment
		payload->mPrimaryItems[charSlot] = i->mPrimary;
		payload->mSecondaryItems[charSlot] = i->mSecondary;

		for (auto j = 0; j < Limits::Account::MAX_EQUIPMENT_SLOTS; j++) {
			payload->mEquipmentColours[charSlot][j].mColour = i->mEquipment[j].mColour;
			payload->mEquipment[charSlot][j] = i->mEquipment[j].mMaterial;
		}

		charSlot++;
	}

	mStreamInterface->QueuePacket(outPacket);
	safe_delete(outPacket);
}

void WorldConnection::_sendPostEnterWorld() {
	auto outPacket = new EQApplicationPacket(OP_PostEnterWorld, 1);
	outPacket->size = 0;
	mStreamInterface->QueuePacket(outPacket);
	safe_delete(outPacket);
}

bool WorldConnection::_handleConnect(const EQApplicationPacket* pPacket) {
	using namespace Payload::World;
	EXPECTED_BOOL(pPacket);
	EXPECTED_BOOL(Connect::sizeCheck(pPacket));

	auto payload = Connect::convert(pPacket);

	String accountIDStr = Utility::safeString(payload->mInformation, 19);
	String accountKey = Utility::safeString(payload->mInformation + accountIDStr.length() + 1, 16);
	
	u32 accountID = 0;
	EXPECTED_BOOL(Utility::stoSafe(accountID, accountIDStr));
	
	// Check: World is expecting this connection.
	const bool authenticated = mWorld->checkAuthentication(accountID, accountKey);
	if (!authenticated) {
		// Check: Login Server Bypass.
		static const std::map<u32, String> bypass = {
			{ 2, "passwords" },
			{ 3, "passwords" },
		};
		auto bypassSearch = bypass.find(accountID);
		const bool bypassFound = bypassSearch != bypass.end() ? bypassSearch->second == accountKey : false;

		// Not authenticated and no bypass found.
		if (!bypassFound) {
			return false;
		}

		// Hack.
		// TODO: Tidy this up. AccountName needs to accessible in a better way.
		auto accountData = ServiceLocator::getAccountManager()->getAccount(accountID);
		EXPECTED_BOOL(accountData);
		mWorld->addAuthentication(accountID, accountData->mAccountName, accountKey, mIP);
		EXPECTED_BOOL(ServiceLocator::getAccountManager()->ensureAccountLoaded(accountID));
	}

	_setAuthenticated(true);
	setAccountID(accountID);

	mZoning = (payload->mZoning == 1);

	// Going to: Another Zone
	if (mZoning) {
		// Resolve the name of the Character zoning based on Account ID.
		EXPECTED_BOOL(ServiceLocator::getZoneManager()->hasZoningCharacter(accountID));
		String characterName = ServiceLocator::getZoneManager()->getZoningCharacterName(accountID);
		EXPECTED_BOOL(characterName.length() > 0);

		_sendLogServer();
		_sendApproveWorld();
		_sendEnterWorld(characterName);
		_sendPostEnterWorld();
	}
	// Going to: Character Selection Screen.
	else {
		EXPECTED_BOOL(ServiceLocator::getAccountManager()->ensureAccountLoaded(accountID));

		_sendGuildList(); // NOTE: Required. Character guild names do not work (on entering world) without it.
		_sendLogServer();
		_sendApproveWorld();
		_sendEnterWorld(""); // Empty character name when coming from Server Select. 
		_sendPostEnterWorld(); // Required.
		_sendExpansionInfo(); // Required.
		_sendCharacterSelectInfo(); // Required.
	}

	return true;

	/*
	OP_GuildsList, OP_LogServer, OP_ApproveWorld
	All sent in EQEmu but not actually required to get to Character Select. More research on the effects of not sending are required.
	*/
}

bool WorldConnection::_handleNameApprovalPacket(const EQApplicationPacket* pPacket) {
	EXPECTED_BOOL(pPacket);

	// NOTE: Unfortunately I can not find a better place to prevent accounts from going over the maximum number of characters.
	// So we check here and just reject the name if the account is at max.
	// It would be better if I could figure out how the client limits it and duplicate that.
	if (ServiceLocator::getAccountManager()->getNumCharacters(mAccountID) >= Limits::Account::MAX_NUM_CHARACTERS) {
		auto outPacket = new EQApplicationPacket(OP_ApproveName, 1);
		outPacket->pBuffer[0] = 0;
		mStreamInterface->QueuePacket(outPacket);
		return true;
	}
	
	bool valid = true;

	// Check: Name Length.
	String characterName = Utility::safeString((char*)pPacket->pBuffer, Limits::Character::MAX_INPUT_LENGTH);
	valid = Limits::Character::nameInputLength(characterName);

	// TODO: Consider why race and class are sent here?
	uchar race = pPacket->pBuffer[64];
	uchar clas = pPacket->pBuffer[68];

	// Check case of first character (must be uppercase)
	if (characterName[0] < 'A' || characterName[0] > 'Z') {
		valid = false;
	}
	// Check each character is alpha.
	for (String::size_type i = 0; i < characterName.length(); i++) {
		if (!isalpha(characterName[i])) {
			valid = false;
			break;
		}
	}
	// Check if name already in use.
	if (valid && !mWorld->isCharacterNameUnique(characterName)) {
		valid = false;
	}
	// Check if name is reserved.
	if (valid && mWorld->isCharacterNameReserved(characterName)) {
		valid = false;
	}
	// Reserve character name.
	if (valid) {
		// For the rare chance that more than one user tries to create a character with same name.
		mWorld->reserveCharacterName(mAccountID, characterName);
		mReservedCharacterName = characterName;
	}

	auto outPacket = new EQApplicationPacket(OP_ApproveName, 1);
	outPacket->pBuffer[0] = valid? 1 : 0;
	mStreamInterface->QueuePacket(outPacket);
	safe_delete(outPacket);

	return true;
}

bool WorldConnection::_handleGenerateRandomNamePacket(const EQApplicationPacket* pPacket) {
	using namespace Payload::World;
	EXPECTED_BOOL(pPacket);

	auto packet = NameGeneration::construct(0, 0, Utility::getRandomName());
	sendPacket(packet);
	delete packet;

	return true;
}

bool WorldConnection::_handleCharacterCreateRequestPacket(const EQApplicationPacket* packet) {
	struct RaceClassAllocation {
		unsigned int Index;
		unsigned int BaseStats[7];
		unsigned int DefaultPointAllocation[7];
	};

	struct RaceClassCombos {
		unsigned int ExpansionRequired;
		unsigned int Race;
		unsigned int Class;
		unsigned int Deity;
		unsigned int AllocationIndex;
		unsigned int Zone;
	};
	/*
	[Client Limitation][HoT] If the client is not an race/class combinations for any one class the class icon will not be greyed out.
	*/
	static const std::list<int> PlayableRaces = {
		PlayableRaceIDs::Human
	};

	static const std::list<int> PlayableClasses = {
		ClassID::Warrior,
		ClassID::Cleric,
		ClassID::Paladin,
		ClassID::Ranger,
		ClassID::Shadowknight,
		ClassID::Druid,
		ClassID::Monk,
		ClassID::Bard,
		ClassID::Rogue,
		ClassID::Shaman,
		ClassID::Necromancer,
		ClassID::Wizard,
		ClassID::Magician,
		ClassID::Enchanter,
		ClassID::Beastlord,
		ClassID::Berserker
	};

	std::list<RaceClassAllocation> raceClassAllocations;
	RaceClassAllocation r;
	r.Index = 0;
	memset(r.BaseStats, 0, sizeof(r.BaseStats));
	memset(r.DefaultPointAllocation, 0, sizeof(r.DefaultPointAllocation));
	raceClassAllocations.push_back(r);
	std::list<RaceClassCombos> raceClassCombos;

	for (auto i : PlayableRaces) {
		for (auto j : PlayableClasses) {
			raceClassCombos.push_back({ 0, i, j, PlayerDeityIDs::Agnostic, 0, ZoneIDs::NorthQeynos });
		}
	}

	u32 allocs = raceClassAllocations.size();
	u32 combos = raceClassCombos.size();
	u32 len = sizeof(RaceClassAllocation) * allocs;
	len += sizeof(RaceClassCombos) * combos;
	len += sizeof(uint8);
	len += sizeof(u32);
	len += sizeof(u32);

	auto outPacket = new EQApplicationPacket(OP_CharacterCreateRequest, len);
	unsigned char *ptr = outPacket->pBuffer;
	*((uint8*)ptr) = 0;
	ptr += sizeof(uint8);

	*((u32*)ptr) = allocs; // number of allocs.
	ptr += sizeof(u32);

	for (auto i : raceClassAllocations) {
		memcpy(ptr, &i, sizeof(RaceClassAllocation));
		ptr += sizeof(RaceClassAllocation);
	}

	*((u32*)ptr) = combos; // number of combos.
	ptr += sizeof(u32);
	for (auto i : raceClassCombos) {
		memcpy(ptr, &i, sizeof(RaceClassCombos));
		ptr += sizeof(RaceClassCombos);
	}

	mStreamInterface->QueuePacket(outPacket);
	safe_delete(outPacket);
	return true;
}

bool WorldConnection::_handleCharacterCreatePacket(const EQApplicationPacket* pPacket) {
	using namespace Payload::World;
	EXPECTED_BOOL(pPacket);
	EXPECTED_BOOL(CreateCharacter::sizeCheck(pPacket->size));

	auto payload = CreateCharacter::convert(pPacket->pBuffer);

	if (!ServiceLocator::getAccountManager()->handleCharacterCreate(mAccountID, mReservedCharacterName, payload)) {
		dropConnection();
		return false;
	}

	mReservedCharacterName = "";
	return true;
}

bool WorldConnection::_handleEnterWorld(const EQApplicationPacket* pPacket) {
	using namespace Payload::World;
	EXPECTED_BOOL(pPacket);
	EXPECTED_BOOL(EnterWorld::sizeCheck(pPacket->size));

	auto payload = EnterWorld::convert(pPacket->pBuffer);
	String characterName = Utility::safeString(payload->mCharacterName, Limits::Character::MAX_NAME_LENGTH);
	EXPECTED_BOOL(Limits::Character::nameLength(characterName));

	// Check: Account owns the Character.
	EXPECTED_BOOL(ServiceLocator::getAccountManager()->checkOwnership(mAccountID, characterName));

	bool success = mWorld->handleEnterWorld(this, characterName, mZoning);

	// World Entry Failed
	if (!success) {
		// Character Select to World -OR- Character Create to World.
		if (!mZoning) {
			_sendZoneUnavailable();
			_sendCharacterSelectInfo();
			// NOTE: This just bumps the client to Character Select screen.
			return true;
		}
		// Zoning
		else {
			// TODO:
		}
		return true;
	}

	return success;
}

void WorldConnection::_sendChatServer(const String& pCharacterName) {
	std::stringstream ss;
	ss << "127.0.0.1" << "," << Settings::getUCSPort() << "," << Settings::getServerShortName() << "." << pCharacterName << ",";
	ss << "U" << std::hex << std::setfill('0') << std::setw(8) << 34; // TODO: Set up mail key
	String data = ss.str();
	auto outPacket = new EQApplicationPacket(OP_SetChatServer, reinterpret_cast<const unsigned char*>(data.c_str()), data.length()+1);
	auto outPacket2 = new EQApplicationPacket(OP_SetChatServer2, reinterpret_cast<const unsigned char*>(data.c_str()), data.length() + 1);
	mStreamInterface->QueuePacket(outPacket);
	mStreamInterface->QueuePacket(outPacket2);
	safe_delete(outPacket);
	safe_delete(outPacket2);
}

bool WorldConnection::_handleDeleteCharacterPacket(const EQApplicationPacket* pPacket) {
	using namespace Payload::World;
	EXPECTED_BOOL(pPacket);
	EXPECTED_BOOL(DeleteCharacter::sizeCheck(pPacket->size));

	String characterName = Utility::safeString(reinterpret_cast<char*>(pPacket->pBuffer), Limits::Character::MAX_NAME_LENGTH);

	if (mWorld->deleteCharacter(mAccountID, characterName)) {
		StringStream ss; ss << "[World Client Connection] Character: " << characterName << " deleted.";
		Log::info(ss.str());
		_sendCharacterSelectInfo();
		return true;
	}

	Log::error("[World Client Connection] Failed to delete character.");
	return false;
}

void WorldConnection::sendZoneServerInfo(const String& pIP, const u16 pPort) {
	using namespace Payload::World;

	auto packet = ZoneServerInfo::construct(pIP, pPort);
	sendPacket(packet);
	delete packet;
}


void WorldConnection::_sendZoneUnavailable() {
	using namespace Payload::World;

	auto packet = ZoneUnavailable::construct("NONE"); // NOTE: Zone name appears to have no effect.
	sendPacket(packet);
	delete packet;
}

void WorldConnection::_sendGuildList() {
	auto outPacket = new EQApplicationPacket(OP_GuildsList);
	outPacket->size = Limits::Guild::MAX_NAME_LENGTH + (Limits::Guild::MAX_NAME_LENGTH * Limits::Guild::MAX_GUILDS); // TODO: Work out the minimum sized packet UF will accept.
	outPacket->pBuffer = reinterpret_cast<unsigned char*>(ServiceLocator::getGuildManager()->_getGuildNames());

	mStreamInterface->QueuePacket(outPacket);
	outPacket->pBuffer = nullptr;
	safe_delete(outPacket);
}

void WorldConnection::_sendApproveWorld() {
	using namespace Payload::World; 

	auto outPacket = new EQApplicationPacket(OP_ApproveWorld, ApproveWorld::size());
	auto payload = ApproveWorld::convert(outPacket->pBuffer);
	uchar foo[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x87, 0x13, 0xbe, 0xc8, 0xa7, 0x77, 0xcb,
		0x27, 0xed, 0xe1, 0xe6, 0x5d, 0x1c, 0xaa, 0xd3, 0x3c, 0x26, 0x3b, 0x6d, 0x8c, 0xdb, 0x36, 0x8d,
		0x91, 0x72, 0xf5, 0xbb, 0xe0, 0x5c, 0x50, 0x6f, 0x09, 0x6d, 0xc9, 0x1e, 0xe7, 0x2e, 0xf4, 0x38,
		0x1b, 0x5e, 0xa8, 0xc2, 0xfe, 0xb4, 0x18, 0x4a, 0xf7, 0x72, 0x85, 0x13, 0xf5, 0x63, 0x6c, 0x16,
		0x69, 0xf4, 0xe0, 0x17, 0xff, 0x87, 0x11, 0xf3, 0x2b, 0xb7, 0x73, 0x04, 0x37, 0xca, 0xd5, 0x77,
		0xf8, 0x03, 0x20, 0x0a, 0x56, 0x8b, 0xfb, 0x35, 0xff, 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x53, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00
	};

	memcpy(payload->mUnknown0, foo, sizeof(foo));
	mStreamInterface->QueuePacket(outPacket);
	safe_delete(outPacket);
}

void WorldConnection::sendPacket(const EQApplicationPacket* pPacket) {
	mStreamInterface->QueuePacket(pPacket);
}
