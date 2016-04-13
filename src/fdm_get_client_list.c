/***************************************************************************************************
 * Copyright (c) 2016, Imagination Technologies Limited and/or its affiliated group companies
 * and/or licensors
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
 * @file fdm_get_client_list.c
 * @brief Lists clients registered on the Awa LWM2M server.
 */

/***************************************************************************************************
 * Includes
 **************************************************************************************************/
#include <stdio.h>
#include <json.h>
#include "awa/server.h"
#include "fdm_server_session.h"
#include "fdm_log.h"
#include "fdm_common.h"
#include "fdm_provision_constrained.h"

/***************************************************************************************************
 * Macros
 **************************************************************************************************/

//! \{
#define LIST_CLIENTS_OPERATION_TIMEOUT  5000
//! \}

/***************************************************************************************************
 * Methods
 **************************************************************************************************/

//! \{
static void ListClients(const AwaServerSession *session, json_object *respObj)
{
    AwaError error;
    json_object *listObj = json_object_new_array();
    AwaServerListClientsOperation *operation = AwaServerListClientsOperation_New(session);
    if (operation == NULL)
    {
        LOG(LOG_ERR, "Failed to create new ListClientsOperation");
        json_object_put(listObj);
        return;
    }
    error = AwaServerListClientsOperation_Perform(operation, LIST_CLIENTS_OPERATION_TIMEOUT);
    if (error == AwaError_Success)
    {
        AwaClientIterator *clientIterator = AwaServerListClientsOperation_NewClientIterator(operation);
        if (clientIterator != NULL)
        {
            while(AwaClientIterator_Next(clientIterator))
            {
                const char *clientID = AwaClientIterator_GetClientID(clientIterator);

                json_bool provisionStatus = IsDeviceProvisioned(session, clientID) ? 1 : 0;
                json_object *clientObj = json_object_new_object();
                json_object_object_add(clientObj, "clientId", json_object_new_string(clientID));
                json_object_object_add(clientObj, "is_device_provisioned", json_object_new_boolean(provisionStatus));
                json_object_array_add(listObj, clientObj);

            }
            AwaClientIterator_Free(&clientIterator);
        }
        else
        {
            LOG(LOG_ERR, "Failed to create new list clients iterator");
        }
    }
    else
    {
        LOG(LOG_ERR, "Failed to perform list clients operation\nerror: %s", AwaError_ToString(error));
    }

    if (AwaServerListClientsOperation_Free(&operation) != AwaError_Success)
    {
        LOG(LOG_ERR, "Failed to free list clients operation");
    }

    json_object_object_add(respObj, "clients", listObj);
}
//! \}

void GetClientList(json_object *respObj)
{
    AwaServerSession *session = NULL;
    session = Server_EstablishSession(SERVER_ADDRESS, SERVER_PORT);
    if (session != NULL)
    {
        ListClients(session, respObj);
        Server_ReleaseSession(&session);
    }
}
