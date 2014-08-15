#pragma once

#include "Constants.h"

#include "../common/timer.h"

class EQStreamInterface;
class EQApplicationPacket;
class Zone;
class Character;
class DataStore;
class CommandHandler;
class Guild;
struct GuildMember;
struct NewSpawn_Struct;

static const String EmptyString = String();

class ZoneClientConnection {
public:
	enum ZoneConnectionStatus {
		NONE,
		ZoneEntryReceived,		// On OP_ZoneEntry
		PlayerProfileSent,
		ClientRequestZoneData,	// On OP_ReqNewZone
		ZoneInformationSent,
		ClientRequestSpawn,		// On OP_ReqClientSpawn
		Complete				// On OP_ClientReady
	};
	enum class ConnectionOrigin {
		Unknown,
		Zone,
		Character_Select
	};
public:
	ZoneClientConnection(EQStreamInterface* pStreamInterface, Zone* pZone);
	~ZoneClientConnection();
	ConnectionOrigin getConnectionOrigin() { return mConnectionOrigin; }
	bool isConnected();
	bool isReadyForZoneIn() { return mZoneConnectionStatus == Complete; }
	Character* getCharacter() { return mCharacter; }
	void update();
	bool _handlePacket(const EQApplicationPacket* pPacket);
	void _handleZoneEntry(const EQApplicationPacket* pPacket);
	void _handleRequestClientSpawn(const EQApplicationPacket* pPacket);

	void dropConnection();

	void sendPosition();
	void sendMessage(MessageType pType, String pMessage);
	void sendSimpleMessage(MessageType pType, StringID pStringID);
	void sendSimpleMessage(MessageType pType, StringID pStringID, String pParameter0, String pParameter1 = EmptyString, String pParameter2 = EmptyString, String pParameter3 = EmptyString, String pParameter4 = EmptyString, String pParameter5 = EmptyString, String pParameter6 = EmptyString, String pParameter7 = EmptyString, String pParameter8 = EmptyString, String pParameter9 = EmptyString);
	void sendAppearance(uint16 pType, uint32 pParameter);
	void sendHPUpdate();

	void sendExperienceUpdate();
	void sendExperienceGain();
	void sendExperienceLoss();
	void sendLevelGain();
	void sendLevelLost();
	void sendLevelAppearance();
	void sendLevelUpdate();
	void sendStats();
	void sendWhoResults(std::list<Character*>& pMatches);

	void sendChannelMessage(const ChannelID pChannel, const String& pSenderName, const String& pMessage);
	void sendTell(const String& pSenderName, const String& pMessage);
	void sendGroupMessage(const String& pSenderName, const String& pMessage);
	void sendGuildMessage(const String& pSenderName, const String& pMessage);

	void sendGroupInvite(const String pFromCharacterName);
	void sendGroupCreate();
	void sendGroupLeaderChange(const String pCharacterName);
	void sendGroupAcknowledge();
	void sendGroupFollow(const String& pLeaderCharacterName, const String& pMemberCharacterName);
	void sendGroupJoin(const String& pCharacterName);
	void sendGroupUpdate(std::list<String>& pGroupMemberNames);
	void sendGroupDisband();
	void sendGroupLeave(const String& pLeavingCharacterName);

	void sendRequestZoneChange(uint32 pZoneID, uint16 pInstanceID);
	void sendZoneChange(uint32 pZoneID, uint16 pInstanceID);

	void sendGuildRank();
	void sendGuildInvite(String pInviterName, GuildID pGuildID);
	void sendGuildMOTD(const String& pMOTD, const String& pMOTDSetByName);
	void sendGuildMOTDReply(const String& pMOTD, const String& pMOTDSetByName);
	void sendGuildMembers(const std::list<GuildMember*>& pGuildMembers);
	void sendGuildURL(const String& pURL);
	void sendGuildChannel(const String& pChannel);

	void populateSpawnStruct(NewSpawn_Struct* pSpawn);
	EQApplicationPacket* makeCharacterSpawnPacket(); // Caller is responsible for memory deallocation.
	EQApplicationPacket* makeCharacterPositionUpdate(); // Caller is responsible for memory deallocation.

	void sendPacket(EQApplicationPacket* pPacket);

private:

	void _sendTimeOfDay();

	void _sendPlayerProfile();
	void _sendZoneEntry();
	void _sendZoneSpawns();
	void _sendTributeUpdate();
	void _sendInventory();
	void _sendWeather();
	void _sendGuildNames();

	void _sendPreLogOutReply();
	void _sendLogOutReply();
	

	void _sendDoors();
	void _sendObjects();
	void _sendZonePoints();
	void _sendAAStats();
	void _sendZoneServerReady();
	void _sendExpZoneIn();
	void _sendWorldObjectsSent();
	void _handleClientUpdate(const EQApplicationPacket* pPacket);
	void _handleSpawnAppearance(const EQApplicationPacket* pPacket);
	void _handleCamp(const EQApplicationPacket* pPacket);
	void _handleChannelMessage(const EQApplicationPacket* pPacket);
	void _handleLogOut(const EQApplicationPacket* pPacket);
	void _handleDeleteSpawn(const EQApplicationPacket* pPacket);
	void _handleRequestNewZoneData(const EQApplicationPacket* pPacket);

	void _sendNewZoneData();
	void _handleClientReady(const EQApplicationPacket* pPacket);
	void _handleSendAATable(const EQApplicationPacket* pPacket);
	void _handleUpdateAA(const EQApplicationPacket* pPacket);
	void _handleTarget(const EQApplicationPacket* pPacket);
	void _handleTGB(const EQApplicationPacket* pPacket);
	void _handleEmote(const EQApplicationPacket* pPacket);
	void _handleAnimation(const EQApplicationPacket* pPacket);
	void _handleWhoAllRequest(const EQApplicationPacket* pPacket);
	void _handleGroupInvite(const EQApplicationPacket* pPacket);
	void _handleGroupFollow(const EQApplicationPacket* pPacket);
	void _handleGroupCanelInvite(const EQApplicationPacket* pPacket);
	void _handleGroupDisband(const EQApplicationPacket* pPacket);
	void _handleGroupMakeLeader(const EQApplicationPacket* pPacket);
	void _handleZoneChange(const EQApplicationPacket* pPacket);

	void _handleGuildCreate(const EQApplicationPacket* pPacket);
	void _handleGuildDelete(const EQApplicationPacket* pPacket);
	void _handleGuildInvite(const EQApplicationPacket* pPacket);
	void _handleGuildInviteAccept(const EQApplicationPacket* pPacket);
	void _handleGuildRemove(const EQApplicationPacket* pPacket);
	void _handleSetGuildMOTD(const EQApplicationPacket* pPacket);
	void _handleGetGuildMOTD(const EQApplicationPacket* pPacket);
	void _handleSetGuildURLOrChannel(const EQApplicationPacket* pPacket);
	void _handleSetGuildPublicNote(const EQApplicationPacket* pPacket);

	ConnectionOrigin mConnectionOrigin;
	bool mConnected;
	Timer mForceSendPositionTimer;
	EQStreamInterface* mStreamInterface;
	Zone* mZone;
	Character* mCharacter;
	ZoneConnectionStatus mZoneConnectionStatus;
	CommandHandler* mCommandHandler; // For now every connection has their own.

	public:
		static void initalise();
		static void deinitialise();
	private:
	
	static EQApplicationPacket* mGroupJoinPacket;
	static EQApplicationPacket* mGroupLeavePacket;
	static EQApplicationPacket* mGroupDisbandPacket;
	static EQApplicationPacket* mGroupLeaderChangePacket;
	static EQApplicationPacket* mGroupUpdateMembersPacket;
};