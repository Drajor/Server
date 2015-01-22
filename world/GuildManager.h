#pragma once

#include "Constants.h"
#include "Character.h"

#include "Types.h"
#include <list>

// For the #guildsearch command.
struct GuildSearchEntry {
	u32 mID;
	String mName;
};
typedef std::list<GuildSearchEntry> GuildSearchResults;

class IDataStore;
class IlogFactory;
class ILog;
class ZoneManager;
class Guild;
class GuildMember;

namespace Data {
	struct Guild;
}

class GuildManager {
public:
	~GuildManager();

	const bool initialise(IDataStore* pDataStore, ILogFactory* pLogFactory);

	inline const bool exists(const String& pGuildName) const { return search(pGuildName) != nullptr; }

	// Character Events
	void onConnect(Character* pCharacter, const u32 pGuildID);
	void onEnterZone(Character* pCharacter);
	void onLeaveZone(Character* pCharacter);
	void onCamp(Character* pCharacter);
	void onLinkdead(Character* pCharacter);
	void onLevelChange(Character* pCharacter);

	// Character is creating a guild.
	const bool onCreate(Character* pCharacter, const String& pGuildName);

	// Character is deleting a guild.
	const bool onDelete(Character* pCharacter);

	// Character is inviting another Character into guild.
	const bool onInvite(Character* pInviter, Character* pInvitee);

	// Character has accepted a guild invite.
	const bool onInviteAccept(Character* pCharacter);

	// Character is promoting a Character.
	const bool onPromote(Character* pPromoter, Character* pPromotee);

	// Character is demoting a Character.
	const bool onDemote(Character* pCharacter, Character* pDemotee);

	// Character is removing another Character from a guild.
	const bool onRemove(Character* pRemover, const String& pRemoveeName);

	// Character is leaving their guild.
	const bool onLeave(Character* pCharacter);
	
	void onMessage(Character* pCharacter, const String& pMessage);

	const bool onSetMOTD(Character* pCharacter, const String& pMOTD);

	const bool onGetMOTD(Character* pCharacter);

	const bool onSetURL(Character* pCharacter, const String& pURL);
	const bool onSetChannel(Character* pCharacter, const String& pChannel);

	const bool onSetPublicNote(Character* pCharacter, const String& pCharacterName, const String& pPublicNote);
		
	const bool onSetFlags(Character* pCharacter, const String& pCharacterName, const bool pBanker, const bool pAlt);
	//const bool handleSetAlt(Character* pCharacter, const String& pAltName, const bool pAlt);

	const bool onMakeLeader(Character* pCharacter, const String& pLeaderName);

	const bool onStatusRequest(Character* pCharacter, const String& pCharacterName);

	inline const std::list<Guild*>& getGuilds() const { return mGuilds; }

	GuildSearchResults getAllGuilds();

private:

	bool mInitialised = false;
	ILogFactory* mLogFactory = nullptr;
	ILog* mLog = nullptr;
	IDataStore* mDataStore = nullptr;

	std::list<Data::Guild*> mData;
	std::list<Guild*> mGuilds;

	void _changeRank(GuildMember* pMember, const GuildRank pNewRank);
	
	// Returns a Guild, searching by name.
	Guild* search(const String& pGuildName) const;

	Guild* _findByID(const u32 pID);
	GuildMember* _findByCharacterName(const String& pCharacterName);
	
	u32 mNextID = 0;
	const u32 getNextGuildID();
	
	const bool save();
};