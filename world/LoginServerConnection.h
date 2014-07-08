/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2002 EQEMu Development Team (http://eqemu.org)

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
#ifndef LOGINSERVER_H
#define LOGINSERVER_H

#include "../common/timer.h"
#include "../common/types.h"

class EmuTCPConnection;
class ServerPacket;
class World;

class LoginServerConnection{
public:
	LoginServerConnection(World* pWorld, const char* pAddress, uint16 pPort, const char* pAccountName, const char* pPassword);
	~LoginServerConnection();

	bool initialise();
	void update();
	bool connect();
	void sendNewInfo();
	void sendStatus();
	void sendPacket(ServerPacket* pPacket);
	bool isConnectReady();
	bool isConnected();

private:
	World* mWorld;
	EmuTCPConnection* mTCPConnection;
	char mLoginServerAddress[256];
	uint32 mLoginServerIP;
	uint16 mLoginServerPort;
	char mLoginAccount[32];
	char mLoginPassword[32];
	Timer mStatusUpdateTimer;
};
#endif
