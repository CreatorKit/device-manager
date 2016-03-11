/***************************************************************************************************
 * Copyright (c) 2016, Imagination Technologies Limited
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions
 *    and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file fdm_server_session.c
 * @brief Defines methods to establish and release sessions with Awa LWM2M server.
 */

/***************************************************************************************************
 * Includes
 **************************************************************************************************/

#include "awa/server.h"
#include "fdm_log.h"

/***************************************************************************************************
 * Methods
 **************************************************************************************************/

AwaServerSession *Server_EstablishSession(const char *address, unsigned int port)
{
	// Initialise Device Management session
	AwaServerSession *session;
	session = AwaServerSession_New();

	if (session == NULL)
	{
		LOG(LOG_ERR, "Failed to create new server session");
		return NULL;
	}

	if (AwaServerSession_SetIPCAsUDP(session, address, port) != AwaError_Success)
	{
		LOG(LOG_ERR, "Failed to set IPC as UDP for server session");
		AwaServerSession_Free(&session);
		return NULL;
	}


	if (AwaServerSession_Connect(session) == AwaError_Success)
	{
		LOG(LOG_INFO, "Session established with server");
	}
	else
	{
		LOG(LOG_ERR, "Failed to establish session with server");
		AwaServerSession_Free(&session);
	}
	return session;
}

void Server_ReleaseSession(AwaServerSession **session)
{
	if (session == NULL)
	{
		return;
	}

	if (AwaServerSession_Disconnect(*session) != AwaError_Success)
	{
		LOG(LOG_ERR, "Failed to disconnect session with server");
	}

	if (AwaServerSession_Free(session) != AwaError_Success)
	{
		LOG(LOG_ERR, "Failed to free session with server");
	}
}
