#include "Payload.h"
#include "Data.h"
#include "Zone.h"
#include "Guild.h"

#pragma pack(1)

struct Equipment {
	u32 mMaterial = 0;
	u32 mUnknown = 0; // Elite material maybe?
	u32 itemid = 0;
	u32 mColour = 0;
};
struct CharacterSelectionHeader {
	u32 mCharacterCount = 0;
	u32 mAdditionalSlots = 0;
};
struct CharacterSelectionPartA {
	u8 mLevel = 0;
	u8 mHairStyle = 0;
	u8 mGender = 0;
};

struct CharacterSelectionPartB {
	u8 mBeardStyle = 0;
	u8 mHairColour = 0;
	u8 mFaceStyle = 0;
	Equipment mEquipment[9];
	u32 mPrimary = 0;
	u32 mSecondary = 0;
	u8 mUnknown0 = 0; // Was 0xFF
	u32 mDeity = 0;
	u16 mZoneID = 0;
	u16 mInstanceID = 0;
	u8 mReturnHome = 0; // 0 = Unavailable, 1 = Available.
	u8 mUnknown1 = 0; // Was 0xFF
	u32 mRace = 0;
	u8 mEnterTutorial = 0; // 0 = Unavailable, 1 = Available.
	u8 mClass = 0;
	u8 mLeftEyeColour = 0;
	u8 mBeardColour = 0;
	u8 mRightEyeColour = 0;
	u32 mDrakkinHeritage = 0;
	u32 mDrakkinTattoo = 0;
	u32 mDrakkinDetails = 0;
	u8 mUnknown2 = 0;
};

#pragma pack()

EQApplicationPacket* Payload::makeCharacterSelection(Data::Account* pData) {
	if (!pData) return nullptr;

	const u32 numAccountCharacters = pData->mCharacterData.size();
	u32 size = 0;

	// Add name lengths.
	for (auto i : pData->mCharacterData) {
		size += i->mName.length() + 1;
	}

	size += sizeof(CharacterSelectionHeader);
	size += sizeof(CharacterSelectionPartA) * numAccountCharacters;
	size += sizeof(CharacterSelectionPartB) * numAccountCharacters;

	unsigned char * data = new unsigned char[size];
	Utility::MemoryWriter writer(data, size);

	// Header.
	CharacterSelectionHeader header;
	header.mCharacterCount = numAccountCharacters;
	header.mAdditionalSlots = 0; // Total Slots = 8 + additional slots.
	writer.write<CharacterSelectionHeader>(header);

	for (auto i : pData->mCharacterData) {
		// Part A.
		CharacterSelectionPartA pA;
		pA.mLevel = i->mLevel;
		pA.mHairStyle = i->mHairStyle;
		pA.mGender = i->mGender;
		writer.write<CharacterSelectionPartA>(pA);

		// Name.
		writer.writeString(i->mName);

		// Part B.
		CharacterSelectionPartB pB;
		pB.mBeardStyle = i->mBeardStyle;
		pB.mHairColour = i->mHairColour;
		pB.mFaceStyle = i->mFaceStyle;

		for (auto j = 0; j < 9; j++) {
			pB.mEquipment[j].mMaterial = i->mEquipment[j].mMaterial;
			pB.mEquipment[j].mColour = i->mEquipment[j].mColour;
		}

		pB.mPrimary = i->mPrimary;
		pB.mSecondary = i->mSecondary;
		pB.mUnknown0 = 0;
		pB.mDeity = i->mDeity;
		pB.mZoneID = i->mZoneID;
		pB.mInstanceID = 0;
		pB.mReturnHome = i->mCanReturnHome ? 1 : 0;
		pB.mUnknown1 = 0;
		pB.mRace = i->mRace;
		pB.mEnterTutorial = i->mCanEnterTutorial ? 1 : 0;
		pB.mClass = i->mClass;
		pB.mLeftEyeColour = i->mEyeColourLeft;
		pB.mBeardColour = i->mBeardColour;
		pB.mRightEyeColour = i->mEyeColourRight;
		pB.mDrakkinHeritage = i->mDrakkinHeritage;
		pB.mDrakkinTattoo = i->mDrakkinTattoo;
		pB.mDrakkinDetails = i->mDrakkinDetails;
		pB.mUnknown2 = 0;
		writer.write<CharacterSelectionPartB>(pB);
	}

	auto packet = new EQApplicationPacket(OP_SendCharInfo, data, size);
	return packet;
}

#pragma pack(1)
namespace Payload {
	struct ZonePointData {
		u32 mID = 0;
		float mY = 0.0f;
		float mX = 0.0f;
		float mZ = 0.0f;
		float mHeading = 0.0f;
		u16 mZoneID = 0;
		u16 mInstanceID = 0;
		u32 mUnknown = 0;
	};

}
#pragma pack()

EQApplicationPacket* Payload::makeZonePoints(const std::list<ZonePoint*>& pZonePoints) {
	const u32 numZonePoints = pZonePoints.size();
	u32 size = 0;

	size += sizeof(u32); // Count.
	size += sizeof(ZonePointData) * numZonePoints;
	
	unsigned char * data = new unsigned char[size];
	Utility::MemoryWriter writer(data, size);

	// Count.
	writer.write<u32>(numZonePoints);

	for (auto i : pZonePoints) {
		ZonePointData p;
		p.mID = i->mID;
		p.mY = i->mPosition.y;
		p.mX = i->mPosition.x;
		p.mZ = i->mPosition.z;
		p.mHeading = i->mHeading;
		p.mZoneID = i->mDestinationZoneID;
		p.mInstanceID = i->mDestinationInstanceID;
		p.mUnknown = 0;

		writer.write<ZonePointData>(p);
	}

	return new EQApplicationPacket(OP_SendZonepoints, data, size);
}

EQApplicationPacket* Payload::makeGuildNameList(const std::list<::Guild*>& pGuilds) {
	const u32 numGuilds = pGuilds.size();

	// Calculate size.
	u32 size = 0;
	size += 64; // Unknown 64 bytes at the start.
	size += 1; // Unknown single byte at the end.
	size += sizeof(u32); // Count.

	// Add name/ID lengths.
	for (auto i : pGuilds) {
		size += i->getName().length() + 1; // Name.
		size += sizeof(u32); // ID.
	}

	auto data = new unsigned char[size];
	Utility::MemoryWriter writer(data, size);

	// Unknown.
	writer._memset(0, 64);
	
	// Count.
	writer.write<u32>(numGuilds);

	// Guilds.
	for (auto i : pGuilds) {
		writer.write<u32>(i->getID());
		writer.writeString(i->getName());
	}

	// Unknown.
	writer.write<u8>(0);

	return new EQApplicationPacket(OP_GuildsList, data, size);
}

EQApplicationPacket* Payload::makeGuildMemberList(const std::list<GuildMember*>& pMembers) {
	static const auto fixedSize = 40; // 40 bytes per member.
	const u32 numMembers = pMembers.size();
	
	// Calculate size.
	u32 size = 0;
	size += 1; // Empty Character name.
	size += sizeof(u32); // Count.
	size += fixedSize * numMembers;

	// Add name lengths.
	for (auto i : pMembers) {
		size += i->getName().length() + 1;
	}

	// Add public note lengths.
	for (auto i : pMembers) {
		size += i->getPublicNote().length() + 1;
	}

	unsigned char * data = new unsigned char[size];
	Utility::MemoryWriter writer(data, size);

	// Count.
	writer.write<u8>(0); // Empty Character name.
	writer.write<u32>(htonl(numMembers));

	// Members.
	for (auto i : pMembers) {
		writer.writeString(i->getName());
		writer.write<u32>(htonl(i->getLevel()));
		writer.write<u32>(htonl(i->getFlags()));
		writer.write<u32>(htonl(i->getClass()));
		writer.write<u32>(htonl(i->getRank()));
		writer.write<u32>(htonl(i->getLastSeen()));
		writer.write<u32>(htonl(i->isTributeEnabled() ? 1 : 0));
		writer.write<u32>(htonl(i->getTotalTribute()));
		writer.write<u32>(htonl(i->getLastTribute()));
		writer.write<u32>(htonl(1)); // Unknown.
		writer.writeString(i->getPublicNote());
		writer.write<u16>(htons(i->getInstanceID()));
		writer.write<u16>(htons(i->getZoneID()));	
	}

	// NOTE: Sending this is currently wiping out the guild name at the top of the 'Guild Management' window.
	return new EQApplicationPacket(OP_GuildMemberList, data, size);
}