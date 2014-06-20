#ifdef LUA_EQEMU

#include "lua.hpp"
#include <luabind/luabind.hpp>

#include "entity.h"
#include "lua_entity.h"
#include "lua_mob.h"
#include "lua_client.h"
#include "lua_npc.h"
#include "lua_corpse.h"
#include "lua_object.h"
#include "lua_door.h"

bool Lua_Entity::IsClient() {
	Lua_Safe_Call_Bool();
	return self->isClient();
}

bool Lua_Entity::IsNPC() {
	Lua_Safe_Call_Bool();
	return self->isNPC();
}

bool Lua_Entity::IsMob() {
	Lua_Safe_Call_Bool();
	return self->isMOB();
}

bool Lua_Entity::IsMerc() {
	Lua_Safe_Call_Bool();
	return self->isMerc();
}

bool Lua_Entity::IsCorpse() {
	Lua_Safe_Call_Bool();
	return self->isCorpse();
}

bool Lua_Entity::IsPlayerCorpse() {
	Lua_Safe_Call_Bool();
	return self->isPlayerCorpse();
}

bool Lua_Entity::IsNPCCorpse() {
	Lua_Safe_Call_Bool();
	return self->isNPCCorpse();
}

bool Lua_Entity::IsObject() {
	Lua_Safe_Call_Bool();
	return self->isObject();
}

bool Lua_Entity::IsDoor() {
	Lua_Safe_Call_Bool();
	return self->isDoor();
}

bool Lua_Entity::IsTrap() {
	Lua_Safe_Call_Bool();
	return self->isTrap();
}

bool Lua_Entity::IsBeacon() {
	Lua_Safe_Call_Bool();
	return self->isBeacon();
}

int Lua_Entity::GetID() {
	Lua_Safe_Call_Bool();
	return self->getID();
}

Lua_Client Lua_Entity::CastToClient() {
	void *d = GetLuaPtrData();
	Client *m = reinterpret_cast<Client*>(d);
	return Lua_Client(m);
}

Lua_NPC Lua_Entity::CastToNPC() {
	void *d = GetLuaPtrData();
	NPC *m = reinterpret_cast<NPC*>(d);
	return Lua_NPC(m);
}

Lua_Mob Lua_Entity::CastToMob() {
	void *d = GetLuaPtrData();
	Mob *m = reinterpret_cast<Mob*>(d);
	return Lua_Mob(m);
}

Lua_Corpse Lua_Entity::CastToCorpse() {
	void *d = GetLuaPtrData();
	Corpse *m = reinterpret_cast<Corpse*>(d);
	return Lua_Corpse(m);
}

Lua_Object Lua_Entity::CastToObject() {
	void *d = GetLuaPtrData();
	Object *m = reinterpret_cast<Object*>(d);
	return Lua_Object(m);
}

Lua_Door Lua_Entity::CastToDoor() {
	void *d = GetLuaPtrData();
	Doors* m = reinterpret_cast<Doors*>(d);
	return Lua_Door(m);
}

luabind::scope lua_register_entity() {
	return luabind::class_<Lua_Entity>("Entity")
		.def(luabind::constructor<>())
		.property("null", &Lua_Entity::Null)
		.property("valid", &Lua_Entity::Valid)
		.def("IsClient", &Lua_Entity::IsClient)
		.def("IsNPC", &Lua_Entity::IsNPC)
		.def("IsMob", &Lua_Entity::IsMob)
		.def("IsMerc", &Lua_Entity::IsMerc)
		.def("IsCorpse", &Lua_Entity::IsCorpse)
		.def("IsPlayerCorpse", &Lua_Entity::IsPlayerCorpse)
		.def("IsNPCCorpse", &Lua_Entity::IsNPCCorpse)
		.def("IsObject", &Lua_Entity::IsObject)
		.def("IsDoor", &Lua_Entity::IsDoor)
		.def("IsTrap", &Lua_Entity::IsTrap)
		.def("IsBeacon", &Lua_Entity::IsBeacon)
		.def("GetID", &Lua_Entity::GetID)
		.def("CastToClient", &Lua_Entity::CastToClient)
		.def("CastToNPC", &Lua_Entity::CastToNPC)
		.def("CastToMob", &Lua_Entity::CastToMob)
		.def("CastToCorpse", &Lua_Entity::CastToCorpse)
		.def("CastToObject", &Lua_Entity::CastToObject)
		.def("CastToDoor", &Lua_Entity::CastToDoor);
}

#endif
