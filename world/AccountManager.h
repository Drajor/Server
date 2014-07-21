#pragma once

#include "../common/types.h"
#include <string>
#include <list>

struct AccountData;
class DataStore;

class AccountManager {
public:
	AccountManager(DataStore* pDataStore);
	~AccountManager();
	bool initialise();

	// Lookup World Account ID associated with Login Server Account ID. 0 = Not found.
	uint32 getWorldAccountID(uint32 pLoginServerAccountID);
	uint32 getStatusFromLoginServerID(uint32 pLoginServerAccountID);
	bool accountExists(uint32 pLoginServerAccountID);
	bool createAccount(uint32 pLoginServerAccountID, std::string pLoginServerAccountName);
private:
	void _clearAccounts();

	std::list<AccountData*> mAccounts;
	DataStore* mDataStore;
};