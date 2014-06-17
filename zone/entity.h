/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2003 EQEMu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#ifndef ENTITY_H
#define ENTITY_H
#include <unordered_map>
#include <queue>

#include "../common/types.h"
#include "../common/linked_list.h"
#include "../common/servertalk.h"
#include "../common/bodytypes.h"
#include "../common/eq_constants.h"

#include "zonedb.h"
#include "zonedump.h"
#include "zonedbasync.h"
#include "QGlobals.h"

class EQApplicationPacket;

class Client;
class Mob;
class NPC;
class Merc;
class Corpse;
class Beacon;
class Petition;
class Object;
class Group;
class Raid;
class Doors;
class Trap;
class Entity;
class EntityList;

extern EntityList entity_list;

class Entity
{
public:
	Entity();
	virtual ~Entity();

	virtual bool isClient() const { return false; }
	virtual bool isNPC() const { return false; }
	virtual bool isMOB() const { return false; }
	virtual bool isMerc() const { return false; }
	virtual bool isCorpse() const { return false; }
	virtual bool isPlayerCorpse() const { return false; }
	virtual bool isNPCCorpse() const { return false; }
	virtual bool isObject() const { return false; }
	virtual bool isDoor() const { return false; }
	virtual bool isTrap() const { return false; }
	virtual bool isBeacon() const { return false; }

	virtual bool process() { return false; }
	virtual bool save() { return true; }
	virtual void depop(bool StartSpawnTimer = false) {}

	Client* castToClient();
	const Client* castToClient() const;
	NPC* castToNPC();
	const NPC* castToNPC() const;
	Mob* castToMOB();
	const Mob* castToMOB() const;
	Merc* castToMerc();
	const Merc* castToMerc() const;
	Corpse* castToCorpse();
	const Corpse* castToCorpse() const;
	Object* castToObject();
	const Object* castToObject() const;
	Doors* castToDoors();
	const Doors* castToDoors() const;
	Trap* castToTrap();
	const Trap* castToTrap() const;
	Beacon* castToBeacon();
	const Beacon* castToBeacon() const;

	inline const uint16& getID() const { return mID; }

	virtual const char* getName() { return ""; }
	// TODO: Look more closely at DBAWComplete and checkCoordLosNoZLeaps before modifying.
	virtual void DBAWComplete(uint8 workpt_b1, DBAsyncWork* dbaw) { mDBAsyncWorkID = 0; }
	bool checkCoordLosNoZLeaps(float cur_x, float cur_y, float cur_z, float trg_x, float trg_y, float trg_z, float perwalk=1);

protected:
	friend class EntityList;
	inline virtual void setID(uint16 pID) { mID = pID; }
	uint32 mDBAsyncWorkID;
private:
	uint16 mID;
};

class EntityList
{
public:
	struct Area {
		Area() : mID(0), mType(0), mMinX(0.0f), mMaxX(0.0f), mMinY(0.0f), mMaxY(0.0f), mMinZ(0.0f), mMaxZ(0.0f) {};
		int mID;
		int mType;
		float mMinX;
		float mMaxX;
		float mMinY;
		float mMaxY;
		float mMinZ;
		float mMaxZ;
	};

	EntityList();
	~EntityList();

	Entity* getID(uint16 pID);
	Mob* getMOB(uint16 pID);
	
	bool isMOBSpawnedByNpcTypeID(uint32 pID);

	// Search for MOB 
	Mob* getMOB(const char* pName);
	Mob* getMOBByNpcTypeID(uint32 pID);
	Mob* getTargetForVirus(Mob* pSpreader);
	inline Mob* getMOBByID(uint16 pID) { return getMOB(pID); }

	NPC* getNPCByNPCTypeID(uint32 pNPCTypeID);
	inline NPC* getNPCByID(uint16 pID) { return mNPCs.count(pID) ? mNPCs.at(pID) : nullptr; }
	inline Merc* getMercByID(uint16 pID) { return mMercs.count(pID) ? mMercs.at(pID) : nullptr; }

	// Search for Client
	Client* getClientByName(const char* pName);
	Client* getClientByAccountID(uint32 pAccountID);
	Client* getClientByCharacterID(uint32 pCharacterID);
	Client* getClientByWID(uint32 pWID);
	Client* getRandomClient(float pX, float pY, float pZ, float pDistance, Client* pExcludeClient = nullptr);
	inline Client* getClientByID(uint16 pID) { return mClients.count(pID) ? mClients.at(pID) : nullptr; }

	Group* getGroupByMOB(Mob* pMOB);
	Group* getGroupByClient(Client* pClient);
	// Returns the Group which has ID pGroupID, or null where it does not exist.
	Group* getGroupByID(uint32 pGroupID);
	// Returns the Group which has leader named pLeaderName, or null where it does not exist.
	Group* getGroupByLeaderName(const char* pLeaderName);

	Raid* getRaidByClient(Client* pClient);
	Raid* getRaidByID(uint32 pRaidID);

	Corpse* getCorpseByOwner(Client* pClient);
	Corpse* getCorpseByOwnerWithinRange(Client* pClient, Mob* pCenter, int pRange);
	Corpse* getCorpseByDatabaseID(uint32 pDatabaseID);
	Corpse* getCorpseByName(const char* pName);
	inline Corpse* getCorpseByID(uint16 pID) { return mCorpses.count(pID) ? mCorpses.at(pID) : nullptr; }

	Spawn2* GetSpawnByID(uint32 id);

	Client* FindCorpseDragger(uint16 CorpseID);

	inline Object *GetObjectByID(uint16 id)
		{ return mObjects.count(id) ? mObjects.at(id) : nullptr; }
	Object *GetObjectByDBID(uint32 id);
	inline Doors *GetDoorsByID(uint16 id)
		{ return mDoors.count(id) ? mDoors.at(id) : nullptr; }
	Doors *GetDoorsByDoorID(uint32 id);
	Doors *GetDoorsByDBID(uint32 id);
	void RemoveAllCorpsesByCharID(uint32 charid);
	void RemoveCorpseByDBID(uint32 dbid);
	int RezzAllCorpsesByCharID(uint32 charid);
	bool IsMobInZone(Mob *who);
	void ClearClientPetitionQueue();
	bool CanAddHateForMob(Mob *p);
	void	SendGuildMOTD(uint32 guild_id);
	void	SendGuildSpawnAppearance(uint32 guild_id);
	void	SendGuildMembers(uint32 guild_id);
	void	RefreshAllGuildInfo(uint32 guild_id);
	void	SendGuildList();
	void	CheckGroupList (const char *fname, const int fline);
	void	GroupProcess();
	void	RaidProcess();
	void	DoorProcess();
	void	ObjectProcess();
	void	CorpseProcess();
	void	MobProcess();
	void	TrapProcess();
	void	BeaconProcess();
	void	ProcessMove(Client *c, float x, float y, float z);
	void	ProcessMove(NPC *n, float x, float y, float z);
	void	AddArea(int id, int type, float min_x, float max_x, float min_y, float max_y, float min_z, float max_z);
	void	RemoveArea(int id);
	void	ClearAreas();
	void	ProcessProximitySay(const char *Message, Client *c, uint8 language = 0);
	void	SendAATimer(uint32 charid,UseAA_Struct* uaa);
	Doors *FindDoor(uint8 door_id);
	Object *FindObject(uint32 object_id);
	Object*	FindNearbyObject(float x, float y, float z, float radius);
	bool	MakeDoorSpawnPacket(EQApplicationPacket* app, Client *client);
	bool	MakeTrackPacket(Client* client);
	void	SendTraders(Client* client);
	void	AddClient(Client*);
	void	AddNPC(NPC*, bool SendSpawnPacket = true, bool dontqueue = false);
	void	AddMerc(Merc*, bool SendSpawnPacket = true, bool dontqueue = false);
	void	AddCorpse(Corpse* pc, uint32 in_id = 0xFFFFFFFF);
	void	AddObject(Object*, bool SendSpawnPacket = true);
	void	AddGroup(Group*);
	void	AddGroup(Group*, uint32 id);
	void	AddRaid(Raid *raid);
	void	AddRaid(Raid*, uint32 id);
	void	AddDoor(Doors* door);
	void	AddTrap(Trap* trap);
	void	AddBeacon(Beacon *beacon);
	void	AddProximity(NPC *proximity_for);
	void	Clear();
	bool	RemoveMob(uint16 delete_id);
	bool	RemoveMob(Mob* delete_mob);
	bool	RemoveClient(uint16 delete_id);
	bool	RemoveClient(Client* delete_client);
	bool	RemoveNPC(uint16 delete_id);
	bool	RemoveMerc(uint16 delete_id);
	bool	RemoveGroup(uint32 delete_id);
	bool	RemoveRaid(uint32 delete_id);
	bool	RemoveCorpse(uint16 delete_id);
	bool	RemoveDoor(uint16 delete_id);
	bool	RemoveTrap(uint16 delete_id);
	bool	RemoveObject(uint16 delete_id);
	bool	RemoveProximity(uint16 delete_npc_id);
	void	RemoveAllMobs();
	void	RemoveAllClients();
	void	RemoveAllNPCs();
	void	RemoveAllMercs();
	void	RemoveAllGroups();
	void	RemoveAllCorpses();
	void	RemoveAllDoors();
	void	DespawnAllDoors();
	void	RespawnAllDoors();
	void	RemoveAllTraps();
	void	RemoveAllObjects();
	void	RemoveAllLocalities();
	void	RemoveAllRaids();
	void	DestroyTempPets(Mob *owner);
	Entity *GetEntityMob(uint16 id);
	Entity *GetEntityMerc(uint16 id);
	Entity *GetEntityDoor(uint16 id);
	Entity *GetEntityObject(uint16 id);
	Entity *GetEntityCorpse(uint16 id);
	Entity *GetEntityTrap(uint16 id);
	Entity *GetEntityBeacon(uint16 id);
	Entity *GetEntityMob(const char *name);
	Entity *GetEntityCorpse(const char *name);

	void DescribeAggro(Client *towho, NPC *from_who, float dist, bool verbose);

	void	Message(uint32 to_guilddbid, uint32 type, const char* message, ...);
	void	MessageStatus(uint32 to_guilddbid, int to_minstatus, uint32 type, const char* message, ...);
	void	MessageClose(Mob* sender, bool skipsender, float dist, uint32 type, const char* message, ...);
	void	Message_StringID(Mob *sender, bool skipsender, uint32 type, uint32 string_id, const char* message1=0,const char* message2=0,const char* message3=0,const char* message4=0,const char* message5=0,const char* message6=0,const char* message7=0,const char* message8=0,const char* message9=0);
	void	FilteredMessage_StringID(Mob *sender, bool skipsender, uint32 type, eqFilterType filter, uint32 string_id, const char* message1=0,const char* message2=0,const char* message3=0,const char* message4=0,const char* message5=0,const char* message6=0,const char* message7=0,const char* message8=0,const char* message9=0);
	void	MessageClose_StringID(Mob *sender, bool skipsender, float dist, uint32 type, uint32 string_id, const char* message1=0,const char* message2=0,const char* message3=0,const char* message4=0,const char* message5=0,const char* message6=0,const char* message7=0,const char* message8=0,const char* message9=0);
	void	FilteredMessageClose_StringID(Mob *sender, bool skipsender, float dist, uint32 type, eqFilterType filter, uint32 string_id, const char* message1=0,const char* message2=0,const char* message3=0,const char* message4=0,const char* message5=0,const char* message6=0,const char* message7=0,const char* message8=0,const char* message9=0);
	void	ChannelMessageFromWorld(const char* from, const char* to, uint8 chan_num, uint32 guilddbid, uint8 language, const char* message);
	void	ChannelMessage(Mob* from, uint8 chan_num, uint8 language, const char* message, ...);
	void	ChannelMessage(Mob* from, uint8 chan_num, uint8 language, uint8 lang_skill, const char* message, ...);
	void	ChannelMessageSend(Mob* to, uint8 chan_num, uint8 language, const char* message, ...);
	void	SendZoneSpawns(Client*);
	void	SendZonePVPUpdates(Client *);
	void	SendZoneSpawnsBulk(Client* client);
	void	Save();
	void	SendZoneCorpses(Client*);
	void	SendZoneCorpsesBulk(Client*);
	void	SendZoneObjects(Client* client);
	void	SendZoneAppearance(Client *c);
	void	SendNimbusEffects(Client *c);
	void	SendUntargetable(Client *c);
	void	DuelMessage(Mob* winner, Mob* loser, bool flee);
	void	QuestJournalledSayClose(Mob *sender, Client *QuestIntiator, float dist, const char* mobname, const char* message);
	void	GroupMessage(uint32 gid, const char *from, const char *message);
	void	ExpeditionWarning(uint32 minutes_left);

	void	RemoveFromTargets(Mob* mob, bool RemoveFromXTargets = false);
	void	RemoveFromXTargets(Mob* mob);
	void	RemoveFromAutoXTargets(Mob* mob);
	void	ReplaceWithTarget(Mob* pOldMob, Mob*pNewTarget);
	void	QueueCloseClients(Mob* sender, const EQApplicationPacket* app, bool ignore_sender=false, float dist=200, Mob* SkipThisMob = 0, bool ackreq = true,eqFilterType filter=FilterNone);
	void	QueueClients(Mob* sender, const EQApplicationPacket* app, bool ignore_sender=false, bool ackreq = true);
	void	QueueClientsStatus(Mob* sender, const EQApplicationPacket* app, bool ignore_sender = false, uint8 minstatus = 0, uint8 maxstatus = 0);
	void	QueueClientsGuild(Mob* sender, const EQApplicationPacket* app, bool ignore_sender = false, uint32 guildeqid = 0);
	void	QueueClientsGuildBankItemUpdate(const GuildBankItemUpdate_Struct *gbius, uint32 GuildID);
	void	QueueClientsByTarget(Mob* sender, const EQApplicationPacket* app, bool iSendToSender = true, Mob* SkipThisMob = 0, bool ackreq = true,
						bool HoTT = true, uint32 ClientVersionBits = 0xFFFFFFFF);

	void	QueueClientsByXTarget(Mob* sender, const EQApplicationPacket* app, bool iSendToSender = true);
	void	QueueToGroupsForNPCHealthAA(Mob* sender, const EQApplicationPacket* app);
	void	QueueManaged(Mob* sender, const EQApplicationPacket* app, bool ignore_sender=false, bool ackreq = true);

	void	AEAttack(Mob *attacker, float dist, int Hand = 13, int count = 0, bool IsFromSpell = false);
	void	AETaunt(Client *caster, float range = 0);
	void	AESpell(Mob *caster, Mob *center, uint16 spell_id, bool affect_caster = true, int16 resist_adjust = 0);
	void	MassGroupBuff(Mob *caster, Mob *center, uint16 spell_id, bool affect_caster = true);
	void	AEBardPulse(Mob *caster, Mob *center, uint16 spell_id, bool affect_caster = true);

	void	RadialSetLogging(Mob *around, bool enabled, bool clients, bool non_clients, float range = 0);

	//trap stuff
	Mob*	GetTrapTrigger(Trap* trap);
	void	SendAlarm(Trap* trap, Mob* currenttarget, uint8 kos);
	Trap*	FindNearbyTrap(Mob* searcher, float max_dist);

	void	AddHealAggro(Mob* target, Mob* caster, uint16 thedam);
	Mob*	FindDefenseNPC(uint32 npcid);
	void	OpenDoorsNear(NPC* opener);
	void	UpdateWho(bool iSendFullUpdate = false);
	void	SendPositionUpdates(Client* client, uint32 cLastUpdate = 0, float range = 0, Entity* alwayssend = 0, bool iSendEvenIfNotChanged = false);
	char*	MakeNameUnique(char* name);
	static char* RemoveNumbers(char* name);
	void	SignalMobsByNPCID(uint32 npc_type, int signal_id);
	void	CountNPC(uint32* NPCCount, uint32* NPCLootCount, uint32* gmspawntype_count);
	void	RemoveEntity(uint16 id);
	void	SendPetitionToAdmins(Petition* pet);
	void	SendPetitionToAdmins();
	void	AddLootToNPCS(uint32 item_id, uint32 count);

	void	ListNPCs(Client* client, const char* arg1 = 0, const char* arg2 = 0, uint8 searchtype = 0);
	void	ListNPCCorpses(Client* client);
	void	ListPlayerCorpses(Client* client);
	void	FindPathsToAllNPCs();
	int32	DeleteNPCCorpses();
	int32	DeletePlayerCorpses();
	void	WriteEntityIDs();
	void	HalveAggro(Mob* who);
	void	DoubleAggro(Mob* who);
	void	Evade(Mob *who);
	void	UpdateHoTT(Mob* target);

	void	Process();
	void	ClearAggro(Mob* targ);
	void	ClearFeignAggro(Mob* targ);
	void	ClearZoneFeignAggro(Client* targ);
	void	AggroZone(Mob* who, int hate = 0);

	bool	Fighting(Mob* targ);
	void	RemoveFromHateLists(Mob* mob, bool settoone = false);
	void	RemoveDebuffs(Mob* caster);


	void	MessageGroup(Mob* sender, bool skipclose, uint32 type, const char* message, ...);

	void	LimitAddNPC(NPC *npc);
	void	LimitRemoveNPC(NPC *npc);
	bool	LimitCheckType(uint32 npc_type, int count);
	bool	LimitCheckGroup(uint32 spawngroup_id, int count);
	bool	LimitCheckBoth(uint32 npc_type, uint32 spawngroup_id, int group_count, int type_count);
	bool	LimitCheckName(const char* npc_name);

	void	CheckClientAggro(Client *around);
	Mob*	AICheckCloseAggro(Mob* sender, float iAggroRange, float iAssistRange);
	int	GetHatedCount(Mob *attacker, Mob *exclude);
	void	AIYellForHelp(Mob* sender, Mob* attacker);
	bool	AICheckCloseBeneficialSpells(NPC* caster, uint8 iChance, float iRange, uint16 iSpellTypes);
	bool	Merc_AICheckCloseBeneficialSpells(Merc* caster, uint8 iChance, float iRange, uint32 iSpellTypes);
	Mob*	GetTargetForMez(Mob* caster);
	uint32	CheckNPCsClose(Mob *center);

	Corpse* GetClosestCorpse(Mob* sender, const char *Name);
	NPC* GetClosestBanker(Mob* sender, uint32 &distance);
	void	CameraEffect(uint32 duration, uint32 intensity);
	Mob*	GetClosestMobByBodyType(Mob* sender, bodyType BodyType);
	void	ForceGroupUpdate(uint32 gid);
	void	SendGroupLeave(uint32 gid, const char *name);
	void	SendGroupJoin(uint32 gid, const char *name);

	void	SaveAllClientsTaskState();
	void	ReloadAllClientsTaskState(int TaskID=0);

	uint16	CreateGroundObject(uint32 itemid, float x, float y, float z, float heading, uint32 decay_time = 300000);
	uint16	CreateGroundObjectFromModel(const char *model, float x, float y, float z, float heading, uint8 type = 0x00, uint32 decay_time = 0);
	uint16	CreateDoor(const char *model, float x, float y, float z, float heading, uint8 type = 0, uint16 size = 100);
	void	ZoneWho(Client *c, Who_All_Struct* Who);
	void	UnMarkNPC(uint16 ID);

	void	GateAllClients();
	void	SignalAllClients(uint32 data);
	void	UpdateQGlobal(uint32 qid, QGlobal newGlobal);
	void	DeleteQGlobal(std::string name, uint32 npcID, uint32 charID, uint32 zoneID);
	void	SendFindableNPCList(Client *c);
	void	UpdateFindableNPCState(NPC *n, bool Remove);
	void	HideCorpses(Client *c, uint8 CurrentMode, uint8 NewMode);

	void GetMobList(std::list<Mob*> &m_list);
	void GetNPCList(std::list<NPC*> &n_list);
	void GetMercList(std::list<Merc*> &n_list);
	void GetClientList(std::list<Client*> &c_list);
	void GetCorpseList(std::list<Corpse*> &c_list);
	void GetObjectList(std::list<Object*> &o_list);
	void GetDoorsList(std::list<Doors*> &d_list);
	void GetSpawnList(std::list<Spawn2*> &d_list);
	void GetTargetsForConeArea(Mob *start, uint32 radius, uint32 height, std::list<Mob*> &m_list);

	void	DepopAll(int NPCTypeID, bool StartSpawnTimer = true);

	uint16 GetFreeID();
	void RefreshAutoXTargets(Client *c);
	void RefreshClientXTargets(Client *c);

protected:
	friend class Zone;
	void	Depop(bool StartSpawnTimer = false);

private:
	void	AddToSpawnQueue(uint16 entityid, NewSpawn_Struct** app);
	void	CheckSpawnQueue();

	//used for limiting spawns
	class SpawnLimitRecord { public: uint32 spawngroup_id; uint32 npc_type; };
	std::map<uint16, SpawnLimitRecord> npc_limit_list;		//entity id -> npc type

	uint32	tsFirstSpawnOnQueue; // timestamp that the top spawn on the spawnqueue was added, should be 0xFFFFFFFF if queue is empty
	uint32	NumSpawnsOnQueue;
	LinkedList<NewSpawn_Struct*> SpawnQueue;

	std::unordered_map<uint16, Client*> mClients;
	std::unordered_map<uint16, Mob*> mMOBs;
	std::unordered_map<uint16, NPC*> mNPCs;
	std::unordered_map<uint16, Merc*> mMercs;
	std::unordered_map<uint16, Corpse*> mCorpses;
	std::unordered_map<uint16, Object*> mObjects;
	std::unordered_map<uint16, Doors*> mDoors;
	std::unordered_map<uint16, Trap*> mTraps;
	std::unordered_map<uint16, Beacon*> mBeacons;
	std::list<NPC *> proximity_list;
	std::list<Group *> mGroups;
	std::list<Raid *> mRaids;
	std::list<Area> mAreas;
	std::queue<uint16> free_ids;

};

class BulkZoneSpawnPacket {
public:
	BulkZoneSpawnPacket(Client* iSendTo, uint32 iMaxSpawnsPerPacket);	// 0 = send zonewide
	virtual ~BulkZoneSpawnPacket();

	bool	AddSpawn(NewSpawn_Struct* ns);
	void	SendBuffer();	// Sends the buffer and cleans up everything - can safely re-use the object after this function call (no need to free and do another new)
private:
	uint32	pMaxSpawnsPerPacket;
	uint32	index;
	NewSpawn_Struct* data;
	Client* pSendTo;
};

#endif

