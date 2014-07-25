#include "Character.h"
#include "Zone.h"
#include "Utility.h"
#include "LogSystem.h"
#include "../common/eq_packet_structs.h"
#include "../common/extprofile.h"
#include "ZoneClientConnection.h"

Character::Character(uint32 pCharacterID) :
mCharacterID(pCharacterID),
mProfile(0),
mExtendedProfile(0),
mName("soandso"),
mLastName(""),
mRace(0),
mRunSpeed(0.7f),
mWalkSpeed(0.35f),
mClass(0),
mGender(0),
mLevel(1),
mDeity(394), // Agnostic
mSize(5.0f),
mStanding(true),
mAFK(false),
mLoggedOut(false),
mTGB(false),
mStatus(0),
mZone(0),
mCurrentHP(100),
mMaximumHP(100),
mCurrentMana(100),
mMaximumMana(100),
mCurrentEndurance(100),
mMaximumEndurance(100),
mX(0),
mY(0),
mZ(0),
mHeading(0),
mDeltaX(0),
mDeltaY(0),
mDeltaZ(0),
mDeltaHeading(0),
mAnimation(0),
mAppearance(SpawnAppearanceAnimation::Standing),
mGM(0),
mGuildID(0xFFFFFFFF),
mGuildRank(0xFF),
mExperience(0)
{ }
Character::~Character() {
	delete mProfile;
	delete mExtendedProfile;
}

void Character::update() {

	if (mGM) {
		if (mSuperGMPower.Check()) {
			mZone->notifyCharacterLevelIncrease(this);
		}
	}
}

bool Character::initialise(PlayerProfile_Struct* pProfile, ExtendedProfile_Struct* pExtendedProfile) {
	mProfile = pProfile;
	mExtendedProfile = pExtendedProfile;

	mName = mProfile->name;
	mRace = mProfile->race;
	mClass = mProfile->class_;
	mGender = mProfile->gender;
	mLevel = mProfile->level;
	mStatus = 255;
	mGM = mProfile->gm;
	mGM = true; // testing!

	mX = mProfile->x;
	mY = mProfile->y;
	mZ = mProfile->z;
	mHeading = mProfile->heading;
	mDeltaX = 0;
	mDeltaY = 0;
	mDeltaZ = 0;
	mDeltaHeading = 0;

	mExperience = mProfile->exp;

	mSuperGMPower.Start(2000);

	return true;
}

bool Character::onZoneIn() {
	return true;
}

bool Character::onZoneOut() {
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

void Character::setAFK(bool pAFK) {
	mAFK = pAFK;
}

bool Character::getAFK() {
	return mAFK;
}

void Character::setShowHelm(bool pShowHelm) {
	mProfile->showhelm = pShowHelm ? 1 : 0;
}
bool Character::getShowHelm() {
	return mProfile->showhelm == 1;
}

void Character::message(uint32 pType, std::string pMessage)
{
	mConnection->sendMessage(pType, pMessage);
}

void Character::setPosition(float pX, float pY, float pZ, float pHeading) {
	mX = pX;
	mY = pY;
	mZ = pZ;
	mHeading = pHeading;
	_updateProfilePosition();
}

void Character::setPosition(Vector3& pPosition) {
	mX = pPosition.x;
	mY = pPosition.y;
	mZ = pPosition.z;
	_updateProfilePosition();
}

void Character::setPosition(Vector4& pPosition) {
	mX = pPosition.x;
	mY = pPosition.y;
	mZ = pPosition.z;
	mHeading = pPosition.h;
	_updateProfilePosition();
}

void Character::setHeading(float pHeading) {
	mHeading = pHeading;
	_updateProfilePosition();
}

void Character::_updateProfilePosition() {
	mProfile->x = mX;
	mProfile->y = mY;
	mProfile->z = mZ;
	mProfile->heading = mHeading;
}

void Character::healPercentage(int pPercent) {
	mConnection->sendHPUpdate();
}

void Character::damage(uint32 pAmount) {
	mCurrentHP -= pAmount;
	mConnection->sendHPUpdate();
}

void Character::setAnonymous(uint8 pAnonymous) {
	mProfile->mAnonymous = pAnonymous;
}

uint8 Character::getAnonymous() {
	return mProfile->mAnonymous;
}


void Character::setPositionDeltas(float pDeltaX, float pDeltaY, float pDeltaZ, int32 pDeltaHeading)
{
	mDeltaX = pDeltaX;
	mDeltaY = pDeltaY;
	mDeltaZ = pDeltaZ;
	mDeltaHeading = pDeltaHeading;
}

uint8 Character::getGM() {
	return mGM ? 1 : 0;
}

void Character::setGM(bool pGM) {
	mGM = pGM;
	mProfile->gm = getGM();
}


void Character::doAnimation(uint8 pAnimationID) {
	mZone->notifyCharacterAnimation(this, 10, pAnimationID, true);
}

void Character::addExperience(uint32 pExperience) {
	mExperience += pExperience;
	mConnection->sendExperienceGain();
	_checkForLevelIncrease();
	mProfile->exp = mExperience;
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
	mProfile->exp = mExperience;

	// Send user a message.
	mConnection->sendExperienceLoss();
	// Update user experience bar.
	mConnection->sendExperienceUpdate();
}

void Character::_checkForLevelIncrease() {
	while (mExperience >= getExperienceForNextLevel()) {
		mExperience -= getExperienceForNextLevel();
		setLevel(mLevel + 1);
		mConnection->sendLevelGain();
		// Notify zone.
		mZone->notifyCharacterLevelIncrease(this);
	}
}

void Character::setLevel(uint8 pLevel) {
	mLevel = pLevel;
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