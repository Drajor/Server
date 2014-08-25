#include "Character.h"
#include "Data.h"
#include "GuildManager.h"
#include "Zone.h"
#include "Utility.h"
#include "LogSystem.h"
#include "../common/eq_packet_structs.h"
#include "../common/extprofile.h"
#include "ZoneClientConnection.h"

static const int AUTO_SAVE_FREQUENCY = 10000;

Character::Character(CharacterData* pCharacterData) :mData(pCharacterData) {
	EXPECTED(mData);
	mName = pCharacterData->mName; // NOTE: This is required for ID before initialise has been called.
}

Character::~Character() {
}

void Character::update() {

	if (mGM) {
		if (mSuperGMPower.Check()) {
			mZone->notifyCharacterLevelIncrease(this);
		}
	}

	if (mAutoSave.Check()) {
		_updateForSave();
		mZone->requestSave(this);
	}
}

bool Character::initialise() {
	EXPECTED_BOOL(mInitialised == false);

	mName = mData->mName;
	mLastName = mData->mLastName;
	mTitle = mData->mTitle;
	mSuffix = mData->mSuffix;
	mRace = mData->mRace;
	mOriginalRace = mRace;
	mGM = true; // mData->mGM; (testing).
	mClass = mData->mClass;
	mGender = mData->mGender;
	mLevel = mData->mLevel;
	mStatus = 255; // mData->mStatus; (testing)

	mX = mData->mX;
	mY = mData->mY;
	mZ = mData->mZ;
	mHeading = mData->mHeading;

	mBeardStyle = mData->mBeardStyle;
	mBeardColour = mData->mBeardColour;
	mHairStyle = mData->mHairStyle;
	mHairColour = mData->mHairColour;
	mFaceStyle = mData->mFace;
	mDrakkinHeritage = mData->mDrakkinHeritage;
	mDrakkinTattoo = mData->mDrakkinTattoo;
	mDrakkinDetails = mData->mDrakkinDetails;

	mSize = Character::getDefaultSize(mRace);

	mExperience = mData->mExperience;

	mPlatinum = mData->mPlatinumCharacter;
	mGold = mData->mGoldCharacter;
	mSilver = mData->mSilverCharacter;
	mCopper = mData->mCopperCharacter;

	mBaseStrength = mData->mStrength;
	mBaseStamina = mData->mStamina;
	mBaseCharisma = mData->mCharisma;
	mBaseDexterity = mData->mDexterity;
	mBaseIntelligence = mData->mIntelligence;
	mBaseAgility = mData->mAgility;
	mBaseWisdom = mData->mWisdom;

	mAutoSave.Start(AUTO_SAVE_FREQUENCY);

	if (mData->mGuildID != NO_GUILD) {
		GuildManager::getInstance().onConnect(this, mData->mGuildID);
	}

	mInitialised = true;
	return true;
}

bool Character::onZoneIn() {
	setZoning(false);
	_processMessageQueue();
	return true;
}

bool Character::onZoneOut() {
	setZoning(true);
	setZone(nullptr);
	setSpawnID(0);
	setConnection(nullptr);

	return true;
}

void Character::setStanding(bool pStanding) {
	mStanding = pStanding;
	if (mStanding) {
		mCampTimer.Disable();
		_setAppearance(Standing);
	}
	else {
		_setAppearance(Sitting);
	}
		
}

void Character::startCamp() {
	mCampTimer.Start(29000, true);
}

void Character::message(MessageType pType, String pMessage)
{
	mConnection->sendMessage(pType, pMessage);
}

void Character::setPosition(float pX, float pY, float pZ, float pHeading) {
	mX = pX;
	mY = pY;
	mZ = pZ;
	mHeading = pHeading;
}

void Character::setPosition(Vector3& pPosition) {
	mX = pPosition.x;
	mY = pPosition.y;
	mZ = pPosition.z;
}

void Character::setPosition(Vector4& pPosition) {
	mX = pPosition.x;
	mY = pPosition.y;
	mZ = pPosition.z;
	mHeading = pPosition.h;
}

void Character::setHeading(float pHeading) {
	mHeading = pHeading;
}

void Character::healPercentage(int pPercent) {
	mConnection->sendHPUpdate();
}

void Character::damage(uint32 pAmount) {
	mCurrentHP -= pAmount;
	mConnection->sendHPUpdate();
}

void Character::setPositionDeltas(float pDeltaX, float pDeltaY, float pDeltaZ, int32 pDeltaHeading)
{
	mDeltaX = pDeltaX;
	mDeltaY = pDeltaY;
	mDeltaZ = pDeltaZ;
	mDeltaHeading = pDeltaHeading;
}

void Character::doAnimation(uint8 pAnimationID) {
	mZone->notifyCharacterAnimation(this, 10, pAnimationID, true);
}

void Character::addExperience(uint32 pExperience) {
	// Handle special case where Character is max level / experience.
	if (mLevel == Character::getMaxCharacterLevel()) {
		// Character is already at max exp - 1
		if (mExperience == getExperienceForNextLevel() - 1) {
			message(MessageType::LightBlue, "You can no longer gain experience.");
			return;
		}
		// This experience will take the Character over the limit.
		else if (mExperience + pExperience > getExperienceForNextLevel() - 1) {
			// Modify pExeperience so that pExperience + mExperience = getExperienceForNextLevel - 1 (Capped).
			pExperience = (getExperienceForNextLevel() - 1 - mExperience);
		}
	}


	mExperience += pExperience;
	mConnection->sendExperienceGain();
	_checkForLevelIncrease();
	// Update user experience bar.
	mConnection->sendExperienceUpdate();
}

void Character::removeExperience(uint32 pExperience) {
	// No loss of level at this stage.
	if (mExperience == 0) return;

	// Prevent experience value wrapping.
	if (pExperience > mExperience) {
		pExperience = mExperience;
	}

	mExperience -= pExperience;

	// Send user a message.
	mConnection->sendExperienceLoss();
	// Update user experience bar.
	mConnection->sendExperienceUpdate();
}

void Character::_checkForLevelIncrease() {
	while (mExperience >= getExperienceForNextLevel()) {
		mExperience -= getExperienceForNextLevel();
		setLevel(mLevel + 1);

		if (hasGuild())
			GuildManager::getInstance().onLevelChange(this);
	}
}

void Character::setLevel(uint8 pLevel) {
	// Ensure not going above maximum level.
	if (pLevel > Character::getMaxCharacterLevel()){
		pLevel = Character::getMaxCharacterLevel();
	}
	
	// Increasing.
	if (pLevel > mLevel) {
		mLevel = pLevel;
		// Notify user.
		mConnection->sendLevelUpdate();
		mConnection->sendLevelGain();
		// Notify zone.
		mZone->notifyCharacterLevelIncrease(this);
	}
	else if (pLevel < mLevel) {
		mExperience = 0; // to be safe.
		mLevel = pLevel;
		// Notify user.
		mConnection->sendLevelUpdate();
		mConnection->sendLevelLost();
		// Notify zone.
		mZone->notifyCharacterLevelDecrease(this);
	}
	
}

uint32 Character::getExperienceRatio() {
	// Protect against division by zero.
	uint32 next = getExperienceForNextLevel();
	if (next == 0) {
		Log::error("[Character] Prevented division by zero in getExperienceRatio");
		return 0;
	}

	return 330.0f * (mExperience / static_cast<float>(next));
}

uint32 Character::getExperienceForLevel(uint8 pLevel)
{
	return (pLevel * pLevel) * 20;
}

float Character::getDefaultSize(uint32 pRace) {
	switch (pRace) {
	case Ogre:
		return 9;
	case Troll:
		return 8;
	case Vahshir:
	case Barbarian:
		return 7;
	case Human:
	case HighElf:
	case Erudite:
	case Iksar:
	case Drakkin:
		return 6;
	case HalfElf:
		return 5.5;
	case WoodElf:
	case DarkElf:
	case Froglok:
		return 5;
	case Dwarf:
		return 4;
	case Halfling:
		return 3.5;
	case Gnome:
		return 3;
	default:
		return 0;
	}
}

void Character::_updateForSave() {
	EXPECTED(mData);
	EXPECTED(mZone);

	mData->mGM = mGM;
	mData->mName = mName;
	mData->mLastName = mLastName;
	mData->mTitle = mTitle;
	mData->mSuffix = mSuffix;

	mData->mLevel = mLevel;
	mData->mExperience = mExperience;

	mData->mRace = mOriginalRace;
	mData->mClass = mClass;

	mData->mBeardStyle = mBeardStyle;
	mData->mBeardColour = mBeardColour;
	mData->mHairStyle = mHairStyle;
	mData->mHairColour = mHairColour;
	mData->mEyeColourLeft = mLeftEyeColour;
	mData->mEyeColourRight = mRightEyeColour;
	mData->mFace = mFaceStyle;
	mData->mDrakkinHeritage = mDrakkinHeritage;
	mData->mDrakkinTattoo = mDrakkinTattoo;
	mData->mDrakkinDetails = mDrakkinDetails;

	mData->mGender = mGender;

	mData->mPlatinumCharacter = mPlatinum;
	mData->mGoldCharacter = mGold;
	mData->mSilverCharacter = mSilver;
	mData->mCopperCharacter = mCopper;

	// TODO: Bank / Cursor currency

	mData->mZoneID = mZone->getID();
	mData->mInstanceID = mZone->getInstanceID();
	mData->mX = mX;
	mData->mY = mY;
	mData->mZ = mZ;
	mData->mHeading = mHeading;

	mData->mStrength = mBaseStrength;
	mData->mStamina = mBaseStamina;
	mData->mCharisma = mBaseCharisma;
	mData->mDexterity = mBaseDexterity;
	mData->mIntelligence = mBaseIntelligence;
	mData->mAgility = mBaseAgility;
	mData->mWisdom = mBaseWisdom;

	mData->mGuildID = mGuildID;
	mData->mGuildRank = mGuildRank;
}

uint32 Character::getBaseStatistic(Statistic pStatistic) {
	switch (pStatistic)
	{
	case Statistic::Strength:
		return mBaseStrength;
		break;
	case Statistic::Stamina:
		return mBaseStamina;
		break;
	case Statistic::Charisma:
		return mBaseCharisma;
		break;
	case Statistic::Dexterity:
		return mBaseDexterity;
		break;
	case Statistic::Intelligence:
		return mBaseIntelligence;
		break;
	case Statistic::Agility:
		return mBaseAgility;
		break;
	case Statistic::Wisdom:
		return mBaseWisdom;
		break;
	default:
		Log::error("[Character] Unknown Statistic in getBaseStatistic.");
		break;
	}

	return 0;
}

void Character::setBaseStatistic(Statistic pStatistic, uint32 pValue) {
	switch (pStatistic) {
	case Statistic::Strength:
		mBaseStrength = pValue;
		break;
	case Statistic::Stamina:
		mBaseStamina = pValue;
		break;
	case Statistic::Charisma:
		mBaseCharisma = pValue;
		break;
	case Statistic::Dexterity:
		mBaseDexterity = pValue;
		break;
	case Statistic::Intelligence:
		mBaseIntelligence = pValue;
		break;
	case Statistic::Agility:
		mBaseAgility = pValue;
		break;
	case Statistic::Wisdom:
		mBaseWisdom = pValue;
		break;
	default:
		Log::error("[Character] Unknown Statistic in setBaseStatistic.");
		break;
	}
}

uint32 Character::getStatistic(Statistic pStatistic) {
	switch (pStatistic)
	{
	case Statistic::Strength:
		return getStrength();
	case Statistic::Stamina:
		return getStamina();
	case Statistic::Charisma:
		return getCharisma();
	case Statistic::Dexterity:
		return getDexterity();
	case Statistic::Intelligence:
		return getIntelligence();
	case Statistic::Agility:
		return getAgility();
	case Statistic::Wisdom:
		return getWisdom();
	default:
		Log::error("[Character] Unknown Statistic in getStatistic.");
		break;
	}

	return 0;
}

void Character::_processMessageQueue() {
	for (auto i : mMessageQueue)
		mConnection->sendChannelMessage(i.mChannelID, i.mSenderName, i.mMessage);
	mMessageQueue.clear();
}

void Character::addQueuedMessage(ChannelID pChannel, const String& pSenderName, const String& pMessage) {
	mMessageQueue.push_back({ pChannel, pSenderName, pMessage });
}