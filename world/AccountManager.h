#pragma once

#include "Constants.h"
#include "Singleton.h"
#include "Data.h"

namespace Data {
	struct Account;
}

namespace Payload {
	namespace World {
		struct CreateCharacter;
	}
}
class Character;

class AccountManager : public Singleton<AccountManager> {
private:
	friend class Singleton<AccountManager>;
	AccountManager() {};
	~AccountManager();
	AccountManager(AccountManager const&); // Do not implement.
	void operator=(AccountManager const&); // Do not implement.
public:
	bool initialise();

	// Returns an Account by ID.
	Data::Account* getAccount(const u32 pID) const;
	bool checkOwnership(const u32 pAccountID, const String& pCharacterName);
	bool createAccount(const u32 pAccountID, const String pAccoutName);

	AccountStatus getStatus(const u32 pAccountID);
	bool create(const String& pAccountName);
	bool exists(const u32 pAccountID);

	// Returns the number of Characters belonging to the Account.
	const u32 getNumCharacters(const u32 pAccountID) const;
	bool isCharacterNameUnique(const String& pCharacterName);

	bool handleCharacterCreate(const u32 pAccountID, const String& pCharacterName, Payload::World::CreateCharacter* pPayload);
	bool deleteCharacter(const u32 pAccountID, const String& pCharacterName);
	const bool updateCharacter(const u32 pAccountID, const Character* pCharacter);

	bool ban(const String& pAccountName);
	bool removeBan(const String& pAccountName);

	bool suspend(const String& pAccountName, const u32 pSuspendUntil);
	bool removeSuspend(const String& pAccountName);
	const bool ensureAccountLoaded(const u32 pAccountID);

	const i32 getSharedPlatinum(const u32 pAccountID) const;
	const bool setSharedPlatinum(const u32 pAccountID, const i32 pPlatinum);

private:
	void _clear();
	bool _save(Data::Account* pAccountData);
	bool _save();

	Data::Account* _find(const u32 pAccountID) const;
	Data::Account* _find(const String& pAccountName) const;
	std::list<String> mCharacterNames;
	std::list<Data::Account*> mAccounts;
};