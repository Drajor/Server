
#include "../debug.h"
#include "Underfoot.h"
#include "../opcodemgr.h"
#include "../logsys.h"
#include "../EQStreamIdent.h"
#include "../crc32.h"

#include "../eq_packet_structs.h"
#include "../MiscFunctions.h"
#include "../StringUtil.h"
#include "../Item.h"
#include "Underfoot_structs.h"
#include "../rulesys.h"

#include <iostream>
#include <sstream>

namespace Underfoot
{

static const char *name = "Underfoot";
static OpcodeManager *opcodes = nullptr;
static Strategy struct_strategy;

void Register(EQStreamIdentifier &into) {
	//create our opcode manager if we havent already
	if(opcodes == nullptr) {
		//TODO: get this file name from the config file
		std::string opfile = "patch_";
		opfile += name;
		opfile += ".conf";
		//load up the opcode manager.
		//TODO: figure out how to support shared memory with multiple patches...
		opcodes = new RegularOpcodeManager();
		if(!opcodes->LoadOpcodes(opfile.c_str())) {
			_log(NET__OPCODES, "Error loading opcodes file %s. Not registering patch %s.", opfile.c_str(), name);
			return;
		}
	}

	//ok, now we have what we need to register.

	EQStream::Signature signature;
	std::string pname;

	//register our world signature.
	pname = std::string(name) + "_world";
	signature.ignore_eq_opcode = 0;
	signature.first_length = sizeof(structs::LoginInfo_Struct);
	signature.first_eq_opcode = opcodes->EmuToEQ(OP_SendLoginInfo);
	into.RegisterPatch(signature, pname.c_str(), &opcodes, &struct_strategy);

	//register our zone signature.
	pname = std::string(name) + "_zone";
	signature.ignore_eq_opcode = opcodes->EmuToEQ(OP_AckPacket);
	signature.first_length = sizeof(structs::ClientZoneEntry_Struct);
	signature.first_eq_opcode = opcodes->EmuToEQ(OP_ZoneEntry);
	into.RegisterPatch(signature, pname.c_str(), &opcodes, &struct_strategy);



	_log(NET__IDENTIFY, "Registered patch %s", name);
}

Strategy::Strategy()
: StructStrategy()
{
	//all opcodes default to passthrough.
	#include "SSRegister.h"
	#include "Underfoot_ops.h"
}

std::string Strategy::Describe() const {
       std::string r;
	r += "Patch ";
	r += name;
	return(r);
}

const EQClientVersion Strategy::ClientVersion() const
{
	return EQClientUnderfoot;
}

#include "SSDefine.h"


// Converts Titanium Slot IDs to Underfoot Slot IDs for use in Encodes
static inline uint32 TitaniumToUnderfootSlot(uint32 TitaniumSlot) {
	uint32 UnderfootSlot = 0;

	if(TitaniumSlot >= 21 && TitaniumSlot <= 53)	// Cursor/Ammo/Power Source and Normal Inventory Slots
	{
		UnderfootSlot = TitaniumSlot + 1;
	}
	else if(TitaniumSlot >= 251 && TitaniumSlot <= 340)		// Bag Slots for Normal Inventory and Cursor
	{
		UnderfootSlot = TitaniumSlot + 11;
	}
	else if(TitaniumSlot >= 2031 && TitaniumSlot <= 2270)	// Bank Bag Slots
	{
		UnderfootSlot = TitaniumSlot + 1;
	}
	else if(TitaniumSlot >= 2531 && TitaniumSlot <= 2550)	// Shared Bank Bag Slots
	{
		UnderfootSlot = TitaniumSlot + 1;
	}
	else if(TitaniumSlot == 9999)	//Unused slot ID to give a place to save Power Slot
	{
		UnderfootSlot = 21;
	}
	else
	{
		UnderfootSlot = TitaniumSlot;
	}

	return UnderfootSlot;
}

// Converts Underfoot Slot IDs to Titanium Slot IDs for use in Decodes
static inline uint32 UnderfootToTitaniumSlot(uint32 UnderfootSlot) {
	uint32 TitaniumSlot = 0;

	if(UnderfootSlot >= 22 && UnderfootSlot <= 54)	// Cursor/Ammo/Power Source and Normal Inventory Slots
	{
		TitaniumSlot = UnderfootSlot - 1;
	}
	else if(UnderfootSlot >= 262 && UnderfootSlot <= 351)	// Bag Slots for Normal Inventory and Cursor
	{
		TitaniumSlot = UnderfootSlot - 11;
	}
	else if(UnderfootSlot >= 2032 && UnderfootSlot <= 2271)	// Bank Bag Slots
	{
		TitaniumSlot = UnderfootSlot - 1;
	}
	else if(UnderfootSlot >= 2532 && UnderfootSlot <= 2551)	// Shared Bank Bag Slots
	{
		TitaniumSlot = UnderfootSlot - 1;
	}
	else if(UnderfootSlot == 21)
	{
		TitaniumSlot = 9999;	//Unused slot ID to give a place to save Power Slot
	}
	else
	{
		TitaniumSlot = UnderfootSlot;
	}

	return TitaniumSlot;
}


ENCODE(OP_OpenNewTasksWindow) {

	AvailableTaskHeader_Struct*	__emu_AvailableTaskHeader;
	AvailableTaskData1_Struct* 	__emu_AvailableTaskData1;
	AvailableTaskData2_Struct* 	__emu_AvailableTaskData2;
	AvailableTaskTrailer_Struct* 	__emu_AvailableTaskTrailer;

	structs::AvailableTaskHeader_Struct*	__eq_AvailableTaskHeader;
	structs::AvailableTaskData1_Struct* 	__eq_AvailableTaskData1;
	structs::AvailableTaskData2_Struct* 	__eq_AvailableTaskData2;
	structs::AvailableTaskTrailer_Struct* 	__eq_AvailableTaskTrailer;

	EQApplicationPacket *in = *p;
	*p = nullptr;

	unsigned char *__emu_buffer = in->pBuffer;

	__emu_AvailableTaskHeader = (AvailableTaskHeader_Struct*)__emu_buffer;

	// For each task, SoF has an extra uint32 and what appears to be space for a null terminated string.
	//
	in->size = in->size + (__emu_AvailableTaskHeader->TaskCount * 5);

	in->pBuffer = new unsigned char[in->size];

	unsigned char *__eq_buffer = in->pBuffer;

	__eq_AvailableTaskHeader = (structs::AvailableTaskHeader_Struct*)__eq_buffer;

	char *__eq_ptr, *__emu_Ptr;

	// Copy Header
	//
	//

	__eq_AvailableTaskHeader->TaskCount = __emu_AvailableTaskHeader->TaskCount;
	__eq_AvailableTaskHeader->unknown1 = __emu_AvailableTaskHeader->unknown1;
	__eq_AvailableTaskHeader->TaskGiver = __emu_AvailableTaskHeader->TaskGiver;

	__emu_Ptr = (char *) __emu_AvailableTaskHeader + sizeof(AvailableTaskHeader_Struct);
	__eq_ptr = (char *) __eq_AvailableTaskHeader + sizeof(structs::AvailableTaskHeader_Struct);

	for(uint32 i=0; i<__emu_AvailableTaskHeader->TaskCount; i++) {

		__emu_AvailableTaskData1 = (AvailableTaskData1_Struct*)__emu_Ptr;
		__eq_AvailableTaskData1 = (structs::AvailableTaskData1_Struct*)__eq_ptr;

		__eq_AvailableTaskData1->TaskID = __emu_AvailableTaskData1->TaskID;
		// This next unknown seems to affect the colour of the task title. 0x3f80000 is what I have seen
		// in Underfoot packets. Changing it to 0x3f000000 makes the title red.
		__eq_AvailableTaskData1->unknown1 = 0x3f800000;
		__eq_AvailableTaskData1->TimeLimit = __emu_AvailableTaskData1->TimeLimit;
		__eq_AvailableTaskData1->unknown2 = __emu_AvailableTaskData1->unknown2;

		__emu_Ptr += sizeof(AvailableTaskData1_Struct);
		__eq_ptr += sizeof(structs::AvailableTaskData1_Struct);

		strcpy(__eq_ptr, __emu_Ptr); // Title

		__emu_Ptr += strlen(__emu_Ptr) + 1;
		__eq_ptr += strlen(__eq_ptr) + 1;

		strcpy(__eq_ptr, __emu_Ptr); // Description

		__emu_Ptr += strlen(__emu_Ptr) + 1;
		__eq_ptr += strlen(__eq_ptr) + 1;

		__eq_ptr[0] = 0;
		__eq_ptr += strlen(__eq_ptr) + 1;

		__emu_AvailableTaskData2 = (AvailableTaskData2_Struct*)__emu_Ptr;
		__eq_AvailableTaskData2 = (structs::AvailableTaskData2_Struct*)__eq_ptr;

		__eq_AvailableTaskData2->unknown1 = __emu_AvailableTaskData2->unknown1;
		__eq_AvailableTaskData2->unknown2 = __emu_AvailableTaskData2->unknown2;
		__eq_AvailableTaskData2->unknown3 = __emu_AvailableTaskData2->unknown3;
		__eq_AvailableTaskData2->unknown4 = __emu_AvailableTaskData2->unknown4;

		__emu_Ptr += sizeof(AvailableTaskData2_Struct);
		__eq_ptr += sizeof(structs::AvailableTaskData2_Struct);

		strcpy(__eq_ptr, __emu_Ptr); // Unknown string

		__emu_Ptr += strlen(__emu_Ptr) + 1;
		__eq_ptr += strlen(__eq_ptr) + 1;

		strcpy(__eq_ptr, __emu_Ptr); // Unknown string

		__emu_Ptr += strlen(__emu_Ptr) + 1;
		__eq_ptr += strlen(__eq_ptr) + 1;

		__emu_AvailableTaskTrailer = (AvailableTaskTrailer_Struct*)__emu_Ptr;
		__eq_AvailableTaskTrailer = (structs::AvailableTaskTrailer_Struct*)__eq_ptr;

		__eq_AvailableTaskTrailer->ItemCount = __emu_AvailableTaskTrailer->ItemCount;
		__eq_AvailableTaskTrailer->unknown1 = __emu_AvailableTaskTrailer->unknown1;
		__eq_AvailableTaskTrailer->unknown2 = __emu_AvailableTaskTrailer->unknown2;
		__eq_AvailableTaskTrailer->StartZone = __emu_AvailableTaskTrailer->StartZone;

		__emu_Ptr += sizeof(AvailableTaskTrailer_Struct);
		__eq_ptr += sizeof(structs::AvailableTaskTrailer_Struct);

		strcpy(__eq_ptr, __emu_Ptr); // Unknown string

		__emu_Ptr += strlen(__emu_Ptr) + 1;
		__eq_ptr += strlen(__eq_ptr) + 1;
	}

	delete[] __emu_buffer;

	dest->FastQueuePacket(&in, ack_req);
}


//ENCODE(OP_SendCharInfo) {
//	ENCODE_LENGTH_EXACT(CharacterSelect_Struct);
//	SETUP_VAR_ENCODE(CharacterSelect_Struct);
//
//
//	//EQApplicationPacket *packet = *p;
//	//const CharacterSelect_Struct *emu = (CharacterSelect_Struct *) packet->pBuffer;
//
//	int char_count;
//	int namelen = 0;
//	for(char_count = 0; char_count < 18; char_count++) {
//		if(emu->name[char_count][0] == '\0')
//			break;
//		if(strcmp(emu->name[char_count], "<none>") == 0)
//			break;
//		namelen += strlen(emu->name[char_count]);
//    }
//
//	int total_length = sizeof(structs::CharacterSelect_Struct)
//		+ char_count * sizeof(structs::CharacterSelectEntry_Struct)
//		+ namelen;
//
//	ALLOC_VAR_ENCODE(structs::CharacterSelect_Struct, total_length);
//
//	//unsigned char *eq_buffer = new unsigned char[total_length];
//	//structs::CharacterSelect_Struct *eq_head = (structs::CharacterSelect_Struct *) eq_buffer;
//
//	eq->char_count = char_count;
//	eq->total_chars = 18;
//
//	unsigned char *bufptr = (unsigned char *) eq->entries;
//	int r;
//	for(r = 0; r < char_count; r++) {
//		{	//pre-name section...
//			structs::CharacterSelectEntry_Struct *eq2 = (structs::CharacterSelectEntry_Struct *) bufptr;
//			eq2->level = emu->level[r];
//			eq2->hairstyle = emu->hairstyle[r];
//			eq2->gender = emu->gender[r];
//			memcpy(eq2->name, emu->name[r], strlen(emu->name[r])+1);
//		}
//		//adjust for name.
//		bufptr += strlen(emu->name[r]);
//		{	//post-name section...
//			structs::CharacterSelectEntry_Struct *eq2 = (structs::CharacterSelectEntry_Struct *) bufptr;
//			eq2->beard = emu->beard[r];
//			eq2->haircolor = emu->haircolor[r];
//			eq2->face = emu->face[r];
//			int k;
//			for(k = 0; k < _MaterialCount; k++) {
//				eq2->equip[k].equip0 = emu->equip[r][k];
//				eq2->equip[k].equip1 = 0;
//				eq2->equip[k].itemid = 0;
//				eq2->equip[k].color.color = emu->cs_colors[r][k].color;
//			}
//			eq2->primary = emu->primary[r];
//			eq2->secondary = emu->secondary[r];
//			eq2->tutorial = emu->tutorial[r]; // was u15
//			eq2->u15 = 0xff;
//			eq2->deity = emu->deity[r];
//			eq2->zone = emu->zone[r];
//			eq2->u19 = 0xFF;
//			eq2->race = emu->race[r];
//			eq2->gohome = emu->gohome[r];
//			eq2->class_ = emu->class_[r];
//			eq2->eyecolor1 = emu->eyecolor1[r];
//			eq2->beardcolor = emu->beardcolor[r];
//			eq2->eyecolor2 = emu->eyecolor2[r];
//			eq2->drakkin_heritage = emu->drakkin_heritage[r];
//			eq2->drakkin_tattoo = emu->drakkin_tattoo[r];
//			eq2->drakkin_details = emu->drakkin_details[r];
//		}
//		bufptr += sizeof(structs::CharacterSelectEntry_Struct);
//	}
//
//	FINISH_ENCODE();
//
//}

//ENCODE(OP_ZoneServerInfo) {
//	SETUP_DIRECT_ENCODE(ZoneServerInfo_Struct, ZoneServerInfo_Struct);
//	OUT_str(ip);
//	OUT(port);
//	FINISH_ENCODE();
//}

//ENCODE(OP_SendZonepoints) {
//	SETUP_VAR_ENCODE(ZonePoints);
//	ALLOC_VAR_ENCODE(structs::ZonePoints, sizeof(structs::ZonePoints) + sizeof(structs::ZonePoint_Entry) * (emu->count + 1));
//
//	eq->count = emu->count;
//	for(uint32 i = 0; i < emu->count; ++i)
//	{
//		eq->zpe[i].iterator = emu->zpe[i].iterator;
//		eq->zpe[i].x = emu->zpe[i].x;
//		eq->zpe[i].y = emu->zpe[i].y;
//		eq->zpe[i].z = emu->zpe[i].z;
//		eq->zpe[i].heading = emu->zpe[i].heading;
//		eq->zpe[i].zoneid = emu->zpe[i].zoneid;
//		eq->zpe[i].zoneinstance = emu->zpe[i].zoneinstance;
//	}
//
//	FINISH_ENCODE();
//}

ENCODE(OP_SendAATable) {
	ENCODE_LENGTH_ATLEAST(SendAA_Struct);

	SETUP_VAR_ENCODE(SendAA_Struct);
	ALLOC_VAR_ENCODE(structs::SendAA_Struct, sizeof(structs::SendAA_Struct) + emu->total_abilities*sizeof(structs::AA_Ability));

	// Check clientver field to verify this AA should be sent for SoF
	// clientver 1 is for all clients and 6 is for Underfoot
	if (emu->clientver <= 6 )
	{
		OUT(id);
		eq->unknown004 = 1;
		//eq->hotkey_sid = (emu->hotkey_sid==4294967295UL)?0:(emu->id - emu->current_level + 1);
		//eq->hotkey_sid2 = (emu->hotkey_sid2==4294967295UL)?0:(emu->id - emu->current_level + 1);
		//eq->title_sid = emu->id - emu->current_level + 1;
		//eq->desc_sid = emu->id - emu->current_level + 1;
		eq->hotkey_sid = (emu->hotkey_sid==4294967295UL)?0:(emu->sof_next_skill);
		eq->hotkey_sid2 = (emu->hotkey_sid2==4294967295UL)?0:(emu->sof_next_skill);
		eq->title_sid = emu->sof_next_skill;
		eq->desc_sid = emu->sof_next_skill;
		OUT(class_type);
		OUT(cost);
		OUT(seq);
		OUT(current_level);
		OUT(prereq_skill);
		OUT(prereq_minpoints);
		eq->type = emu->sof_type;
		OUT(spellid);
		OUT(spell_type);
		OUT(spell_refresh);
		OUT(classes);
		OUT(berserker);
		//eq->max_level = emu->sof_max_level;
		OUT(max_level);
		OUT(last_id);
		OUT(next_id);
		OUT(cost2);
		eq->aa_expansion = emu->aa_expansion;
		eq->special_category = emu->special_category;
		OUT(total_abilities);
		unsigned int r;
		for(r = 0; r < emu->total_abilities; r++) {
			OUT(abilities[r].skill_id);
			OUT(abilities[r].base1);
			OUT(abilities[r].base2);
			OUT(abilities[r].slot);
		}
	}
	FINISH_ENCODE();
}

//ENCODE(OP_LeadershipExpUpdate) {
//	SETUP_DIRECT_ENCODE(LeadershipExpUpdate_Struct, structs::LeadershipExpUpdate_Struct);
//	OUT(group_leadership_exp);
//	OUT(group_leadership_points);
//	OUT(raid_leadership_exp);
//	OUT(raid_leadership_points);
//	FINISH_ENCODE();
//}

//ENCODE(OP_GuildMemberLevelUpdate) {
//	SETUP_DIRECT_ENCODE(GuildMemberLevelUpdate_Struct, structs::GuildMemberLevelUpdate_Struct);
//	OUT(guild_id);
//	OUT_str(member_name);
//	OUT(level);
//	FINISH_ENCODE();
//}

ENCODE(OP_RespondAA) {
	SETUP_DIRECT_ENCODE(AATable_Struct, structs::AATable_Struct);

	eq->aa_spent = emu->aa_spent;
	eq->aa_assigned = emu->aa_spent;
	eq->aa_spent3 = emu->aa_spent;
	eq->unknown012 = 0;
	eq->unknown016 = 0;
	eq->unknown020 = 0;

	for(uint32 i = 0; i < MAX_PP_AA_ARRAY; ++i)
	{
		eq->aa_list[i].aa_skill = emu->aa_list[i].aa_skill;
		eq->aa_list[i].aa_value = emu->aa_list[i].aa_value;
		eq->aa_list[i].unknown08 = emu->aa_list[i].unknown08;
	}

	FINISH_ENCODE();
}

ENCODE(OP_PlayerProfile) {
	SETUP_DIRECT_ENCODE(PlayerProfile_Struct, structs::PlayerProfile_Struct);

	uint32 r;

	eq->available_slots=0xffffffff;
	memset(eq->unknown06284, 0xff, sizeof(eq->unknown06284));
	memset(eq->unknown07284, 0xff, sizeof(eq->unknown07284));

//	OUT(checksum);
	OUT(gender);
	OUT(race);
	OUT(class_);
//	OUT(unknown00016);
	OUT(level);
	eq->level1 = emu->level;
//	OUT(unknown00022[2]);
	for(r = 0; r < 5; r++) {
		OUT(binds[r].zoneId);
		OUT(binds[r].x);
		OUT(binds[r].y);
		OUT(binds[r].z);
		OUT(binds[r].heading);
	}
	OUT(deity);
	OUT(intoxication);
	OUT_array(spellSlotRefresh, structs::MAX_PP_MEMSPELL);
	OUT(abilitySlotRefresh);
	OUT(points); // Relocation Test
//	OUT(unknown0166[4]);
	OUT(haircolor);
	OUT(beardcolor);
	OUT(eyecolor1);
	OUT(eyecolor2);
	OUT(hairstyle);
	OUT(beard);
//	OUT(unknown00178[10]);
	for(r = 0; r < 9; r++) {
		eq->equipment[r].equip0 = emu->item_material[r];
		eq->equipment[r].equip1 = 0;
		eq->equipment[r].itemId = 0;
		//eq->colors[r].color = emu->colors[r].color;
	}
	for(r = 0; r < 7; r++) {
		OUT(item_tint[r].color);
	}
//	OUT(unknown00224[48]);
	//NOTE: new client supports 300 AAs, our internal rep/PP
	//only supports 240..
	for(r = 0; r < MAX_PP_AA_ARRAY; r++) {
		OUT(aa_array[r].AA);
		OUT(aa_array[r].value);
	}
//	OUT(unknown02220[4]);
	OUT(mana);
	OUT(cur_hp);
	OUT(STR);
	OUT(STA);
	OUT(CHA);
	OUT(AGI);
	OUT(INT);
	OUT(DEX);
	OUT(WIS);
	OUT(face);
//	OUT(unknown02264[47]);
	OUT_array(spell_book, structs::MAX_PP_SPELLBOOK);
//	OUT(unknown4184[128]);
	OUT_array(mem_spells, structs::MAX_PP_MEMSPELL);
//	OUT(unknown04396[32]);
	OUT(platinum);
	OUT(gold);
	OUT(silver);
	OUT(copper);
	OUT(platinum_cursor);
	OUT(gold_cursor);
	OUT(silver_cursor);
	OUT(copper_cursor);

	OUT_array(skills, structs::MAX_PP_SKILL);	// 1:1 direct copy (100 dword)

//	OUT(unknown04760[236]);
	OUT(toxicity);
	OUT(thirst_level);
	OUT(hunger_level);
	//PS this needs to be figured out more; but it was 'good enough'
	for(r = 0; r < structs::BUFF_COUNT; r++)
	{
		if(emu->buffs[r].spellid != 0xFFFF && emu->buffs[r].spellid != 0)
		{
			eq->buffs[r].unknown004 = 0x3f800000;
			eq->buffs[r].slotid = 2;
			eq->buffs[r].player_id = 0x000717fd;
		}
		else
		{
			eq->buffs[r].slotid = 0;
		}
		//OUT(buffs[r].slotid);
		OUT(buffs[r].level);
		//OUT(buffs[r].bard_modifier);
		//OUT(buffs[r].effect);
		OUT(buffs[r].spellid);
		OUT(buffs[r].duration);
		OUT(buffs[r].counters);
		//OUT(buffs[r].player_id);
	}
	for(r = 0; r < MAX_PP_DISCIPLINES; r++) {
		OUT(disciplines.values[r]);
	}
	OUT_array(recastTimers, structs::MAX_RECAST_TYPES);
//	OUT(unknown08124[360]);
	OUT(endurance);
	OUT(aapoints_spent);
	OUT(aapoints);
//	OUT(unknown06160[4]);
	//NOTE: new client supports 20 bandoliers, our internal rep
	//only supports 4..
	for(r = 0; r < 4; r++) {
		OUT_str(bandoliers[r].name);
		uint32 k;
		for(k = 0; k < structs::MAX_PLAYER_BANDOLIER_ITEMS; k++) {
			OUT(bandoliers[r].items[k].item_id);
			OUT(bandoliers[r].items[k].icon);
			OUT_str(bandoliers[r].items[k].item_name);
		}
	}
//	OUT(unknown07444[5120]);
	for(r = 0; r < structs::MAX_POTIONS_IN_BELT; r++) {
		OUT(potionbelt.items[r].item_id);
		OUT(potionbelt.items[r].icon);
		OUT_str(potionbelt.items[r].item_name);
	}
//	OUT(unknown12852[8]);
//	OUT(unknown12864[76]);
	OUT_str(name);
	OUT_str(last_name);
	OUT(guild_id);
	OUT(birthday);
	OUT(lastlogin);
	OUT(timePlayedMin);
	OUT(pvp);
	OUT(mAnonymous);
	OUT(gm);
	OUT(guildrank);
	OUT(guildbanker);
//	OUT(unknown13054[12]);
	OUT(exp);
//	OUT(unknown13072[8]);
	OUT(timeentitledonaccount);
	OUT_array(languages, structs::MAX_PP_LANGUAGE);
//	OUT(unknown13109[7]);
	OUT(y); //reversed x and y
	OUT(x);
	OUT(z);
	OUT(heading);
//	OUT(unknown13132[4]);
	OUT(platinum_bank);
	OUT(gold_bank);
	OUT(silver_bank);
	OUT(copper_bank);
	OUT(platinum_shared);
//	OUT(unknown13156[84]);
	//OUT(expansions);
	eq->expansions = 0xffff;
//	OUT(unknown13244[12]);
	OUT(autosplit);
//	OUT(unknown13260[16]);
	OUT(zone_id);
	OUT(zoneInstance);
	for(r = 0; r < structs::MAX_GROUP_MEMBERS; r++) {
		OUT_str(groupMembers[r]);
	}
	strcpy(eq->groupLeader, emu->groupMembers[0]);
//	OUT_str(groupLeader);
//	OUT(unknown13728[660]);
	OUT(entityid);
	OUT(leadAAActive);
//	OUT(unknown14392[4]);
	OUT(ldon_points_guk);
	OUT(ldon_points_mir);
	OUT(ldon_points_mmc);
	OUT(ldon_points_ruj);
	OUT(ldon_points_tak);
	OUT(ldon_points_available);
//	OUT(unknown14420[132]);
	OUT(tribute_time_remaining);
	OUT(career_tribute_points);
//	OUT(unknown7208);
	OUT(tribute_points);
//	OUT(unknown7216);
	OUT(tribute_active);
	for(r = 0; r < structs::MAX_PLAYER_TRIBUTES; r++) {
		OUT(tributes[r].tribute);
		OUT(tributes[r].tier);
	}
//	OUT(unknown14616[8]);
	OUT(group_leadership_exp);
//	OUT(unknown14628);
	OUT(raid_leadership_exp);
	OUT(group_leadership_points);
	OUT(raid_leadership_points);
	OUT_array(leader_abilities.ranks, structs::MAX_LEADERSHIP_AA_ARRAY);
//	OUT(unknown14772[128]);
	OUT(air_remaining);
	OUT(PVPKills);
	OUT(PVPDeaths);
	OUT(PVPCurrentPoints);
	OUT(PVPCareerPoints);
	OUT(PVPBestKillStreak);
	OUT(PVPWorstDeathStreak);
	OUT(PVPCurrentKillStreak);
//	OUT(unknown17892[4580]);
	OUT(expAA);
//	OUT(unknown19516[40]);
	OUT(currentRadCrystals);
	OUT(careerRadCrystals);
	OUT(currentEbonCrystals);
	OUT(careerEbonCrystals);
	OUT(groupAutoconsent);
	OUT(raidAutoconsent);
	OUT(guildAutoconsent);
//	OUT(unknown19575[5]);
	eq->level3 = emu->level;
	eq->showhelm = emu->showhelm;
	OUT(RestTimer);
//	OUT(unknown19584[4]);
//	OUT(unknown19588);


const uint8 bytes[] = {
0xa3,0x02,0x00,0x00,0x95,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x19,0x00,0x00,0x00,
0x19,0x00,0x00,0x00,0x19,0x00,0x00,0x00,0x0F,0x00,0x00,0x00,0x0F,0x00,0x00,0x00,
0x0F,0x00,0x00,0x00,0x0F,0x00,0x00,0x00,0x1F,0x85,0xEB,0x3E,0x33,0x33,0x33,0x3F,
0x04,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x07,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

	memcpy(eq->unknown18020, bytes, sizeof(bytes));

	//set the checksum...
	CRC32::SetEQChecksum(__packet->pBuffer, sizeof(structs::PlayerProfile_Struct)-4);

	FINISH_ENCODE();
}

//ENCODE(OP_NewZone) {
//	SETUP_DIRECT_ENCODE(NewZone_Struct, structs::NewZone_Struct);
//	OUT_str(char_name);
//	OUT_str(zone_short_name);
//	OUT_str(zone_long_name);
//	OUT(ztype);
//	int r;
//	for(r = 0; r < 4; r++) {
//		OUT(fog_red[r]);
//		OUT(fog_green[r]);
//		OUT(fog_blue[r]);
//		OUT(fog_minclip[r]);
//		OUT(fog_maxclip[r]);
//	}
//	OUT(gravity);
//	OUT(time_type);
//	for(r = 0; r < 4; r++) {
//		OUT(rain_chance[r]);
//	}
//	for(r = 0; r < 4; r++) {
//		OUT(rain_duration[r]);
//	}
//	for(r = 0; r < 4; r++) {
//		OUT(snow_chance[r]);
//	}
//	for(r = 0; r < 4; r++) {
//		OUT(snow_duration[r]);
//	}
//	for(r = 0; r < 32; r++) {
//		eq->unknown537[r] = 0xFF;	//observed
//	}
//	OUT(sky);
//	OUT(zone_exp_multiplier);
//	OUT(safe_y);
//	OUT(safe_x);
//	OUT(safe_z);
//	OUT(max_z);
//	OUT(underworld);
//	OUT(minclip);
//	OUT(maxclip);
//	OUT_str(zone_short_name2);
//	OUT(zone_id);
//	OUT(zone_instance);
//	OUT(SuspendBuffs);
//
//	eq->FogDensity = emu->fog_density;
//
//    /*fill in some unknowns with observed values, hopefully it will help */
//	eq->unknown800 = -1;
//	eq->unknown844 = 600;
//	eq->unknown880 = 50;
//	eq->unknown884 = 10;
//	eq->unknown888 = 1;
//	eq->unknown889 = 0;
//	eq->unknown890 = 1;
//	eq->unknown891 = 0;
//	eq->unknown892 = 0;
//	eq->unknown893 = 0;
//	eq->fall_damage = 0;	// 0 = Fall Damage on, 1 = Fall Damage off
//	eq->unknown895 = 0;
//	eq->unknown896 = 180;
//	eq->unknown900 = 180;
//	eq->unknown904 = 180;
//	eq->unknown908 = 2;
//	eq->unknown912 = 2;
//
//	FINISH_ENCODE();
//}

ENCODE(OP_Track)
{
	EQApplicationPacket *in = *p;
	*p = nullptr;

	unsigned char *__emu_buffer = in->pBuffer;
	Track_Struct *emu = (Track_Struct *) __emu_buffer;

	int EntryCount = in->size / sizeof(Track_Struct);

	if(EntryCount == 0 || ((in->size % sizeof(Track_Struct))) != 0)
	{
		_log(NET__STRUCTS, "Wrong size on outbound %s: Got %d, expected multiple of %d", opcodes->EmuToName(in->GetOpcode()), in->size, sizeof(Track_Struct));
		delete in;
		return;
	}

	int PacketSize = 2;

	for(int i = 0; i < EntryCount; ++i, ++emu)
		PacketSize += (12 + strlen(emu->name));

	emu = (Track_Struct *) __emu_buffer;

	in->size = PacketSize;
	in->pBuffer = new unsigned char[in->size];

	char *Buffer = (char *)in->pBuffer;

	VARSTRUCT_ENCODE_TYPE(uint16, Buffer, EntryCount);

	for(int i = 0; i < EntryCount; ++i, ++emu)
	{
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->entityid);
		VARSTRUCT_ENCODE_TYPE(float, Buffer, emu->distance);
		VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->level);
		VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->NPC);
		VARSTRUCT_ENCODE_STRING(Buffer, emu->name);
		VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->GroupMember);
	}

	delete[] __emu_buffer;

	dest->FastQueuePacket(&in, ack_req);
}

ENCODE(OP_PetBuffWindow)
{
	EQApplicationPacket *in = *p;
	*p = nullptr;

	unsigned char *__emu_buffer = in->pBuffer;

	PetBuff_Struct *emu = (PetBuff_Struct *) __emu_buffer;

	int PacketSize = 12 + (emu->buffcount * 17);

	in->size = PacketSize;

	in->pBuffer = new unsigned char[in->size];

	char *Buffer = (char *)in->pBuffer;

	VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->petid);
	VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0);
	VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 1);
	VARSTRUCT_ENCODE_TYPE(uint16, Buffer, emu->buffcount);

	for(unsigned int i = 0; i < BUFF_COUNT; ++i)
	{
		if(emu->spellid[i])
		{
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, i);
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->spellid[i]);
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->ticsremaining[i]);
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0);
			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0);	// This is a string. Name of the caster of the buff.
		}
	}
	VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->buffcount);

	delete[] __emu_buffer;
	dest->FastQueuePacket(&in, ack_req);
}

ENCODE(OP_Barter)
{
	EQApplicationPacket *in = *p;
	*p = nullptr;

	char *Buffer = (char *)in->pBuffer;

	uint32 SubAction = VARSTRUCT_DECODE_TYPE(uint32, Buffer);

	if(SubAction != Barter_BuyerAppearance)
	{
		dest->FastQueuePacket(&in, ack_req);

		return;
	}

	unsigned char *__emu_buffer = in->pBuffer;

	in->size = 80;

	in->pBuffer = new unsigned char[in->size];

	char *OutBuffer = (char *)in->pBuffer;

	char Name[64];

	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, SubAction);
	uint32 EntityID = VARSTRUCT_DECODE_TYPE(uint32, Buffer);
	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, EntityID);
	uint8 Toggle = VARSTRUCT_DECODE_TYPE(uint8, Buffer);
	VARSTRUCT_DECODE_STRING(Name, Buffer);
	VARSTRUCT_ENCODE_STRING(OutBuffer, Name);
	OutBuffer = (char *)in->pBuffer + 72;
	VARSTRUCT_ENCODE_TYPE(uint8, OutBuffer, Toggle);

	delete[] __emu_buffer;

	dest->FastQueuePacket(&in, ack_req);

}

ENCODE(OP_BazaarSearch)
{
	EQApplicationPacket *in = *p;
	*p = nullptr;

	char *Buffer = (char *)in->pBuffer;

	uint8 SubAction = VARSTRUCT_DECODE_TYPE(uint8, Buffer);

	if(SubAction != BazaarSearchResults)
	{
		dest->FastQueuePacket(&in, ack_req);

		return;
	}

	unsigned char *__emu_buffer = in->pBuffer;

	BazaarSearchResults_Struct *emu = (BazaarSearchResults_Struct *) __emu_buffer;

	int EntryCount = in->size / sizeof(BazaarSearchResults_Struct);

	if(EntryCount == 0 || (in->size % sizeof(BazaarSearchResults_Struct)) != 0)
	{
		_log(NET__STRUCTS, "Wrong size on outbound %s: Got %d, expected multiple of %d", opcodes->EmuToName(in->GetOpcode()), in->size, sizeof(BazaarSearchResults_Struct));
		delete in;
		return;
	}
	in->size = EntryCount * sizeof(structs::BazaarSearchResults_Struct);

	in->pBuffer = new unsigned char[in->size];

	memset(in->pBuffer, 0, in->size);

	structs::BazaarSearchResults_Struct *eq = (structs::BazaarSearchResults_Struct *)in->pBuffer;

	for(int i = 0; i < EntryCount; ++i, ++emu, ++eq)
	{
		OUT(Beginning.Action);
		OUT(SellerID);
		memcpy(eq->SellerName, emu->SellerName, sizeof(eq->SellerName));
		OUT(NumItems);
		OUT(ItemID);
		OUT(SerialNumber);
		memcpy(eq->ItemName, emu->ItemName, sizeof(eq->ItemName));
		OUT(Cost);
		OUT(ItemStat);
	}

	delete[] __emu_buffer;

	dest->FastQueuePacket(&in, ack_req);
}

//ENCODE(OP_NewSpawn) {  ENCODE_FORWARD(OP_ZoneSpawns); }
//ENCODE(OP_ZoneEntry){  ENCODE_FORWARD(OP_ZoneSpawns); }
//ENCODE(OP_ZoneSpawns) {
//		//consume the packet
//		EQApplicationPacket *in = *p;
//		*p = nullptr;
//
//		//store away the emu struct
//		unsigned char *__emu_buffer = in->pBuffer;
//		Spawn_Struct *emu = (Spawn_Struct *) __emu_buffer;
//
//		//determine and verify length
//		int entrycount = in->size / sizeof(Spawn_Struct);
//		if(entrycount == 0 || (in->size % sizeof(Spawn_Struct)) != 0) {
//			_log(NET__STRUCTS, "Wrong size on outbound %s: Got %d, expected multiple of %d", opcodes->EmuToName(in->GetOpcode()), in->size, sizeof(Spawn_Struct));
//			delete in;
//			return;
//		}
//
//
//		//_log(NET__STRUCTS, "Spawn name is [%s]", emu->name);
//
//		emu = (Spawn_Struct *) __emu_buffer;
//
//		//_log(NET__STRUCTS, "Spawn packet size is %i, entries = %i", in->size, entrycount);
//
//		char *Buffer = (char *) in->pBuffer;
//
//
//		int r;
//		int k;
//		for(r = 0; r < entrycount; r++, emu++) {
//
//			int PacketSize = sizeof(structs::Spawn_Struct);
//
//			PacketSize += strlen(emu->name);
//			PacketSize += strlen(emu->lastName);
//
//			if(strlen(emu->title))
//				PacketSize += strlen(emu->title) + 1;
//
//			if(strlen(emu->suffix))
//				PacketSize += strlen(emu->suffix) + 1;
//
//			if(emu->DestructibleObject)
//			{
//				PacketSize = PacketSize - 4;	// No bodytype
//				PacketSize += 53;	// Fixed portion
//				PacketSize += strlen(emu->DestructibleModel) + 1;
//				PacketSize += strlen(emu->DestructibleName2) + 1;
//				PacketSize += strlen(emu->DestructibleString) + 1;
//			}
//
//			bool ShowName = 1;
//			if(emu->bodytype >= 66)
//			{
//				emu->race = 127;
//				emu->bodytype = 11;
//				emu->gender = 0;
//				ShowName = 0;
//			}
//
//			float SpawnSize = emu->size;
//			if(!((emu->NPC == 0) || (emu->race <=12) || (emu->race == 128) || (emu->race == 130) || (emu->race == 330) || (emu->race == 522)))
//			{
//				PacketSize -= (sizeof(structs::EquipStruct) * 9);
//
//				if(emu->size == 0)
//				{
//					emu->size = 6;
//					SpawnSize = 6;
//				}
//			}
//
//			if(SpawnSize == 0)
//			{
//				SpawnSize = 3;
//			}
//
//			EQApplicationPacket *outapp = new EQApplicationPacket(OP_ZoneEntry, PacketSize);
//			Buffer = (char *) outapp->pBuffer;
//
//			VARSTRUCT_ENCODE_STRING(Buffer, emu->name);
//			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->spawnId);
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->level);
//
//			if(emu->DestructibleObject)
//			{
//				VARSTRUCT_ENCODE_TYPE(float, Buffer, 10);	// was int and 0x41200000
//			}
//			else
//			{
//				VARSTRUCT_ENCODE_TYPE(float, Buffer, SpawnSize - 0.7);	// Eye Height?
//			}
//
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->NPC);
//
//			structs::Spawn_Struct_Bitfields *Bitfields = (structs::Spawn_Struct_Bitfields*)Buffer;
//
//			Bitfields->afk = 0;
//			Bitfields->linkdead = 0;
//			Bitfields->gender = emu->gender;
//
//			Bitfields->invis = emu->invis;
//			Bitfields->sneak = 0;
//			Bitfields->lfg = emu->lfg;
//			Bitfields->gm = emu->gm;
//			Bitfields->anon = emu->anon;
//			Bitfields->showhelm = emu->showhelm;
//			Bitfields->targetable = 1;
//			Bitfields->targetable_with_hotkey = (emu->IsMercenary ? 0 : 1);
//			Bitfields->statue = 0;
//			Bitfields->trader = 0;
//			Bitfields->buyer = 0;
//
//			Bitfields->showname = ShowName;
//
//			if(emu->DestructibleObject)
//			{
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0x1d600000);
//				Buffer = Buffer -4;
//			}
//
//			Bitfields->ispet = emu->is_pet;
//
//			Buffer += sizeof(structs::Spawn_Struct_Bitfields);
//			std::cout << "sizeof Bitfields= " << sizeof(structs::Spawn_Struct_Bitfields) << std::endl;
//
//			uint8 OtherData = 0;
//
//			if(strlen(emu->title))
//				OtherData = OtherData | 0x04;
//
//			if(strlen(emu->suffix))
//				OtherData = OtherData | 0x08;
//
//			if(emu->DestructibleObject)
//				OtherData = OtherData | 0xd1;	// Live has 0xe1 for OtherData
//
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, OtherData);
//
//			if(emu->DestructibleObject)
//			{
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0x00000000);
//			}
//			else
//			{
//				VARSTRUCT_ENCODE_TYPE(float, Buffer, -1);	// unknown3
//			}
//			VARSTRUCT_ENCODE_TYPE(float, Buffer, 0);	// unknown4
//
//			if(emu->DestructibleObject)
//			{
//				VARSTRUCT_ENCODE_STRING(Buffer, emu->DestructibleModel);
//				VARSTRUCT_ENCODE_STRING(Buffer, emu->DestructibleName2);
//				VARSTRUCT_ENCODE_STRING(Buffer, emu->DestructibleString);
//
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->DestructibleAppearance);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->DestructibleUnk1);
//
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->DestructibleID1);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->DestructibleID2);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->DestructibleID3);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->DestructibleID4);
//
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->DestructibleUnk2);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->DestructibleUnk3);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->DestructibleUnk4);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->DestructibleUnk5);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->DestructibleUnk6);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->DestructibleUnk7);
//				VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->DestructibleUnk8);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->DestructibleUnk9);
//			}
//
//			VARSTRUCT_ENCODE_TYPE(float, Buffer, emu->size);
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->face);
//			VARSTRUCT_ENCODE_TYPE(float, Buffer, emu->walkspeed);
//			VARSTRUCT_ENCODE_TYPE(float, Buffer, emu->runspeed);
//			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->race);
//			/*
//			if(emu->bodytype >=66)
//			{
//				VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0);	// showname
//			}
//			else
//			{
//				VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 1);	// showname
//			}*/
//
//
//			if(!emu->DestructibleObject)
//			{
//				// Setting this next field to zero will cause a crash. Looking at ShowEQ, if it is zero, the bodytype field is not
//				// present. Will sort that out later.
//				VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 1);	// This is a properties count field
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->bodytype);
//			}
//			else
//			{
//				VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0);
//			}
//
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->curHp);
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->haircolor);
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->beardcolor);
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->eyecolor1);
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->eyecolor2);
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->hairstyle);
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->beard);
//			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->drakkin_heritage);
//			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->drakkin_tattoo);
//			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->drakkin_details);
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0);	// ShowEQ calls this 'Holding'
//			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->deity);
//			if(emu->NPC)
//			{
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0xFFFFFFFF);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0x00000000);
//			}
//			else
//			{
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->guildID);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->guildrank);
//			}
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->class_);
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0);	// pvp
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->StandState);	// standstate
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->light);
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->flymode);
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->equip_chest2); // unknown8
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0); // unknown9
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0); // unknown10
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->helm); // unknown11	
//			VARSTRUCT_ENCODE_STRING(Buffer, emu->lastName);
//			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0);	// aatitle
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0); // unknown12
//			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->petOwnerId);
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0); // unknown13
//			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0); // unknown14 - Stance 64 = normal 4 = aggressive 40 = stun/mezzed
//			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0); // unknown15
//			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0); // unknown16
//			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0); // unknown17
//			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0xffffffff); // unknown18
//			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0xffffffff); // unknown19
//
//
//			structs::Spawn_Struct_Position *Position = (structs::Spawn_Struct_Position*)Buffer;
//
//			Position->deltaX = emu->deltaX;
//			Position->deltaHeading = emu->deltaHeading;
//			Position->deltaY = emu->deltaY;
//			Position->y = emu->y;
//			Position->animation = emu->animation;
//			Position->heading = emu->heading;
//			Position->x = emu->x;
//			Position->z = emu->z;
//			Position->deltaZ = emu->deltaZ;
//
//			Buffer += sizeof(structs::Spawn_Struct_Position);
//
//			if((emu->NPC == 0) || (emu->race <=12) || (emu->race == 128) || (emu ->race == 130) || (emu->race == 330) || (emu->race == 522))
//			{
//				for(k = 0; k < 9; ++k)
//				{
//					{
//						VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->colors[k].color);
//					}
//				}
//			}
//			else
//			{
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0);
//
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->equipment[MaterialPrimary]);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0);
//
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->equipment[MaterialSecondary]);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0);
//				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0);
//			}
//
//
//			if((emu->NPC == 0) || (emu->race <=12) || (emu->race == 128) || (emu->race == 130) || (emu->race == 330) || (emu->race == 522))
//			{
//				structs::EquipStruct *Equipment = (structs::EquipStruct *)Buffer;
//
//				for(k = 0; k < 9; k++) {
//					Equipment[k].equip0 = emu->equipment[k];
//					Equipment[k].equip1 = 0;
//					Equipment[k].itemId = 0;
//				}
//
//				Buffer += (sizeof(structs::EquipStruct) * 9);
//			}
//			if(strlen(emu->title))
//			{
//				VARSTRUCT_ENCODE_STRING(Buffer, emu->title);
//			}
//
//			if(strlen(emu->suffix))
//			{
//				VARSTRUCT_ENCODE_STRING(Buffer, emu->suffix);
//			}
//			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0); // Unknown;
//			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0); // Unknown;
//			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->IsMercenary); //IsMercenary
//			Buffer += 28; // Unknown;
//
//			std::cout << "sizeof  outapp->size = " << outapp->size << std::endl;
//			dest->FastQueuePacket(&outapp, ack_req);
//	}
//
//	delete in;
//}

ENCODE(OP_MercenaryDataResponse) {
	//consume the packet
	EQApplicationPacket *in = *p;
	*p = nullptr;

	//store away the emu struct
	unsigned char *__emu_buffer = in->pBuffer;
	MercenaryMerchantList_Struct *emu = (MercenaryMerchantList_Struct *) __emu_buffer;

	char *Buffer = (char *) in->pBuffer;

	int PacketSize = sizeof(structs::MercenaryMerchantList_Struct) - 4 + emu->MercTypeCount * 4;
	PacketSize += (sizeof(structs::MercenaryListEntry_Struct) - sizeof(structs::MercenaryStance_Struct)) * emu->MercCount;

	uint32 r;
	uint32 k;
	for(r = 0; r < emu->MercCount; r++)
	{
		PacketSize += sizeof(structs::MercenaryStance_Struct) * emu->Mercs[r].StanceCount;
	}

	EQApplicationPacket *outapp = new EQApplicationPacket(OP_MercenaryDataResponse, PacketSize);
	Buffer = (char *) outapp->pBuffer;

	VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercTypeCount);

	for(r = 0; r < emu->MercTypeCount; r++)
	{
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercGrades[r]);
	}

	VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercCount);

	for(r = 0; r < emu->MercCount; r++)
	{
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->Mercs[r].MercID);
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->Mercs[r].MercType);
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->Mercs[r].MercSubType);
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->Mercs[r].PurchaseCost);
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->Mercs[r].UpkeepCost);
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->Mercs[r].AltCurrencyCost);
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->Mercs[r].AltCurrencyUpkeep);
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->Mercs[r].AltCurrencyType);
		VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->Mercs[r].MercUnk01);
		VARSTRUCT_ENCODE_TYPE(int32, Buffer, emu->Mercs[r].TimeLeft);
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->Mercs[r].MerchantSlot);
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->Mercs[r].MercUnk02);
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->Mercs[r].StanceCount);
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->Mercs[r].MercUnk03);
		VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->Mercs[r].MercUnk04);
		for(k = 0; k < emu->Mercs[r].StanceCount; k++)
		{
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->Mercs[r].Stances[k].StanceIndex);
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->Mercs[r].Stances[k].Stance);
		}
	}

	dest->FastQueuePacket(&outapp, ack_req);

	delete in;
}

// This packet does not appear to exist in UF, but leaving it here just in case
ENCODE(OP_MercenaryDataUpdate) {
	//consume the packet
	EQApplicationPacket *in = *p;
	*p = nullptr;

	//store away the emu struct
	unsigned char *__emu_buffer = in->pBuffer;
	MercenaryDataUpdate_Struct *emu = (MercenaryDataUpdate_Struct *) __emu_buffer;

	char *Buffer = (char *) in->pBuffer;

	EQApplicationPacket *outapp;

	uint32 PacketSize = 0;

	// There are 2 different sized versions of this packet depending if a merc is hired or not
	if (emu->MercStatus >= 0)
	{
		PacketSize += sizeof(structs::MercenaryDataUpdate_Struct) + (sizeof(structs::MercenaryData_Struct) - sizeof(structs::MercenaryStance_Struct)) * emu->MercCount;

		uint32 r;
		uint32 k;
		for(r = 0; r < emu->MercCount; r++)
		{
			PacketSize += sizeof(structs::MercenaryStance_Struct) * emu->MercData[r].StanceCount;
		}

		outapp = new EQApplicationPacket(OP_MercenaryDataUpdate, PacketSize);
		Buffer = (char *) outapp->pBuffer;

		VARSTRUCT_ENCODE_TYPE(int32, Buffer, emu->MercStatus);
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercCount);

		for(r = 0; r < emu->MercCount; r++)
		{
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercData[r].MercID);
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercData[r].MercType);
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercData[r].MercSubType);
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercData[r].PurchaseCost);
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercData[r].UpkeepCost);
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercData[r].AltCurrencyCost);
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercData[r].AltCurrencyUpkeep);
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercData[r].AltCurrencyType);
			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->MercData[r].MercUnk01);
			VARSTRUCT_ENCODE_TYPE(int32, Buffer, emu->MercData[r].TimeLeft);
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercData[r].MerchantSlot);
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercData[r].MercUnk02);
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercData[r].StanceCount);
			VARSTRUCT_ENCODE_TYPE(int32, Buffer, emu->MercData[r].MercUnk03);
			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, emu->MercData[r].MercUnk04);
			for(k = 0; k < emu->MercData[r].StanceCount; k++)
			{
				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercData[r].Stances[k].StanceIndex);
				VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercData[r].Stances[k].Stance);
			}
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercData[r].MercUnk05);
		}
	}
	else
	{
		PacketSize += sizeof(structs::NoMercenaryHired_Struct);

		outapp = new EQApplicationPacket(OP_MercenaryDataUpdate, PacketSize);
		Buffer = (char *) outapp->pBuffer;

		VARSTRUCT_ENCODE_TYPE(int32, Buffer, emu->MercStatus);
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, emu->MercCount);
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 1);
	}

	dest->FastQueuePacket(&outapp, ack_req);

	delete in;
}

//ENCODE(OP_ItemLinkResponse) {  ENCODE_FORWARD(OP_ItemPacket); }
//ENCODE(OP_ItemPacket) {
//	//consume the packet
//	EQApplicationPacket *in = *p;
//	*p = nullptr;
//
//	unsigned char *__emu_buffer = in->pBuffer;
//	ItemPacket_Struct *old_item_pkt=(ItemPacket_Struct *)__emu_buffer;
//	InternalSerializedItem_Struct *int_struct=(InternalSerializedItem_Struct *)(old_item_pkt->SerializedItem);
//
//	uint32 length;
//	char *serialized=SerializeItem((ItemInst *)int_struct->inst,int_struct->slot_id,&length,0);
//
//	if (!serialized) {
//		_log(NET__STRUCTS, "Serialization failed on item slot %d.",int_struct->slot_id);
//		delete in;
//		return;
//	}
//	in->size = length+4;
//	in->pBuffer = new unsigned char[in->size];
//	ItemPacket_Struct *new_item_pkt=(ItemPacket_Struct *)in->pBuffer;
//	new_item_pkt->PacketType=old_item_pkt->PacketType;
//	memcpy(new_item_pkt->SerializedItem,serialized,length);
//
//	delete[] __emu_buffer;
//	safe_delete_array(serialized);
//	dest->FastQueuePacket(&in, ack_req);
//}

//ENCODE(OP_GuildMemberList) {
//	//consume the packet
//	EQApplicationPacket *in = *p;
//	*p = nullptr;
//
//	//store away the emu struct
//	unsigned char *__emu_buffer = in->pBuffer;
//	Internal_GuildMembers_Struct *emu = (Internal_GuildMembers_Struct *) in->pBuffer;
//
//
//
//	//make a new EQ buffer.
//	uint32 pnl = strlen(emu->player_name);
//	uint32 length = sizeof(structs::GuildMembers_Struct) + pnl +
//		emu->count*sizeof(structs::GuildMemberEntry_Struct)
//		+ emu->name_length + emu->note_length;
//	in->pBuffer = new uint8[length];
//	in->size = length;
//	//no memset since we fill every byte.
//
//	uint8 *buffer;
//	buffer = in->pBuffer;
//
//	//easier way to setup GuildMembers_Struct
//	//set prefix name
//	strcpy((char *)buffer, emu->player_name);
//	buffer += pnl;
//	*buffer = '\0';
//	buffer++;
//
//	//add member count.
//	*((uint32 *) buffer) = htonl( emu->count );
//	buffer += sizeof(uint32);
//
//	if(emu->count > 0) {
//		Internal_GuildMemberEntry_Struct *emu_e = emu->member;
//		const char *emu_name = (const char *) (__emu_buffer +
//				sizeof(Internal_GuildMembers_Struct) + //skip header
//				emu->count * sizeof(Internal_GuildMemberEntry_Struct)	//skip static length member data
//				);
//		const char *emu_note = (emu_name +
//				emu->name_length + //skip name contents
//				emu->count	//skip string terminators
//				);
//
//		structs::GuildMemberEntry_Struct *e = (structs::GuildMemberEntry_Struct *) buffer;
//
//		uint32 r;
//		for(r = 0; r < emu->count; r++, emu_e++) {
//
//			//the order we set things here must match the struct
//
////nice helper macro
///*#define SlideStructString(field, str) \
//		strcpy(e->field, str.c_str()); \
//		e = (GuildMemberEntry_Struct *) ( ((uint8 *)e) + str.length() )*/
//#define SlideStructString(field, str) \
//		{ \
//			int sl = strlen(str); \
//			memcpy(e->field, str, sl+1); \
//			e = (structs::GuildMemberEntry_Struct *) ( ((uint8 *)e) + sl ); \
//			str += sl + 1; \
//		}
//#define PutFieldN(field) \
//		e->field = htonl(emu_e->field)
//
//			SlideStructString( name, emu_name );
//			PutFieldN(level);
//			PutFieldN(banker);
//			PutFieldN(class_);
//			PutFieldN(rank);
//			PutFieldN(time_last_on);
//			PutFieldN(tribute_enable);
//			PutFieldN(total_tribute);
//			PutFieldN(last_tribute);
//			e->unknown_one = htonl(1);
//			SlideStructString( public_note, emu_note );
//			e->zoneinstance = 0;
//			e->zone_id = htons(emu_e->zone_id);
//
//
//#undef SlideStructString
//#undef PutFieldN
//
//			e++;
//		}
//	}
//
//
//	delete[] __emu_buffer;
//
//	dest->FastQueuePacket(&in, ack_req);
//}


//ENCODE(OP_SpawnDoor) {
//	SETUP_VAR_ENCODE(Door_Struct);
//	int door_count = __packet->size/sizeof(Door_Struct);
//	int total_length = door_count * sizeof(structs::Door_Struct);
//	ALLOC_VAR_ENCODE(structs::Door_Struct, total_length);
//	int r;
//	for(r = 0; r < door_count; r++) {
//		strcpy(eq[r].name, emu[r].name);
//		eq[r].xPos = emu[r].xPos;
//		eq[r].yPos = emu[r].yPos;
//		eq[r].zPos = emu[r].zPos;
//		eq[r].heading = emu[r].heading;
//		eq[r].incline = emu[r].incline;
//		eq[r].size = emu[r].size;
//		eq[r].doorId = emu[r].doorId;
//		eq[r].opentype = emu[r].opentype;
//		eq[r].state_at_spawn = emu[r].state_at_spawn;
//		eq[r].invert_state = emu[r].invert_state;
//		eq[r].door_param = emu[r].door_param;
//		eq[r].unknown0076 = 0;
//		eq[r].unknown0077 = 1; // Both must be 1 to allow clicking doors
//		eq[r].unknown0078 = 0;
//		eq[r].unknown0079 = 1; // Both must be 1 to allow clicking doors
//		eq[r].unknown0080 = 0;
//		eq[r].unknown0081 = 0;
//		eq[r].unknown0082 = 0;
//	}
//	FINISH_ENCODE();
//}

//ENCODE(OP_GroundSpawn)
//{
//
//	// We are not encoding the spawn_id field here, or a size but it doesn't appear to matter.
//	//
//	EQApplicationPacket *in = *p;
//	*p = nullptr;
//
//	Object_Struct *emu = (Object_Struct *) in->pBuffer;
//
//	unsigned char *__emu_buffer = in->pBuffer;
//
//	in->size = strlen(emu->object_name) + 58;
//
//	in->pBuffer = new unsigned char[in->size];
//
//	char *OutBuffer = (char *)in->pBuffer;
//
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, emu->drop_id);
//	VARSTRUCT_ENCODE_STRING(OutBuffer, emu->object_name);
//	VARSTRUCT_ENCODE_TYPE(uint16, OutBuffer, emu->zone_id);
//	VARSTRUCT_ENCODE_TYPE(uint16, OutBuffer, emu->zone_instance);
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, 0);	// Unknown, observed 0x00006762
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, 0);	// Unknown, observer 0x7fffbb64
//	VARSTRUCT_ENCODE_TYPE(float, OutBuffer, emu->heading);
//	// This next field is actually a float. There is a groundspawn in freeportwest (sack of money sitting on some barrels) which requires this
//	// field to be set to (float)255.0 to appear at all, and also the size field below to be 5, to be the correct size. I think SoD has the same
//	// issue.
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, 0);
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, 0);	// Unknown, observed 0
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, 0);	// This appears to be the size field.
//	VARSTRUCT_ENCODE_TYPE(float, OutBuffer, emu->y);
//	VARSTRUCT_ENCODE_TYPE(float, OutBuffer, emu->x);
//	VARSTRUCT_ENCODE_TYPE(float, OutBuffer, emu->z);
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, emu->object_type);	// Unknown, observed 0x00000014
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, 0xffffffff);	// Unknown, observed 0xffffffff
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, 0);	// Unknown, observed 0x00000014
//	VARSTRUCT_ENCODE_TYPE(uint8, OutBuffer, 0);	// Unknown, observed 0x00
//
//	delete[] __emu_buffer;
//
//	dest->FastQueuePacket(&in, ack_req);
//}

//ENCODE(OP_ManaChange) {
//	ENCODE_LENGTH_EXACT(ManaChange_Struct);
//	SETUP_DIRECT_ENCODE(ManaChange_Struct, structs::ManaChange_Struct);
//	OUT(new_mana);
//	OUT(stamina);
//	OUT(spell_id);
//	eq->unknown16 = -1; // Self Interrupt/Success = -1, Fizzle = 1, Other Interrupt = 2?
//	FINISH_ENCODE();
//}

//ENCODE(OP_OnLevelMessage)
//{
//	ENCODE_LENGTH_EXACT(OnLevelMessage_Struct);
//	SETUP_DIRECT_ENCODE(OnLevelMessage_Struct, structs::OnLevelMessage_Struct);
//	memcpy(eq->Title, emu->Title, sizeof(eq->Title));
//	memcpy(eq->Text, emu->Text, sizeof(eq->Text));
//	OUT(Buttons);
//	OUT(Duration);
//	OUT(PopupID);
//	OUT(NegativeID);
//	// These two field names are used if Buttons == 1.
//	OUT_str(ButtonName0);
//	OUT_str(ButtonName1);
//	FINISH_ENCODE();
//}

//ENCODE(OP_Illusion) {
//	ENCODE_LENGTH_EXACT(Illusion_Struct);
//	SETUP_DIRECT_ENCODE(Illusion_Struct, structs::Illusion_Struct);
//	OUT(spawnid);
//	OUT_str(charname);
//	OUT(race);
//	OUT(unknown006[0]);
//	OUT(unknown006[1]);
//	OUT(gender);
//	OUT(texture);
//	OUT(helmtexture);
//	OUT(face);
//	OUT(hairstyle);
//	OUT(haircolor);
//	OUT(beard);
//	OUT(beardcolor);
//	OUT(size);
//	OUT(drakkin_heritage);
//	OUT(drakkin_tattoo);
//	OUT(drakkin_details);
//
//	FINISH_ENCODE();
//}

//ENCODE(OP_ShopPlayerBuy)
//{
//	ENCODE_LENGTH_EXACT(Merchant_Sell_Struct);
//	SETUP_DIRECT_ENCODE(Merchant_Sell_Struct, structs::Merchant_Sell_Struct);
//	OUT(npcid);
//	OUT(playerid);
//	OUT(itemslot);
//	OUT(quantity);
//	OUT(price);
//
//	FINISH_ENCODE();
//}

//ENCODE(OP_ClientUpdate) {
//	ENCODE_LENGTH_EXACT(PlayerPositionUpdateServer_Struct);
//	SETUP_DIRECT_ENCODE(PlayerPositionUpdateServer_Struct, structs::PlayerPositionUpdateServer_Struct);
//	OUT(spawn_id);
//	OUT(x_pos);
//	OUT(delta_x);
//	OUT(delta_y);
//	OUT(z_pos);
//	OUT(delta_heading);
//	OUT(y_pos);
//	OUT(delta_z);
//	OUT(animation);
//	OUT(heading);
//	FINISH_ENCODE();
//}

ENCODE(OP_ExpansionInfo) {
	ENCODE_LENGTH_EXACT(ExpansionInfo_Struct);
	SETUP_DIRECT_ENCODE(ExpansionInfo_Struct, structs::ExpansionInfo_Struct);
	OUT(Expansions);
	FINISH_ENCODE();
}

ENCODE(OP_LogServer) {
	ENCODE_LENGTH_EXACT(LogServer_Struct);
 	SETUP_DIRECT_ENCODE(LogServer_Struct, structs::LogServer_Struct);
 	strcpy(eq->worldshortname, emu->worldshortname);

 	OUT(enablevoicemacros);
 	OUT(enablemail);
	OUT(enable_pvp);
	OUT(enable_FV);

	eq->unknown016 = 1;
	eq->unknown020[0] = 1;

 	// These next two need to be set like this for the Tutorial Button to work.
 	eq->unknown263[0] = 0;
 	eq->unknown263[2] = 1;
 	eq->unknown263[4] = 1;
 	eq->unknown263[5] = 1;
 	eq->unknown263[6] = 1;
 	eq->unknown263[9] = 8;
 	eq->unknown263[19] = 0x80;
 	eq->unknown263[20] = 0x3f;
 	eq->unknown263[23] = 0x80;
 	eq->unknown263[24] = 0x3f;
 	eq->unknown263[33] = 1;

 	FINISH_ENCODE();
}

//ENCODE(OP_Consider) {
//	ENCODE_LENGTH_EXACT(Consider_Struct);
//	SETUP_DIRECT_ENCODE(Consider_Struct, structs::Consider_Struct);
//	OUT(playerid);
//	OUT(targetid);
//	OUT(faction);
//	OUT(level);
//	OUT(pvpcon);
//	FINISH_ENCODE();
//}

ENCODE(OP_Action) {
	ENCODE_LENGTH_EXACT(Action_Struct);
	SETUP_DIRECT_ENCODE(Action_Struct, structs::ActionAlt_Struct);
	OUT(target);
	OUT(source);
	OUT(level);
	eq->instrument_mod = 1.0f + (emu->instrument_mod - 10) / 10.0f;
	eq->knockback_angle = emu->sequence;
	OUT(type);
	OUT(spell);
	eq->level2 = eq->level;
	eq->effect_flag = emu->buff_unknown;
	eq->unknown37 = 0x01;
	eq->unknown44 = 0xFFFFFFFF;
	eq->unknown48 = 0xFFFFFFFF;
	eq->unknown52 = 0xFFFFFFFF;

	/*OUT(target);
	OUT(source);
	OUT(level);
	OUT(instrument_mod);
	eq->sequence = emu->sequence;
	OUT(type);
	//OUT(damage);
	OUT(spell);
	eq->level2 = emu->level;
	OUT(buff_unknown); // if this is 4, a buff icon is made
	//eq->unknown0036 = -1;
	//eq->unknown0040 = -1;
	//eq->unknown0044 = -1;*/
	FINISH_ENCODE();
}

ENCODE(OP_Buff) {
	ENCODE_LENGTH_EXACT(SpellBuffFade_Struct);
	SETUP_DIRECT_ENCODE(SpellBuffFade_Struct, structs::SpellBuffFade_Struct_Underfoot);
	OUT(entityid);
	OUT(slot);
	OUT(level);
	OUT(effect);
	//eq->unknown7 = 10;
	OUT(spellid);
	OUT(duration);
	OUT(slotid);
	OUT(bufffade);	// Live (October 2011) sends a 2 rather than 0 when a buff is created, but it doesn't seem to matter.
	eq->unknown008 = 1.0f;
	FINISH_ENCODE();
}

//ENCODE(OP_CancelTrade) {
//	ENCODE_LENGTH_EXACT(CancelTrade_Struct);
//	SETUP_DIRECT_ENCODE(CancelTrade_Struct, structs::CancelTrade_Struct);
//	OUT(fromid);
//	OUT(action);
//	FINISH_ENCODE();
//}

//ENCODE(OP_ShopPlayerSell) {
//	ENCODE_LENGTH_EXACT(Merchant_Purchase_Struct);
//	SETUP_DIRECT_ENCODE(Merchant_Purchase_Struct, structs::Merchant_Purchase_Struct);
//	OUT(npcid);
//	eq->itemslot = TitaniumToUnderfootSlot(emu->itemslot);
//	OUT(quantity);
//	OUT(price);
//	FINISH_ENCODE();
//}

//ENCODE(OP_ApplyPoison) {
//	ENCODE_LENGTH_EXACT(ApplyPoison_Struct);
//	SETUP_DIRECT_ENCODE(ApplyPoison_Struct, structs::ApplyPoison_Struct);
//	eq->inventorySlot = TitaniumToUnderfootSlot(emu->inventorySlot);
//	OUT(success);
//	FINISH_ENCODE();
//}

ENCODE(OP_Trader) {

	if((*p)->size != sizeof(TraderBuy_Struct)) {
		EQApplicationPacket *in = *p;
		*p = nullptr;
		dest->FastQueuePacket(&in, ack_req);
		return;
	}
	ENCODE_FORWARD(OP_TraderBuy);
}

ENCODE(OP_TraderBuy) {

	ENCODE_LENGTH_EXACT(TraderBuy_Struct);
	SETUP_DIRECT_ENCODE(TraderBuy_Struct, structs::TraderBuy_Struct);

	OUT(Action);
	OUT(Price);
	OUT(TraderID);
	memcpy(eq->ItemName, emu->ItemName, sizeof(eq->ItemName));
	OUT(ItemID);
	OUT(Quantity);
	OUT(AlreadySold);

	FINISH_ENCODE();
}

//ENCODE(OP_LootItem) {
//
//	ENCODE_LENGTH_EXACT(LootingItem_Struct);
//	SETUP_DIRECT_ENCODE(LootingItem_Struct, structs::LootingItem_Struct);
//	OUT(lootee);
//	OUT(looter);
//	eq->slot_id = emu->slot_id + 1;
//	OUT(auto_loot);
//
//	FINISH_ENCODE();
//}

ENCODE(OP_TributeItem) {
	ENCODE_LENGTH_EXACT(TributeItem_Struct);
	SETUP_DIRECT_ENCODE(TributeItem_Struct, structs::TributeItem_Struct);

	eq->slot = TitaniumToUnderfootSlot(emu->slot);
	OUT(quantity);
	OUT(tribute_master_id);
	OUT(tribute_points);

	FINISH_ENCODE();
}

ENCODE(OP_SomeItemPacketMaybe) {
	// This Opcode is not named very well. It is used for the animation of arrows leaving the player's bow
	// and flying to the target.
	//

	ENCODE_LENGTH_EXACT(Arrow_Struct);
	SETUP_DIRECT_ENCODE(Arrow_Struct, structs::Arrow_Struct);

	OUT(src_y);
	OUT(src_x);
	OUT(src_z);
	OUT(velocity);
	OUT(launch_angle);
	OUT(tilt);
	OUT(arc);
	OUT(source_id);
	OUT(target_id);
	OUT(item_id);

	eq->unknown070 = 135; // This needs to be set to something, else we get a 1HS animation instead of ranged.

	OUT(item_type);
	OUT(skill);

	strcpy(eq->model_name, emu->model_name);

	FINISH_ENCODE();
}

//ENCODE(OP_ReadBook) {
//
//	ENCODE_LENGTH_ATLEAST(BookText_Struct);
//	SETUP_DIRECT_ENCODE(BookText_Struct, structs::BookRequest_Struct);
//
//	if(emu->window == 0xFF)
//		eq->window = 0xFFFFFFFF;
//	else
//		eq->window = emu->window;
//	OUT(type);
//	eq->invslot = TitaniumToUnderfootSlot(emu->invslot);
//	strn0cpy(eq->txtfile, emu->booktext, sizeof(eq->txtfile));
//	FINISH_ENCODE();
//}

ENCODE(OP_Stun) {

	ENCODE_LENGTH_EXACT(Stun_Struct);
	SETUP_DIRECT_ENCODE(Stun_Struct, structs::Stun_Struct);
	OUT(duration);
	eq->unknown005 = 163;
	eq->unknown006 = 67;

	FINISH_ENCODE();
}

ENCODE(OP_ZonePlayerToBind)
{
	ENCODE_LENGTH_ATLEAST(ZonePlayerToBind_Struct);
	ZonePlayerToBind_Struct *zps = (ZonePlayerToBind_Struct*)(*p)->pBuffer;

	std::stringstream ss(std::stringstream::in | std::stringstream::out | std::stringstream::binary);

	unsigned char *buffer1 = new unsigned char[sizeof(structs::ZonePlayerToBindHeader_Struct) + strlen(zps->zone_name)];
	structs::ZonePlayerToBindHeader_Struct *zph = (structs::ZonePlayerToBindHeader_Struct*)buffer1;
	unsigned char *buffer2 = new unsigned char[sizeof(structs::ZonePlayerToBindFooter_Struct)];
	structs::ZonePlayerToBindFooter_Struct *zpf = (structs::ZonePlayerToBindFooter_Struct*)buffer2;

	zph->x = zps->x;
	zph->y = zps->y;
	zph->z = zps->z;
	zph->heading = zps->heading;
	zph->bind_zone_id = zps->bind_zone_id;
	zph->bind_instance_id = zps->bind_instance_id;
	strcpy(zph->zone_name, zps->zone_name);

	zpf->unknown021 = 1;
	zpf->unknown022 = 0;
	zpf->unknown023 = 0;
	zpf->unknown024 = 0;

	ss.write((const char*)buffer1, (sizeof(structs::ZonePlayerToBindHeader_Struct) + strlen(zps->zone_name)));
	ss.write((const char*)buffer2, sizeof(structs::ZonePlayerToBindFooter_Struct));

	delete[] buffer1;
	delete[] buffer2;
	delete[] (*p)->pBuffer;

	(*p)->pBuffer = new unsigned char[ss.str().size()];
	(*p)->size = ss.str().size();

	memcpy((*p)->pBuffer, ss.str().c_str(), ss.str().size());
	dest->FastQueuePacket(&(*p));
}

ENCODE(OP_AdventureMerchantSell) {
	ENCODE_LENGTH_EXACT(Adventure_Sell_Struct);
	SETUP_DIRECT_ENCODE(Adventure_Sell_Struct, structs::Adventure_Sell_Struct);

	eq->unknown000 = 1;
	OUT(npcid);
	eq->slot = TitaniumToUnderfootSlot(emu->slot);
	OUT(charges);
	OUT(sell_price);

	FINISH_ENCODE();
}

ENCODE(OP_RaidUpdate)
{
	EQApplicationPacket *inapp = *p;
	*p = nullptr;
	unsigned char * __emu_buffer = inapp->pBuffer;
	RaidGeneral_Struct *raid_gen = (RaidGeneral_Struct*)__emu_buffer;

	if(raid_gen->action == 0) // raid add has longer length than other raid updates
	{
		RaidAddMember_Struct* in_add_member = (RaidAddMember_Struct*)__emu_buffer;

		EQApplicationPacket *outapp = new EQApplicationPacket(OP_RaidUpdate, sizeof(structs::RaidAddMember_Struct));
		structs::RaidAddMember_Struct *add_member = (structs::RaidAddMember_Struct*)outapp->pBuffer;

		add_member->raidGen.action = in_add_member->raidGen.action;
		add_member->raidGen.parameter = in_add_member->raidGen.parameter;
		strn0cpy(add_member->raidGen.leader_name, in_add_member->raidGen.leader_name, 64);
		strn0cpy(add_member->raidGen.player_name, in_add_member->raidGen.player_name, 64);
		add_member->_class = in_add_member->_class;
		add_member->level= in_add_member->level;
		add_member->isGroupLeader = in_add_member->isGroupLeader;
		add_member->flags[0] = in_add_member->flags[0];
		add_member->flags[1] = in_add_member->flags[1];
		add_member->flags[2] = in_add_member->flags[2];
		add_member->flags[3] = in_add_member->flags[3];
		add_member->flags[4] = in_add_member->flags[4];
		dest->FastQueuePacket(&outapp);
	}
	else
	{
		RaidGeneral_Struct* in_raid_general = (RaidGeneral_Struct*)__emu_buffer;

		EQApplicationPacket *outapp = new EQApplicationPacket(OP_RaidUpdate, sizeof(structs::RaidGeneral_Struct));
		structs::RaidGeneral_Struct *raid_general = (structs::RaidGeneral_Struct*)outapp->pBuffer;
		strn0cpy(raid_general->leader_name, in_raid_general->leader_name, 64);
		strn0cpy(raid_general->player_name, in_raid_general->player_name, 64);
		raid_general->action = in_raid_general->action;
		raid_general->parameter = in_raid_general->parameter;
		dest->FastQueuePacket(&outapp);
	}
	delete[] __emu_buffer;
}

ENCODE(OP_RaidJoin)
{
	EQApplicationPacket *inapp = *p;
	unsigned char * __emu_buffer = inapp->pBuffer;
	RaidCreate_Struct *raid_create = (RaidCreate_Struct*)__emu_buffer;

	EQApplicationPacket *outapp_create = new EQApplicationPacket(OP_RaidUpdate, sizeof(structs::RaidGeneral_Struct));
	structs::RaidGeneral_Struct *general = (structs::RaidGeneral_Struct*)outapp_create->pBuffer;

	general->action = 8;
	general->parameter = 1;
	strn0cpy(general->leader_name, raid_create->leader_name, 64);
	strn0cpy(general->player_name, raid_create->leader_name, 64);

	dest->FastQueuePacket(&outapp_create);
	delete[] __emu_buffer;
}

ENCODE(OP_VetRewardsAvaliable)
{
	EQApplicationPacket *inapp = *p;
	unsigned char * __emu_buffer = inapp->pBuffer;

	uint32 count = ((*p)->Size() / sizeof(InternalVeteranReward));
	*p = nullptr;

	EQApplicationPacket *outapp_create = new EQApplicationPacket(OP_VetRewardsAvaliable, (sizeof(structs::VeteranReward)*count));
	uchar *old_data = __emu_buffer;
	uchar *data = outapp_create->pBuffer;
	for(unsigned int i = 0; i < count; ++i)
	{
		structs::VeteranReward *vr = (structs::VeteranReward*)data;
		InternalVeteranReward *ivr = (InternalVeteranReward*)old_data;

		vr->claim_count = ivr->claim_count;
		vr->claim_id = ivr->claim_id;
		vr->number_available = ivr->number_available;
		for(int x = 0; x < 8; ++x)
		{
			vr->items[x].item_id = ivr->items[x].item_id;
			strcpy(vr->items[x].item_name, ivr->items[x].item_name);
			vr->items[x].charges = ivr->items[x].charges;
		}

		old_data += sizeof(InternalVeteranReward);
		data += sizeof(structs::VeteranReward);
	}

	dest->FastQueuePacket(&outapp_create);
	delete inapp;
}

//ENCODE(OP_WhoAllResponse)
//{
//	EQApplicationPacket *in = *p;
//	*p = nullptr;
//
//	char *InBuffer = (char *)in->pBuffer;
//
//	WhoAllReturnStruct *wars = (WhoAllReturnStruct*)InBuffer;
//
//	int Count = wars->playercount;
//
//	EQApplicationPacket *outapp = new EQApplicationPacket(OP_WhoAllResponse, in->size + (Count * 4));
//
//	char *OutBuffer = (char *)outapp->pBuffer;
//
//	memcpy(OutBuffer, InBuffer, sizeof(WhoAllReturnStruct));
//
//	OutBuffer += sizeof(WhoAllReturnStruct);
//	InBuffer += sizeof(WhoAllReturnStruct);
//
//	for(int i = 0; i < Count; ++i)
//	{
//		uint32 x;
//
//		x = VARSTRUCT_DECODE_TYPE(uint32, InBuffer);
//
//		VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, x);
//
//		InBuffer += 4;
//		VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, 0);
//		VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, 0xffffffff);
//
//		char Name[64];
//
//
//		VARSTRUCT_DECODE_STRING(Name, InBuffer);	// Char Name
//		VARSTRUCT_ENCODE_STRING(OutBuffer, Name);
//
//		x = VARSTRUCT_DECODE_TYPE(uint32, InBuffer);
//		VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, x);
//
//		VARSTRUCT_DECODE_STRING(Name, InBuffer);	// Guild Name
//		VARSTRUCT_ENCODE_STRING(OutBuffer, Name);
//
//		for(int j = 0; j < 7; ++j)
//		{
//			x = VARSTRUCT_DECODE_TYPE(uint32, InBuffer);
//			VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, x);
//		}
//
//		VARSTRUCT_DECODE_STRING(Name, InBuffer);		// Account
//		VARSTRUCT_ENCODE_STRING(OutBuffer, Name);
//
//		x = VARSTRUCT_DECODE_TYPE(uint32, InBuffer);
//		VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, x);
//	}
//
//	//_hex(NET__ERROR, outapp->pBuffer, outapp->size);
//	dest->FastQueuePacket(&outapp);
//
//	delete in;
//}

//ENCODE(OP_GroupInvite) {
//	ENCODE_LENGTH_EXACT(GroupGeneric_Struct);
//	SETUP_DIRECT_ENCODE(GroupGeneric_Struct, structs::GroupInvite_Struct);
//	memcpy(eq->invitee_name, emu->name1, sizeof(eq->invitee_name));
//	memcpy(eq->inviter_name, emu->name2, sizeof(eq->inviter_name));
//	FINISH_ENCODE();
//}

//ENCODE(OP_GroupFollow) {
//	ENCODE_LENGTH_EXACT(GroupGeneric_Struct);
//	SETUP_DIRECT_ENCODE(GroupGeneric_Struct, structs::GroupFollow_Struct);
//	memcpy(eq->name1, emu->name1, sizeof(eq->name1));
//	memcpy(eq->name2, emu->name2, sizeof(eq->name2));
//	FINISH_ENCODE();
//}

//ENCODE(OP_GroupFollow2) {
//	ENCODE_LENGTH_EXACT(GroupGeneric_Struct);
//	SETUP_DIRECT_ENCODE(GroupGeneric_Struct, structs::GroupFollow_Struct);
//	memcpy(eq->name1, emu->name1, sizeof(eq->name1));
//	memcpy(eq->name2, emu->name2, sizeof(eq->name2));
//	FINISH_ENCODE();
//}

//ENCODE(OP_GroupCancelInvite)
//{
//	ENCODE_LENGTH_EXACT(GroupCancel_Struct);
//	SETUP_DIRECT_ENCODE(GroupCancel_Struct, structs::GroupCancel_Struct);
//	memcpy(eq->name1, emu->name1, sizeof(eq->name1));
//	memcpy(eq->name2, emu->name2, sizeof(eq->name2));
//	OUT(toggle);
//	FINISH_ENCODE();
//}

ENCODE(OP_GroupUpdate)
{
	//_log(NET__ERROR, "OP_GroupUpdate");
	EQApplicationPacket *in = *p;

	GroupJoin_Struct *gjs = (GroupJoin_Struct*)in->pBuffer;

	//_log(NET__ERROR, "Received outgoing OP_GroupUpdate with action code %i", gjs->action);
	if((gjs->action == groupActLeave) || (gjs->action == groupActDisband))
	{
		if((gjs->action == groupActDisband) || !strcmp(gjs->yourname, gjs->membername))
		{
			//_log(NET__ERROR, "Group Leave, yourname = %s, membername = %s", gjs->yourname, gjs->membername);

			EQApplicationPacket *outapp = new EQApplicationPacket(OP_GroupDisbandYou, sizeof(structs::GroupGeneric_Struct));

			structs::GroupGeneric_Struct *ggs = (structs::GroupGeneric_Struct*)outapp->pBuffer;
			memcpy(ggs->name1, gjs->yourname, sizeof(ggs->name1));
			memcpy(ggs->name2, gjs->membername, sizeof(ggs->name1));
			dest->FastQueuePacket(&outapp);

			// Make an empty GLAA packet to clear out their useable GLAAs
			//
			outapp = new EQApplicationPacket(OP_GroupLeadershipAAUpdate, sizeof(GroupLeadershipAAUpdate_Struct));

			dest->FastQueuePacket(&outapp);

			delete in;

			return;
		}
		//if(gjs->action == groupActLeave)
		//	_log(NET__ERROR, "Group Leave, yourname = %s, membername = %s", gjs->yourname, gjs->membername);

		EQApplicationPacket *outapp = new EQApplicationPacket(OP_GroupDisbandOther, sizeof(structs::GroupGeneric_Struct));

		structs::GroupGeneric_Struct *ggs = (structs::GroupGeneric_Struct*)outapp->pBuffer;
		memcpy(ggs->name1, gjs->yourname, sizeof(ggs->name1));
		memcpy(ggs->name2, gjs->membername, sizeof(ggs->name2));
		//_hex(NET__ERROR, outapp->pBuffer, outapp->size);
		dest->FastQueuePacket(&outapp);

		delete in;
		return;


	}

	if(in->size == sizeof(GroupUpdate2_Struct))
	{
		// Group Update2
		//_log(NET__ERROR, "Struct is GroupUpdate2");

		unsigned char *__emu_buffer = in->pBuffer;
		GroupUpdate2_Struct *gu2 = (GroupUpdate2_Struct*) __emu_buffer;

		//_log(NET__ERROR, "Yourname is %s", gu2->yourname);

		int MemberCount = 1;

		int PacketLength = 8 + strlen(gu2->leadersname) + 1 + 22 + strlen(gu2->yourname) + 1;

		for(int i = 0; i < 5; ++i)
		{
			//_log(NET__ERROR, "Membername[%i] is %s", i,  gu2->membername[i]);
			if(gu2->membername[i][0] != '\0')
			{
				PacketLength += (22 + strlen(gu2->membername[i]) + 1);
				++MemberCount;
			}
		}

		//_log(NET__ERROR, "Leadername is %s", gu2->leadersname);

		EQApplicationPacket *outapp = new EQApplicationPacket(OP_GroupUpdateB, PacketLength);

		char *Buffer = (char *)outapp->pBuffer;

		// Header
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0);	// Think this should be SpawnID, but it doesn't seem to matter
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, MemberCount);
		VARSTRUCT_ENCODE_STRING(Buffer, gu2->leadersname);

		// Leader
		//

		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0);
		VARSTRUCT_ENCODE_STRING(Buffer, gu2->yourname);
		VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0);
		VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0);
		//VARSTRUCT_ENCODE_STRING(Buffer, "");
		VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0);	// This is a string
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0x46);	// Observed 0x41 and 0x46 here
		VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0);
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0);
		VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0);
		VARSTRUCT_ENCODE_TYPE(uint16, Buffer, 0);

		int MemberNumber = 1;

		for(int i = 0; i < 5; ++i)
		{
			if(gu2->membername[i][0] == '\0')
				continue;

			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, MemberNumber++);
			VARSTRUCT_ENCODE_STRING(Buffer, gu2->membername[i]);
			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0);
			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0);
			//VARSTRUCT_ENCODE_STRING(Buffer, "");
			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0);	// This is a string
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0x41);	// Observed 0x41 and 0x46 here
			VARSTRUCT_ENCODE_TYPE(uint8, Buffer, 0);
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0);	// Low byte is Main Assist Flag
			VARSTRUCT_ENCODE_TYPE(uint32, Buffer, 0);
			VARSTRUCT_ENCODE_TYPE(uint16, Buffer, 0);
		}

		//_hex(NET__ERROR, outapp->pBuffer, outapp->size);
		dest->FastQueuePacket(&outapp);

		outapp = new EQApplicationPacket(OP_GroupLeadershipAAUpdate, sizeof(GroupLeadershipAAUpdate_Struct));

		GroupLeadershipAAUpdate_Struct *GLAAus = (GroupLeadershipAAUpdate_Struct*)outapp->pBuffer;

		GLAAus->NPCMarkerID = gu2->NPCMarkerID;
		memcpy(&GLAAus->LeaderAAs, &gu2->leader_aas, sizeof(GLAAus->LeaderAAs));

		dest->FastQueuePacket(&outapp);
		delete in;

		return;

	}
	//_log(NET__ERROR, "Generic GroupUpdate, yourname = %s, membername = %s", gjs->yourname, gjs->membername);
	ENCODE_LENGTH_EXACT(GroupJoin_Struct);
	SETUP_DIRECT_ENCODE(GroupJoin_Struct, structs::GroupJoin_Struct);

	memcpy(eq->membername, emu->membername, sizeof(eq->membername));

	EQApplicationPacket *outapp = new EQApplicationPacket(OP_GroupLeadershipAAUpdate, sizeof(GroupLeadershipAAUpdate_Struct));

	GroupLeadershipAAUpdate_Struct *GLAAus = (GroupLeadershipAAUpdate_Struct*)outapp->pBuffer;

	GLAAus->NPCMarkerID = emu->NPCMarkerID;

	memcpy(&GLAAus->LeaderAAs, &emu->leader_aas, sizeof(GLAAus->LeaderAAs));
	//_hex(NET__ERROR, __packet->pBuffer, __packet->size);
	FINISH_ENCODE();

	dest->FastQueuePacket(&outapp);
}

//ENCODE(OP_ChannelMessage)
//{
//	EQApplicationPacket *in = *p;
//	*p = nullptr;
//
//	ChannelMessage_Struct *emu = (ChannelMessage_Struct *) in->pBuffer;
//
//	unsigned char *__emu_buffer = in->pBuffer;
//
//	in->size = strlen(emu->sender) + 1 + strlen(emu->targetname) + 1 + strlen(emu->message) + 1 + 36;
//
//	in->pBuffer = new unsigned char[in->size];
//
//	char *OutBuffer = (char *)in->pBuffer;
//
//	VARSTRUCT_ENCODE_STRING(OutBuffer, emu->sender);
//	VARSTRUCT_ENCODE_STRING(OutBuffer, emu->targetname);
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, 0);	// Unknown
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, emu->language);
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, emu->chan_num);
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, 0);	// Unknown
//	VARSTRUCT_ENCODE_TYPE(uint8, OutBuffer, 0);	// Unknown
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, emu->skill_in_language);
//	VARSTRUCT_ENCODE_STRING(OutBuffer, emu->message);
//
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, 0);	// Unknown
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, 0);	// Unknown
//	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, 0);	// Unknown
//	VARSTRUCT_ENCODE_TYPE(uint16, OutBuffer, 0);	// Unknown
//	VARSTRUCT_ENCODE_TYPE(uint8, OutBuffer, 0);	// Unknown
//
//	delete[] __emu_buffer;
//
//	dest->FastQueuePacket(&in, ack_req);
//}

ENCODE(OP_GuildsList)
{
	EQApplicationPacket *in = *p;
	*p = nullptr;

	uint32 NumberOfGuilds = in->size / 64;

	uint32 PacketSize = 68;	// 64 x 0x00 + a uint32 that I am guessing is the highest guild ID in use.

	unsigned char *__emu_buffer = in->pBuffer;

	char *InBuffer = (char *)__emu_buffer;

	uint32 HighestGuildID = 0;

	for(unsigned int i = 0; i < NumberOfGuilds; ++i)
	{
		if(InBuffer[0])
		{
			PacketSize += (5 + strlen(InBuffer));
			HighestGuildID = i - 1;
		}
		InBuffer += 64;
	}

	PacketSize++;	// Appears to be an extra 0x00 at the very end.

	in->size = PacketSize;

	in->pBuffer = new unsigned char[in->size];

	InBuffer = (char *)__emu_buffer;

	char *OutBuffer = (char *)in->pBuffer;

	// Init the first 64 bytes to zero, as per live.
	//
	memset(OutBuffer, 0, 64);

	OutBuffer += 64;

	VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, HighestGuildID);

	for(unsigned int i = 0; i < NumberOfGuilds; ++i)
	{
		if(InBuffer[0])
		{
			VARSTRUCT_ENCODE_TYPE(uint32, OutBuffer, i - 1);
			VARSTRUCT_ENCODE_STRING(OutBuffer, InBuffer);
		}
		InBuffer += 64;
	}

	VARSTRUCT_ENCODE_TYPE(uint8, OutBuffer, 0x00);

	delete[] __emu_buffer;

	dest->FastQueuePacket(&in, ack_req);
}

ENCODE(OP_DzExpeditionEndsWarning)
{
	ENCODE_LENGTH_EXACT(ExpeditionExpireWarning);
	SETUP_DIRECT_ENCODE(ExpeditionExpireWarning, structs::ExpeditionExpireWarning);
	OUT(minutes_remaining);
	FINISH_ENCODE();
}

ENCODE(OP_DzExpeditionInfo)
{
	ENCODE_LENGTH_EXACT(ExpeditionInfo_Struct);
	SETUP_DIRECT_ENCODE(ExpeditionInfo_Struct, structs::ExpeditionInfo_Struct);
	OUT(max_players);
	eq->unknown004 = 785316192;
	eq->unknown008 = 435601;
	strcpy(eq->expedition_name, emu->expedition_name);
	strcpy(eq->leader_name, emu->leader_name);
	FINISH_ENCODE();
}

ENCODE(OP_DzCompass)
{
	SETUP_VAR_ENCODE(ExpeditionCompass_Struct);
	ALLOC_VAR_ENCODE(structs::ExpeditionCompass_Struct, sizeof(structs::ExpeditionInfo_Struct) + sizeof(structs::ExpeditionCompassEntry_Struct) * emu->count);
	OUT(count);

	for(uint32 i = 0; i < emu->count; ++i)
	{
		OUT(entries[i].x);
		OUT(entries[i].y);
		OUT(entries[i].z);
	}

	FINISH_ENCODE();
}

ENCODE(OP_DzMemberList)
{
	SETUP_VAR_ENCODE(ExpeditionMemberList_Struct);
	std::stringstream ss(std::stringstream::in | std::stringstream::out | std::stringstream::binary);

	uint32 client_id = 0;
	uint8 null_term = 0;
	ss.write((const char*)&client_id, sizeof(uint32));
	ss.write((const char*)&emu->count, sizeof(uint32));
	for(uint32 i = 0; i < emu->count; ++i)
	{
		ss.write(emu->entries[i].name, strlen(emu->entries[i].name));
		ss.write((const char*)&null_term, sizeof(char));
		ss.write((const char*)&emu->entries[i].status, sizeof(char));
	}

	__packet->size = ss.str().length();
	__packet->pBuffer = new unsigned char[__packet->size];
	memcpy(__packet->pBuffer, ss.str().c_str(), __packet->size);

	FINISH_ENCODE();
}

ENCODE(OP_DzExpeditionList)
{
	SETUP_VAR_ENCODE(ExpeditionLockoutList_Struct);
	std::stringstream ss(std::stringstream::in | std::stringstream::out | std::stringstream::binary);
	uint32 client_id = 0;
	uint8 null_term = 0;
	ss.write((const char*)&client_id, sizeof(uint32));
	ss.write((const char*)&emu->count, sizeof(uint32));
	for(uint32 i = 0; i < emu->count; ++i)
	{
		ss.write(emu->entries[i].expedition, strlen(emu->entries[i].expedition));
		ss.write((const char*)&null_term, sizeof(char));
		ss.write((const char*)&emu->entries[i].time_left, sizeof(uint32));
		ss.write((const char*)&client_id, sizeof(uint32));
		ss.write(emu->entries[i].expedition_event, strlen(emu->entries[i].expedition_event));
		ss.write((const char*)&null_term, sizeof(char));
	}

	__packet->size = ss.str().length();
	__packet->pBuffer = new unsigned char[__packet->size];
	memcpy(__packet->pBuffer, ss.str().c_str(), __packet->size);

	FINISH_ENCODE();
}

ENCODE(OP_DzLeaderStatus)
{
	SETUP_VAR_ENCODE(ExpeditionLeaderSet_Struct);
	std::stringstream ss(std::stringstream::in | std::stringstream::out | std::stringstream::binary);
	uint32 client_id = 0;
	uint8 null_term = 0;

	ss.write((const char*)&client_id, sizeof(uint32));
	ss.write((const char*)&client_id, sizeof(uint32));
	ss.write(emu->leader_name, strlen(emu->leader_name));
	ss.write((const char*)&null_term, sizeof(char));
	ss.write((const char*)&client_id, sizeof(uint32));
	ss.write((const char*)&client_id, sizeof(uint32));//0xffffffff
	ss.write((const char*)&client_id, sizeof(uint32));
	ss.write((const char*)&client_id, sizeof(uint32));
	ss.write((const char*)&client_id, sizeof(uint32));
	ss.write((const char*)&client_id, sizeof(uint32));
	ss.write((const char*)&client_id, sizeof(uint32));
	ss.write((const char*)&client_id, sizeof(uint32));
	ss.write((const char*)&client_id, sizeof(uint32));
	ss.write((const char*)&client_id, sizeof(uint32));
	ss.write((const char*)&client_id, sizeof(uint32));
	ss.write((const char*)&client_id, sizeof(uint32));//1
	ss.write((const char*)&client_id, sizeof(uint32));

	__packet->size = ss.str().length();
	__packet->pBuffer = new unsigned char[__packet->size];
	memcpy(__packet->pBuffer, ss.str().c_str(), __packet->size);

	FINISH_ENCODE();
}

ENCODE(OP_DzJoinExpeditionConfirm)
{
	ENCODE_LENGTH_EXACT(ExpeditionJoinPrompt_Struct);
	SETUP_DIRECT_ENCODE(ExpeditionJoinPrompt_Struct, structs::ExpeditionJoinPrompt_Struct);
	strcpy(eq->expedition_name, emu->expedition_name);
	strcpy(eq->player_name, emu->player_name);
	FINISH_ENCODE();
}

ENCODE(OP_TargetBuffs) {  ENCODE_FORWARD(OP_BuffCreate); }
ENCODE(OP_BuffCreate)
{
	SETUP_VAR_ENCODE(BuffIcon_Struct);

	uint32 sz = 12 + (17 * emu->count);
	__packet->size = sz;
	__packet->pBuffer = new unsigned char[sz];
	memset(__packet->pBuffer, 0, sz);

	uchar *ptr = __packet->pBuffer;
	*((uint32*)ptr) = emu->entity_id;
	ptr += sizeof(uint32);
	ptr += sizeof(uint32);
	*((uint8*)ptr) = 1;
	ptr += sizeof(uchar);
	*((uint16*)ptr) = emu->count;
	ptr += sizeof(uint16);

	for(uint16 i = 0; i < emu->count; ++i)
	{
		uint16 buffslot = emu->entries[i].buff_slot;
		if(emu->entries[i].buff_slot >= 25 && emu->entries[i].buff_slot < 37)
		{
			buffslot += 5;
		}
		else if(emu->entries[i].buff_slot >= 37)
		{
			buffslot += 14;
		}

		*((uint32*)ptr) = buffslot;
		ptr += sizeof(uint32);
		*((uint32*)ptr) = emu->entries[i].spell_id;
		ptr += sizeof(uint32);
		*((uint32*)ptr) = emu->entries[i].tics_remaining;
		ptr += sizeof(uint32);
		ptr += sizeof(uint32);
		ptr += 1;
	}
	FINISH_ENCODE();
	/*
	uint32 write_var32 = 60;
	uint8 write_var8 = 1;
	ss.write((const char*)&emu->entity_id, sizeof(uint32));
	ss.write((const char*)&write_var32, sizeof(uint32));
	ss.write((const char*)&write_var8, sizeof(uint8));
	ss.write((const char*)&emu->count, sizeof(uint16));
	write_var32 = 0;
	write_var8 = 0;
	for(uint16 i = 0; i < emu->count; ++i)
	{
		if(emu->entries[i].buff_slot >= 25 && emu->entries[i].buff_slot < 37)
		{
			emu->entries[i].buff_slot += 5;
		}
		else if(emu->entries[i].buff_slot >= 37)
		{
			emu->entries[i].buff_slot += 14;
		}
		ss.write((const char*)&emu->entries[i].buff_slot, sizeof(uint32));
		ss.write((const char*)&emu->entries[i].spell_id, sizeof(uint32));
		ss.write((const char*)&emu->entries[i].tics_remaining, sizeof(uint32));
		ss.write((const char*)&write_var32, sizeof(uint32));
		ss.write((const char*)&write_var8, sizeof(uint8));
	}
	ss.write((const char*)&write_var8, sizeof(uint8));
	*/
}

//ENCODE(OP_WearChange)
//{
//	ENCODE_LENGTH_EXACT(WearChange_Struct);
//	SETUP_DIRECT_ENCODE(WearChange_Struct, structs::WearChange_Struct);
//	OUT(spawn_id);
//	OUT(material);
//	OUT(unknown06);
//	OUT(elite_material);
//	OUT(color.color);
//	OUT(wear_slot_id);
//	FINISH_ENCODE();
//}

ENCODE(OP_SpawnAppearance)
{
	EQApplicationPacket *in = *p;
	*p = nullptr;

	unsigned char *emu_buffer = in->pBuffer;

	SpawnAppearance_Struct *sas = (SpawnAppearance_Struct *)emu_buffer;

	if(sas->type != AT_Size)
	{
		dest->FastQueuePacket(&in, ack_req);
		return;
	}

	EQApplicationPacket *outapp = new EQApplicationPacket(OP_ChangeSize, sizeof(ChangeSize_Struct));

	ChangeSize_Struct *css = (ChangeSize_Struct *)outapp->pBuffer;

	css->EntityID = sas->spawn_id;
	css->Size = (float)sas->parameter;
	css->Unknown08 = 0;
	css->Unknown12 = 1.0f;

	dest->FastQueuePacket(&outapp, ack_req);

	delete in;
}

ENCODE(OP_DisciplineUpdate)
{
	ENCODE_LENGTH_EXACT(Disciplines_Struct);
	SETUP_DIRECT_ENCODE(Disciplines_Struct, structs::Disciplines_Struct);

	memcpy(&eq->values, &emu->values, sizeof(Disciplines_Struct));

	FINISH_ENCODE();
}

ENCODE(OP_AltCurrencySell)
{
    ENCODE_LENGTH_EXACT(AltCurrencySellItem_Struct);
	SETUP_DIRECT_ENCODE(AltCurrencySellItem_Struct, structs::AltCurrencySellItem_Struct);

    OUT(merchant_entity_id);
    eq->slot_id = TitaniumToUnderfootSlot(emu->slot_id);
    OUT(charges);
    OUT(cost);
    FINISH_ENCODE();
}
//
//ENCODE(OP_AltCurrency)
//{
//    EQApplicationPacket *in = *p;
//	*p = nullptr;
//
//	unsigned char *emu_buffer = in->pBuffer;
//    uint32 opcode = *((uint32*)emu_buffer);
//
//    if(opcode == 8) {
//        AltCurrencyPopulate_Struct *populate = (AltCurrencyPopulate_Struct*)emu_buffer;
//
//        EQApplicationPacket *outapp = new EQApplicationPacket(OP_AltCurrency, sizeof(structs::AltCurrencyPopulate_Struct)
//            + sizeof(structs::AltCurrencyPopulateEntry_Struct) * populate->count);
//        structs::AltCurrencyPopulate_Struct *out_populate = (structs::AltCurrencyPopulate_Struct*)outapp->pBuffer;
//
//        out_populate->opcode = populate->opcode;
//        out_populate->count = populate->count;
//        for(uint32 i = 0; i < populate->count; ++i) {
//            out_populate->entries[i].currency_number = populate->entries[i].currency_number;
//            out_populate->entries[i].currency_number2 = populate->entries[i].currency_number2;
//            out_populate->entries[i].item_id = populate->entries[i].item_id;
//            out_populate->entries[i].item_icon = populate->entries[i].item_icon;
//            out_populate->entries[i].stack_size = populate->entries[i].stack_size;
//            out_populate->entries[i].unknown00 = populate->entries[i].unknown00;
//        }
//
//        dest->FastQueuePacket(&outapp, ack_req);
//    } else {
//        EQApplicationPacket *outapp = new EQApplicationPacket(OP_AltCurrency, sizeof(AltCurrencyUpdate_Struct));
//        memcpy(outapp->pBuffer, emu_buffer, sizeof(AltCurrencyUpdate_Struct));
//        dest->FastQueuePacket(&outapp, ack_req);
//    }
//
//    //dest->FastQueuePacket(&outapp, ack_req);
//    delete in;
//}

//ENCODE(OP_InspectRequest) {
//	ENCODE_LENGTH_EXACT(Inspect_Struct);
//	SETUP_DIRECT_ENCODE(Inspect_Struct, structs::Inspect_Struct);
//	OUT(TargetID);
//	OUT(PlayerID);
//	FINISH_ENCODE();
//}
//
//DECODE(OP_InspectRequest) {
//	DECODE_LENGTH_EXACT(structs::Inspect_Struct);
//	SETUP_DIRECT_DECODE(Inspect_Struct, structs::Inspect_Struct);
//	IN(TargetID);
//	IN(PlayerID);
//	FINISH_DIRECT_DECODE();
//}

DECODE(OP_BazaarSearch)
{
	char *Buffer = (char *)__packet->pBuffer;

	uint8 SubAction = VARSTRUCT_DECODE_TYPE(uint8, Buffer);

	if((SubAction != BazaarInspectItem) || (__packet->size != sizeof(structs::NewBazaarInspect_Struct)))
		return;

	SETUP_DIRECT_DECODE(NewBazaarInspect_Struct, structs::NewBazaarInspect_Struct);
	MEMSET_IN(structs::NewBazaarInspect_Struct);
	IN(Beginning.Action);
	memcpy(emu->Name, eq->Name, sizeof(emu->Name));
	IN(SerialNumber);

	FINISH_DIRECT_DECODE();
}

//DECODE(OP_RaidInvite) {
//	DECODE_LENGTH_EXACT(structs::RaidGeneral_Struct);
//	SETUP_DIRECT_DECODE(RaidGeneral_Struct, structs::RaidGeneral_Struct);
//
//	strn0cpy(emu->leader_name, eq->leader_name, 64);
//	strn0cpy(emu->player_name, eq->player_name, 64);
//	IN(action);
//	IN(parameter);
//
//	FINISH_DIRECT_DECODE();
//}

DECODE(OP_AdventureMerchantSell) {
	DECODE_LENGTH_EXACT(structs::Adventure_Sell_Struct);
	SETUP_DIRECT_DECODE(Adventure_Sell_Struct, structs::Adventure_Sell_Struct);

	IN(npcid);
	emu->slot = UnderfootToTitaniumSlot(eq->slot);
	IN(charges);
	IN(sell_price);

	FINISH_DIRECT_DECODE();
}


//DECODE(OP_ApplyPoison) {
//	DECODE_LENGTH_EXACT(structs::ApplyPoison_Struct);
//	SETUP_DIRECT_DECODE(ApplyPoison_Struct, structs::ApplyPoison_Struct);
//
//	emu->inventorySlot = UnderfootToTitaniumSlot(eq->inventorySlot);
//	IN(success);
//
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_CastSpell) {
//	DECODE_LENGTH_EXACT(structs::CastSpell_Struct);
//	SETUP_DIRECT_DECODE(CastSpell_Struct, structs::CastSpell_Struct);
//
//	if(eq->slot == 13)
//	{
//		emu->slot = 10;
//	}
//	else
//	{
//		IN(slot);
//	}
//	IN(spell_id);
//	emu->inventoryslot = UnderfootToTitaniumSlot(eq->inventoryslot);
//	IN(target_id);
//
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_ItemLinkClick) {
//	DECODE_LENGTH_EXACT(structs::ItemViewRequest_Struct);
//	SETUP_DIRECT_DECODE(ItemViewRequest_Struct, structs::ItemViewRequest_Struct);
//	MEMSET_IN(ItemViewRequest_Struct);
//
//	IN(item_id);
//	int r;
//	for (r = 0; r < 5; r++) {
//		IN(augments[r]);
//	}
//	IN(link_hash);
//	IN(icon);
//
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_ConsiderCorpse) { DECODE_FORWARD(OP_Consider); }
//DECODE(OP_Consider) {
//	DECODE_LENGTH_EXACT(structs::Consider_Struct);
//	SETUP_DIRECT_DECODE(Consider_Struct, structs::Consider_Struct);
//	IN(playerid);
//	IN(targetid);
//	IN(faction);
//	IN(level);
//	//emu->cur_hp = 1;
//	//emu->max_hp = 2;
//	//emu->pvpcon = 0;
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_ShopPlayerBuy)
//{
//	DECODE_LENGTH_EXACT(structs::Merchant_Sell_Struct);
//	SETUP_DIRECT_DECODE(Merchant_Sell_Struct, structs::Merchant_Sell_Struct);
//
//	IN(npcid);
//	IN(playerid);
//	IN(itemslot);
//	IN(quantity);
//	IN(price);
//
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_ClientUpdate) {
//    // for some odd reason, there is an extra byte on the end of this on occasion..
//	DECODE_LENGTH_ATLEAST(structs::PlayerPositionUpdateClient_Struct);
//	SETUP_DIRECT_DECODE(PlayerPositionUpdateClient_Struct, structs::PlayerPositionUpdateClient_Struct);
//	IN(spawn_id);
//	IN(sequence);
//	IN(x_pos);
//	IN(y_pos);
//	IN(z_pos);
//	IN(heading);
//	IN(delta_x);
//	IN(delta_y);
//	IN(delta_z);
//	IN(delta_heading);
//	IN(animation);
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_CharacterCreate) {
//	DECODE_LENGTH_EXACT(structs::CharCreate_Struct);
//	SETUP_DIRECT_DECODE(CharCreate_Struct, structs::CharCreate_Struct);
//	IN(class_);
//	IN(beardcolor);
//	IN(beard);
//	IN(hairstyle);
//	IN(gender);
//	IN(race);
//
//	if(RuleB(World, EnableTutorialButton) && eq->tutorial)
//		emu->start_zone = RuleI(World, TutorialZoneID);
//	else
//		emu->start_zone = eq->start_zone;
//
//	IN(haircolor);
//	IN(deity);
//	IN(STR);
//	IN(STA);
//	IN(AGI);
//	IN(DEX);
//	IN(WIS);
//	IN(INT);
//	IN(CHA);
//	IN(face);
//	IN(eyecolor1);
//	IN(eyecolor2);
//	IN(drakkin_heritage);
//	IN(drakkin_tattoo);
//	IN(drakkin_details);
//
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_WhoAllRequest) {
//	DECODE_LENGTH_EXACT(structs::Who_All_Struct);
//	SETUP_DIRECT_DECODE(Who_All_Struct, structs::Who_All_Struct);
//
//	memcpy(emu->whom, eq->whom, sizeof(emu->whom));
//	IN(wrace);
//	IN(wclass);
//	IN(lvllow);
//	IN(lvlhigh);
//	IN(gmlookup);
//	IN(guildid);
//	IN(type);
//
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_GroupInvite2)
//{
//	//_log(NET__ERROR, "Received incoming OP_GroupInvite2. Forwarding");
//	DECODE_FORWARD(OP_GroupInvite);
//}

//DECODE(OP_GroupInvite) {
//	//EQApplicationPacket *in = __packet;
//	//_log(NET__ERROR, "Received incoming OP_GroupInvite");
//	//_hex(NET__ERROR, in->pBuffer, in->size);
//	DECODE_LENGTH_EXACT(structs::GroupInvite_Struct);
//	SETUP_DIRECT_DECODE(GroupGeneric_Struct, structs::GroupInvite_Struct);
//	memcpy(emu->name1, eq->invitee_name, sizeof(emu->name1));
//	memcpy(emu->name2, eq->inviter_name, sizeof(emu->name2));
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_GroupFollow) {
//	//EQApplicationPacket *in = __packet;
//	//_log(NET__ERROR, "Received incoming OP_GroupFollow");
//	//_hex(NET__ERROR, in->pBuffer, in->size);
//	DECODE_LENGTH_EXACT(structs::GroupFollow_Struct);
//	SETUP_DIRECT_DECODE(GroupGeneric_Struct, structs::GroupFollow_Struct);
//	memcpy(emu->name1, eq->name1, sizeof(emu->name1));
//	memcpy(emu->name2, eq->name2, sizeof(emu->name2));
//	FINISH_DIRECT_DECODE();
//}
//
//DECODE(OP_GroupFollow2) {
//	//EQApplicationPacket *in = __packet;
//	//_log(NET__ERROR, "Received incoming OP_GroupFollow2");
//	//_hex(NET__ERROR, in->pBuffer, in->size);
//	DECODE_LENGTH_EXACT(structs::GroupFollow_Struct);
//	SETUP_DIRECT_DECODE(GroupGeneric_Struct, structs::GroupFollow_Struct);
//	memcpy(emu->name1, eq->name1, sizeof(emu->name1));
//	memcpy(emu->name2, eq->name2, sizeof(emu->name2));
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_GroupDisband) {
//	//EQApplicationPacket *in = __packet;
//	//_log(NET__ERROR, "Received incoming OP_Disband");
//	//_hex(NET__ERROR, in->pBuffer, in->size);
//	DECODE_LENGTH_EXACT(structs::GroupGeneric_Struct);
//	SETUP_DIRECT_DECODE(GroupGeneric_Struct, structs::GroupGeneric_Struct);
//	memcpy(emu->name1, eq->name1, sizeof(emu->name1));
//	memcpy(emu->name2, eq->name2, sizeof(emu->name2));
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_GroupCancelInvite)
//{
//	DECODE_LENGTH_EXACT(structs::GroupCancel_Struct);
//	SETUP_DIRECT_DECODE(GroupCancel_Struct, structs::GroupCancel_Struct);
//	memcpy(emu->name1, eq->name1, sizeof(emu->name1));
//	memcpy(emu->name2, eq->name2, sizeof(emu->name2));
//	IN(toggle);
//	FINISH_DIRECT_DECODE();
//}

DECODE(OP_Buff) {
	DECODE_LENGTH_EXACT(structs::SpellBuffFade_Struct_Underfoot);
	SETUP_DIRECT_DECODE(SpellBuffFade_Struct, structs::SpellBuffFade_Struct_Underfoot);
	IN(entityid);
	IN(slot);
	IN(level);
	IN(effect);
	IN(spellid);
	IN(duration);
	IN(slotid);
	IN(bufffade);
	FINISH_DIRECT_DECODE();
}

//DECODE(OP_ShopPlayerSell) {
//	DECODE_LENGTH_EXACT(structs::Merchant_Purchase_Struct);
//	SETUP_DIRECT_DECODE(Merchant_Purchase_Struct, structs::Merchant_Purchase_Struct);
//
//	IN(npcid);
//	emu->itemslot = UnderfootToTitaniumSlot(eq->itemslot);
//	IN(quantity);
//	IN(price);
//
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_Save) {
//	DECODE_LENGTH_EXACT(structs::Save_Struct);
//	SETUP_DIRECT_DECODE(Save_Struct, structs::Save_Struct);
//	memcpy(emu->unknown00, eq->unknown00, sizeof(emu->unknown00));
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_FindPersonRequest) {
//	DECODE_LENGTH_EXACT(structs::FindPersonRequest_Struct);
//	SETUP_DIRECT_DECODE(FindPersonRequest_Struct, structs::FindPersonRequest_Struct);
//	IN(npc_id);
//	IN(client_pos.x);
//	IN(client_pos.y);
//	IN(client_pos.z);
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_WearChange) {
//	DECODE_LENGTH_EXACT(structs::WearChange_Struct);
//	SETUP_DIRECT_DECODE(WearChange_Struct, structs::WearChange_Struct);
//	IN(spawn_id);
//	IN(material);
//	IN(unknown06);
//	IN(elite_material);
//	IN(color.color);
//	IN(wear_slot_id);
//	emu->hero_forge_model = 0;
//	emu->unknown18		  = 0;
//	FINISH_DIRECT_DECODE();
//}

DECODE(OP_TraderBuy)
{
	DECODE_LENGTH_EXACT(structs::TraderBuy_Struct);
	SETUP_DIRECT_DECODE(TraderBuy_Struct, structs::TraderBuy_Struct);
	MEMSET_IN(TraderBuy_Struct);

	IN(Action);
	IN(Price);
	IN(TraderID);
	memcpy(emu->ItemName, eq->ItemName, sizeof(emu->ItemName));
	IN(ItemID);
	IN(Quantity);

	FINISH_DIRECT_DECODE();
}

//DECODE(OP_LootItem) {
//	DECODE_LENGTH_EXACT(structs::LootingItem_Struct);
//	SETUP_DIRECT_DECODE(LootingItem_Struct, structs::LootingItem_Struct);
//	IN(lootee);
//	IN(looter);
//	emu->slot_id = eq->slot_id - 1;
//	IN(auto_loot);
//
//	FINISH_DIRECT_DECODE();
//}

DECODE(OP_TributeItem) {
	DECODE_LENGTH_EXACT(structs::TributeItem_Struct);
	SETUP_DIRECT_DECODE(TributeItem_Struct, structs::TributeItem_Struct);

	emu->slot = UnderfootToTitaniumSlot(eq->slot);
	IN(quantity);
	IN(tribute_master_id);
	IN(tribute_points);

	FINISH_DIRECT_DECODE();
}

//DECODE(OP_ReadBook) {
//	DECODE_LENGTH_EXACT(structs::BookRequest_Struct);
//	SETUP_DIRECT_DECODE(BookRequest_Struct, structs::BookRequest_Struct);
//
//	IN(type);
//	emu->invslot = UnderfootToTitaniumSlot(eq->invslot);
//	emu->window = (uint8) eq->window;
//	strn0cpy(emu->txtfile, eq->txtfile, sizeof(emu->txtfile));
//
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_TradeSkillCombine) {
//	DECODE_LENGTH_EXACT(structs::NewCombine_Struct);
//	SETUP_DIRECT_DECODE(NewCombine_Struct, structs::NewCombine_Struct);
//
//	emu->container_slot = UnderfootToTitaniumSlot(eq->container_slot);
//
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_AugmentItem) {
//	DECODE_LENGTH_EXACT(structs::AugmentItem_Struct);
//	SETUP_DIRECT_DECODE(AugmentItem_Struct, structs::AugmentItem_Struct);
//
//	emu->container_slot = UnderfootToTitaniumSlot(eq->container_slot);
//	emu->augment_slot = eq->augment_slot;
//
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_AugmentInfo) {
//	DECODE_LENGTH_EXACT(structs::AugmentInfo_Struct);
//	SETUP_DIRECT_DECODE(AugmentInfo_Struct, structs::AugmentInfo_Struct);
//
//	IN(itemid);
//	IN(window);
//
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_FaceChange) {
//
//	DECODE_LENGTH_EXACT(structs::FaceChange_Struct);
//	SETUP_DIRECT_DECODE(FaceChange_Struct, structs::FaceChange_Struct);
//	IN(haircolor);
//	IN(beardcolor);
//	IN(eyecolor1);
//	IN(eyecolor2);
//	IN(hairstyle);
//	IN(beard);
//	IN(face);
//	IN(drakkin_heritage);
//	IN(drakkin_tattoo);
//	IN(drakkin_details);
//
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_LoadSpellSet)
//{
//	DECODE_LENGTH_EXACT(structs::LoadSpellSet_Struct);
//	SETUP_DIRECT_DECODE(LoadSpellSet_Struct, structs::LoadSpellSet_Struct);
//
//	for(unsigned int i = 0; i < MAX_PP_MEMSPELL; ++i)
//		if(eq->spell[i]==0)
//			emu->spell[i] = 0xFFFFFFFF;
//		else
//			emu->spell[i] = eq->spell[i];
//
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_EnvDamage) {
//	DECODE_LENGTH_EXACT(structs::EnvDamage2_Struct);
//	SETUP_DIRECT_DECODE(EnvDamage2_Struct, structs::EnvDamage2_Struct);
//	IN(id);
//	IN(damage);
//	IN(dmgtype);
//	emu->constant = 0xFFFF;
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_PetCommands)
//{
//	DECODE_LENGTH_EXACT(structs::PetCommand_Struct);
//	SETUP_DIRECT_DECODE(PetCommand_Struct, structs::PetCommand_Struct);
//
//	switch(eq->command)
//	{
//		case 0x00:
//			emu->command = 0x04;	// Health
//			break;
//		case 0x01:
//			emu->command = 0x10;	// Leader
//			break;
//		case 0x02:
//			emu->command = 0x07;	// Attack
//			break;
//		case 0x04:
//			emu->command = 0x08;	// Follow
//			break;
//		case 0x05:
//			emu->command = 0x05;	// Guard
//			break;
//		case 0x06:
//			emu->command = 0x09;	// Sit. Needs work. This appears to be a toggle between Sit/Stand now.
//			break;
//		case 0x0c:
//			emu->command = 0x0b;	// Taunt
//			break;
//		case 0x0f:
//			emu->command = 0x0c;	// Hold
//			break;
//		case 0x10:
//			emu->command = 0x1b;	// Hold on
//			break;
//		case 0x11:
//			emu->command = 0x1c;	// Hold off
//			break;
//		case 0x1c:
//			emu->command = 0x01;	// Back
//			break;
//		case 0x1d:
//			emu->command = 0x02;	// Leave/Go Away
//			break;
//		case 0x15:
//			emu->command = 0x12;	// No Cast - /command
//			break;
//		case 0x16:
//			emu->command = 0x12;	// No Cast - Pet Window
//			break;
//		case 0x18:
//			emu->command = 0x13;	// Focus - Pet Window
//			break;
//		default:
//			emu->command = eq->command;
//	}
//	OUT(unknown);
//
//	FINISH_DIRECT_DECODE();
//}

//DECODE(OP_ChannelMessage)
//{
//	unsigned char *__eq_buffer = __packet->pBuffer;
//
//	char *InBuffer = (char *)__eq_buffer;
//
//	char Sender[64];
//	char Target[64];
//
//	VARSTRUCT_DECODE_STRING(Sender, InBuffer);
//	VARSTRUCT_DECODE_STRING(Target, InBuffer);
//
//	InBuffer += 4;
//
//	uint32 Language = VARSTRUCT_DECODE_TYPE(uint32, InBuffer);
//	uint32 Channel = VARSTRUCT_DECODE_TYPE(uint32, InBuffer);
//
//	InBuffer += 5;
//
//	uint32 Skill = VARSTRUCT_DECODE_TYPE(uint32, InBuffer);
//
//	__packet->size = sizeof(ChannelMessage_Struct) + strlen(InBuffer) + 1;
//	__packet->pBuffer = new unsigned char[__packet->size];
//	ChannelMessage_Struct *emu = (ChannelMessage_Struct *) __packet->pBuffer;
//
//	strn0cpy(emu->targetname, Target, sizeof(emu->targetname));
//	strn0cpy(emu->sender, Target, sizeof(emu->sender));
//	emu->language = Language;
//	emu->chan_num = Channel;
//	emu->skill_in_language = Skill;
//	strcpy(emu->message, InBuffer);
//
//	delete [] __eq_buffer;
//}

//DECODE(OP_BuffRemoveRequest)
//{
//	// This is to cater for the fact that short buff box buffs start at 30 as opposed to 25 in prior clients.
//	//
//	DECODE_LENGTH_EXACT(structs::BuffRemoveRequest_Struct);
//	SETUP_DIRECT_DECODE(BuffRemoveRequest_Struct, structs::BuffRemoveRequest_Struct);
//
//	emu->SlotID = (eq->SlotID < 30 ) ? eq->SlotID : (eq->SlotID - 5);
//
//	IN(EntityID);
//
//	FINISH_DIRECT_DECODE();
//}

uint32 NextItemInstSerialNumber = 1;
uint32 MaxInstances = 2000000000;

static inline int32 GetNextItemInstSerialNumber() {

	if(NextItemInstSerialNumber >= MaxInstances)
		NextItemInstSerialNumber = 1;
	else
		NextItemInstSerialNumber++;

	return NextItemInstSerialNumber;
}

DECODE(OP_AltCurrencySellSelection)
{
    DECODE_LENGTH_EXACT(structs::AltCurrencySelectItem_Struct);
	SETUP_DIRECT_DECODE(AltCurrencySelectItem_Struct, structs::AltCurrencySelectItem_Struct);
    IN(merchant_entity_id);
    emu->slot_id = UnderfootToTitaniumSlot(eq->slot_id);
    FINISH_DIRECT_DECODE();
}

DECODE(OP_AltCurrencySell)
{
    DECODE_LENGTH_EXACT(structs::AltCurrencySellItem_Struct);
	SETUP_DIRECT_DECODE(AltCurrencySellItem_Struct, structs::AltCurrencySellItem_Struct);
    IN(merchant_entity_id);
    emu->slot_id = UnderfootToTitaniumSlot(eq->slot_id);
    IN(charges);
    IN(cost);
    FINISH_DIRECT_DECODE();
}

} //end namespace Underfoot
