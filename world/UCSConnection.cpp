#include "UCSConnection.h"
#include "../common/debug.h"
#include "../common/md5.h"
#include "../common/packet_dump.h"
#include "../common/EmuTCPConnection.h"
#include "../common/servertalk.h"

UCSConnection::UCSConnection() : mTCPConnection(0), mAuthenticated(false) { }

void UCSConnection::setConnection(EmuTCPConnection* pConnection) {
	if(mTCPConnection) {
		_log(UCS__ERROR, "Incoming UCS Connection while we were already connected to a UCS.");
		mTCPConnection->Disconnect();
	}

	mTCPConnection = pConnection;
	mAuthenticated = false;
}

bool UCSConnection::update() {
	if (!mTCPConnection || !mTCPConnection->Connected())
		return false;

	ServerPacket *pack = 0;

	while((pack = mTCPConnection->PopPacket()))
	{
		if (!mAuthenticated)
		{
			std::string SharedKey = "jkads89adsjkhadsjkhads8932jkhadsjhadsjk3289"; // TODO: Hardcoded for now.
			if (pack->opcode == ServerOP_ZAAuth && pack->size == 16)
			{
				uint8 tmppass[16];

				MD5::Generate((const uchar*)SharedKey.c_str(), SharedKey.length(), tmppass);

				if (memcmp(pack->pBuffer, tmppass, 16) == 0)
					mAuthenticated = true;
				else
				{
					struct in_addr in;
					in.s_addr = getIP();
					_log(UCS__ERROR, "UCS authorization failed.");
					ServerPacket* pack = new ServerPacket(ServerOP_ZAAuthFailed);
					sendPacket(pack);
					delete pack;
					disconnect();
					return false;
				}
			}
			else
			{
				struct in_addr in;
				in.s_addr = getIP();
				_log(UCS__ERROR, "UCS authorization failed.");
				ServerPacket* pack = new ServerPacket(ServerOP_ZAAuthFailed);
				sendPacket(pack);
				delete pack;
				disconnect();
				return false;
			}
			delete pack;
			continue;
		}
		switch(pack->opcode)
		{
			case 0:
				break;

			case ServerOP_KeepAlive:
			{
				// ignore this
				break;
			}
			case ServerOP_ZAAuth:
			{
				_log(UCS__ERROR, "Got authentication from UCS when they are already authenticated.");
				break;
			}
			default:
			{
				_log(UCS__ERROR, "Unknown ServerOPcode from UCS 0x%04x, size %d", pack->opcode, pack->size);
				DumpPacket(pack->pBuffer, pack->size);
				break;
			}
		}

		delete pack;
	}
	return(true);
}

bool UCSConnection::sendPacket(ServerPacket* pPacket) {
	if(!mTCPConnection) return false;
	return mTCPConnection->SendPacket(pPacket);
}

void UCSConnection::disconnect() {
	if (mTCPConnection) mTCPConnection->Disconnect();
}

uint32 UCSConnection::getIP() const {
	return mTCPConnection ? mTCPConnection->GetrIP() : 0;
}
