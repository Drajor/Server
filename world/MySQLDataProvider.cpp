#include "MySQLDataProvider.h"
#include "Data.h"
#include "DatabaseConnection.h"
#include "../common/timer.h"
#include "../common/eq_packet_structs.h"
#include "../common/extprofile.h"
#include "../common/StringUtil.h"
#include "Utility.h"
#include "LogSystem.h"
#include "Profile.h"

#define KEEP_ALIVE_TIMER 10000

#define USE_FIELD_MACROS int j = 0
#define NEXT_FIELD row[j++]
#define NEXT_FIELD_INT atoi(NEXT_FIELD) 
#define NEXT_FIELD_BOOL atoi(NEXT_FIELD) > 0 ? true : false

MySQLDataProvider::MySQLDataProvider() : mDatabaseConnection(0), mKeepAliveTimer(0) { }

MySQLDataProvider::~MySQLDataProvider() {
	safe_delete(mKeepAliveTimer);
	safe_delete(mDatabaseConnection);
}

bool MySQLDataProvider::initialise() {
	mKeepAliveTimer = new Timer(KEEP_ALIVE_TIMER);
	mKeepAliveTimer->Trigger();

	mDatabaseConnection = new DatabaseConnection();
	return mDatabaseConnection->initialise();
}

void MySQLDataProvider::update() {
	// Is it time to ping mysql?
	if (mKeepAliveTimer->Check()) {
		mKeepAliveTimer->Start();
		mDatabaseConnection->ping();
	}
}
//
//bool MySQLDataProvider::getAccounts(std::list<AccountData*>& pAccounts) {
//	Profile p("MySQLDataProvider::getAccounts");
//	static const String GET_ACCOUNTS_QUERY = "SELECT id, name, charname, sharedplat, status, lsaccount_id, gmspeed, hideme, suspendeduntil FROM account";
//	static const uint32 GET_ACCOUNTS_QUERY_LENGTH = GET_ACCOUNTS_QUERY.length();
//	char errorBuffer[MYSQL_ERRMSG_SIZE];
//	MYSQL_RES* result;
//	if (mDatabaseConnection->runQuery(GET_ACCOUNTS_QUERY.c_str(), GET_ACCOUNTS_QUERY_LENGTH, errorBuffer, &result)) {
//		for (int i = 0; i < mysql_num_rows(result); i++) {
//			USE_FIELD_MACROS;
//			MYSQL_ROW row = mysql_fetch_row(result);
//			AccountData* accountData = new AccountData();
//			accountData->mWorldAccountID = NEXT_FIELD_INT;
//			accountData->mName = NEXT_FIELD;
//			accountData->mCharacterName = NEXT_FIELD;
//			accountData->mSharedPlatinum = NEXT_FIELD_INT;
//			accountData->mStatus = NEXT_FIELD_INT;
//			accountData->mLoginServerAccountID = NEXT_FIELD_INT;
//			accountData->mGMSpeed = NEXT_FIELD_BOOL;
//			accountData->mHidden = NEXT_FIELD_BOOL;
//			pAccounts.push_back(accountData);
//		}
//	} else {
//		return false;
//	}
//
//	mysql_free_result(result);
//	return true;
//}

//bool MySQLDataProvider::getCharacterSelectInfo(uint32 pWorldAccountID, CharacterSelect_Struct* pCharacterSelectData) {
//	Profile p("MySQLDataProvider::getCharacterSelectInfo");
//	static const int NUM_CHARACTERS = 10;
//	static const String GET_CHARACTERS_QUERY = "SELECT name, profile, zonename, class, level FROM character_ WHERE account_id = %i order by name limit 10";
//	char errorBuffer[MYSQL_ERRMSG_SIZE];
//	char* query = 0;
//	uint32 queryLength = MakeAnyLenString(&query, GET_CHARACTERS_QUERY.c_str(), pWorldAccountID);
//	MYSQL_RES* result;
//	if (mDatabaseConnection->runQuery(query, queryLength, errorBuffer, &result)) {
//		safe_delete_array(query);
//
//		// TODO: Why is only part of this data initialised here?
//		for (int i = 0; i < NUM_CHARACTERS; i++) {
//			strcpy(pCharacterSelectData->name[i], "<none>");
//			pCharacterSelectData->zone[i] = 0;
//			pCharacterSelectData->level[i] = 0;
//			pCharacterSelectData->tutorial[i] = 0;
//			pCharacterSelectData->gohome[i] = 0;
//		}
//
//		for (int i = 0; i < mysql_num_rows(result); i++) {
//			MYSQL_ROW row = mysql_fetch_row(result);
//			strcpy(pCharacterSelectData->name[i], row[0]);
//
//			// Copy data from Player Profile.
//			PlayerProfile_Struct* playerProfile = (PlayerProfile_Struct*)row[1];
//			pCharacterSelectData->class_[i] = playerProfile->class_;
//			pCharacterSelectData->level[i] = playerProfile->level;
//			pCharacterSelectData->race[i] = playerProfile->race;
//			pCharacterSelectData->gender[i] = playerProfile->gender;
//			pCharacterSelectData->deity[i] = playerProfile->deity;
//			pCharacterSelectData->zone[i] = playerProfile->zone_id;
//			pCharacterSelectData->face[i] = playerProfile->face;
//			pCharacterSelectData->haircolor[i] = playerProfile->haircolor;
//			pCharacterSelectData->beardcolor[i] = playerProfile->beardcolor;
//			pCharacterSelectData->eyecolor2[i] = playerProfile->eyecolor2;
//			pCharacterSelectData->eyecolor1[i] = playerProfile->eyecolor1;
//			pCharacterSelectData->hairstyle[i] = playerProfile->hairstyle;
//			pCharacterSelectData->beard[i] = playerProfile->beard;
//			pCharacterSelectData->drakkin_heritage[i] = playerProfile->drakkin_heritage;
//			pCharacterSelectData->drakkin_tattoo[i] = playerProfile->drakkin_tattoo;
//			pCharacterSelectData->drakkin_details[i] = playerProfile->drakkin_details;
//			pCharacterSelectData->tutorial[i] = 0; // Disabled.
//			pCharacterSelectData->gohome[i] = 0; // Disabled.
//
//			// No equipment yet. NUDES!
//			for (int j = 0; j < 9; j++) {
//				pCharacterSelectData->equip[i][j] = 2;
//				pCharacterSelectData->cs_colors[i][j].color = 0;
//			}
//
//			pCharacterSelectData->primary[i] = 10744;
//			pCharacterSelectData->secondary[i] = 10744;
//
//		}
//	} else {
//		return false;
//	}
//	mysql_free_result(result);
//	return true;
//}

//bool MySQLDataProvider::isCharacterNameUnique(String pCharacterName) {
//	Profile p("MySQLDataProvider::isCharacterNameUnique");
//	static const String CHECK_NAME_QUERY = "SELECT name FROM character_ WHERE BINARY name = '%s'"; // BINARY makes it case sensitive.
//	char errorBuffer[MYSQL_ERRMSG_SIZE];
//	char* query = 0;
//	uint32 queryLength = MakeAnyLenString(&query, CHECK_NAME_QUERY.c_str(), pCharacterName.c_str());
//	MYSQL_RES* result;
//	if (mDatabaseConnection->runQuery(query, queryLength, errorBuffer, &result)) {
//		// Expect zero rows if name is not in use.
//		if (mysql_num_rows(result) != 0) {
//			mysql_free_result(result);
//			return false;
//		}
//	}
//	else {
//		mysql_free_result(result);
//		return false;
//	}
//	mysql_free_result(result);
//	return true;
//}

//bool MySQLDataProvider::deleteCharacter(String pCharacterName) {
//	Profile p("MySQLDataProvider::deleteCharacter");
//	// Find character by ID
//	const uint32 characterID = _getCharacterID(pCharacterName);
//	if (characterID == 0) {
//		Log::error("Character ID(%i) not found when attempting to delete %s"); // TODO: pCharacterName
//		return false;
//	}
//
//	static const String DEL_CHARACTER_QUERY = "DELETE FROM character_ WHERE id = %i";
//	char errorBuffer[MYSQL_ERRMSG_SIZE];
//	char* query = 0;
//	uint32 queryLength = MakeAnyLenString(&query, DEL_CHARACTER_QUERY.c_str(), characterID);
//	uint32 numRowsAffected = 0;
//	mDatabaseConnection->runQuery(query, queryLength, errorBuffer, 0, &numRowsAffected);
//	if (numRowsAffected == 1) {
//		Log::info("Character with ID(%i) and Name(%s) deleted successfully."); // TODO:
//		return true;
//	}
//
//	Log::error("Zero rows affected whilst trying to delete character with ID(%i) and Name(%s)"); // TODO:
//	return false;
//}

uint32 MySQLDataProvider::_getCharacterID(String pCharacterName) {
	Profile p("MySQLDataProvider::_getCharacterID");
	static const String QUERY = "SELECT id FROM character_ WHERE BINARY name = '%s'"; // BINARY makes it case sensitive.
	char errorBuffer[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	uint32 queryLength = MakeAnyLenString(&query, QUERY.c_str(), pCharacterName.c_str());
	MYSQL_RES* result;
	if (mDatabaseConnection->runQuery(query, queryLength, errorBuffer, &result)) {
		// Name not found.
		if (mysql_num_rows(result) != 1) {
			mysql_free_result(result);
			return 0;
		}
		MYSQL_ROW row = mysql_fetch_row(result);
		mysql_free_result(result);
		return static_cast<uint32>(atoi(row[0]));
	}

	mysql_free_result(result);
	return 0;
}

//bool MySQLDataProvider::createCharacter(uint32 pWorldAccountID, String pCharacterName, PlayerProfile_Struct* pProfile, ExtendedProfile_Struct* pExtendedProfile) {
//	Profile p("MySQLDataProvider::createCharacter");
//	char errbuf[MYSQL_ERRMSG_SIZE];
//	char query[256 + sizeof(PlayerProfile_Struct)* 2 + sizeof(ExtendedProfile_Struct)* 2 + 5];
//	char* end = query;
//	uint32 characterID = 0;
//
//	// construct the character_ query
//	end += sprintf(end, "INSERT INTO character_ SET account_id=%i, name='%s', timelaston=0, zonename='northqeynos', x=0, y=0, z=0, profile=\'", pWorldAccountID, pCharacterName.c_str());
//	end += mDatabaseConnection->escapeString(end, (char*)pProfile, sizeof(PlayerProfile_Struct));
//	end += sprintf(end, "\', extprofile=\'");
//	end += mDatabaseConnection->escapeString(end, (char*)pExtendedProfile, sizeof(ExtendedProfile_Struct));
//	end += sprintf(end, "\'");
//
//	if (mDatabaseConnection->runQuery(query, (uint32)(end - query), errbuf)){
//		return true;
//	}
//	return false;
//}

//// TODO: This may be better as a generic get WorldAccountID by Character Name if there are other uses.
//bool MySQLDataProvider::checkOwnership(uint32 pWorldAccountID, String pCharacterName) {
//	Profile p("MySQLDataProvider::checkOwnership");
//	static const String QUERY = "SELECT account_id FROM character_ WHERE BINARY name = '%s'"; // BINARY makes it case sensitive.
//	char errorBuffer[MYSQL_ERRMSG_SIZE];
//	char* query = 0;
//	uint32 queryLength = MakeAnyLenString(&query, QUERY.c_str(), pCharacterName.c_str());
//	MYSQL_RES* result;
//	if (mDatabaseConnection->runQuery(query, queryLength, errorBuffer, &result)) {
//		// Name not found.
//		if (mysql_num_rows(result) != 1) {
//			Log::error("Attempting to check ownership of character that doesn't exist."); // TODO:
//			mysql_free_result(result);
//			return false;
//		}
//		MYSQL_ROW row = mysql_fetch_row(result);
//		bool isOwner = pWorldAccountID == static_cast<uint32>(atoi(row[0]));
//		mysql_free_result(result);
//		return isOwner;
//	}
//
//	Log::error("Checking ownership of character failed."); // TODO:
//	mysql_free_result(result);
//	return false;
//}

bool MySQLDataProvider::loadCharacter(String pCharacterName, uint32& pCharacterID, PlayerProfile_Struct* pProfile, ExtendedProfile_Struct* pExtendedProfile) {
	Profile p("MySQLDataProvider::loadCharacter");
	static const String QUERY = "SELECT profile, extprofile, id FROM character_ WHERE BINARY name = '%s'"; // BINARY makes it case sensitive.
	char errorBuffer[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	uint32 queryLength = MakeAnyLenString(&query, QUERY.c_str(), pCharacterName.c_str());
	MYSQL_RES* result;
	if (mDatabaseConnection->runQuery(query, queryLength, errorBuffer, &result)) {
		MYSQL_ROW row = mysql_fetch_row(result);
		PlayerProfile_Struct* profile = (PlayerProfile_Struct*)row[0];
		ExtendedProfile_Struct* extendedProfile = (ExtendedProfile_Struct*)row[1];
		pCharacterID = static_cast<uint32>(atoi(row[2]));

		memcpy(pProfile, profile, sizeof(PlayerProfile_Struct));
		memcpy(pExtendedProfile, extendedProfile, sizeof(ExtendedProfile_Struct));

		mysql_free_result(result);
		return true;
	}

	Log::error("Load Character failed.");
	mysql_free_result(result);
	return true;
}

bool MySQLDataProvider::saveCharacter(uint32 pCharacterID, PlayerProfile_Struct* pProfile, ExtendedProfile_Struct* pExtendedProfile) {
	Profile p("MySQLDataProvider::saveCharacter");

	char errbuf[MYSQL_ERRMSG_SIZE];
	char query[256 + sizeof(PlayerProfile_Struct)* 2 + sizeof(ExtendedProfile_Struct)* 2 + 5];
	char* end = query;
	
	// construct the character_ query
	end += sprintf(end, "UPDATE character_ SET profile=\'");
	end += mDatabaseConnection->escapeString(end, (char*)pProfile, sizeof(PlayerProfile_Struct));
	end += sprintf(end, "\', extprofile=\'");
	end += mDatabaseConnection->escapeString(end, (char*)pExtendedProfile, sizeof(ExtendedProfile_Struct));
	end += sprintf(end, "\' WHERE id=%i", pCharacterID);

	uint32 numRowsAffected = 0;
	if (mDatabaseConnection->runQuery(query, (uint32)(end - query), errbuf, 0, &numRowsAffected)){
		return true;
	}
	return false;
}

void MySQLDataProvider::copyProfile(PlayerProfile_Struct* pProfileTo, PlayerProfile_Struct* pProfileFrom)
{

}

//bool MySQLDataProvider::createAccount(uint32 pLoginServerAccountID, String pLoginServerAccountName) {
//	Profile p("MySQLDataProvider::createAccount");
//	mStringStream.clear();
//	mStringStream << "INSERT INTO account SET lsaccount_id=" << pLoginServerAccountID << ", name='" << pLoginServerAccountName << "', time_creation=UNIX_TIMESTAMP();";
//	const String query = mStringStream.str();
//	mStringStream.clear();
//
//	uint32 queryLength = query.length();
//	char errorBuffer[MYSQL_ERRMSG_SIZE];
//	uint32 numRowsAffected = 0;
//
//	if (mDatabaseConnection->runQuery(query.c_str(), queryLength, errorBuffer, 0, &numRowsAffected)) {
//		if (numRowsAffected != 1) {
//			Log::error("[MYSQL Data Provider] Number of rows affected in createAccount was wrong.");
//			return false;
//		}
//		return true;
//	}
//
//	Log::error("[MYSQL Data Provider] Query in createAccount failed.");
//	return false;
//}
