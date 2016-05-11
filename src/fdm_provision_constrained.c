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
  * @file fdm_provision_constrained.c
  * @brief Implementation for provisioning constrained devices
  */

/***************************************************************************************************
 * Includes
 **************************************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <stdarg.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include "awa/common.h"
#include "awa/server.h"
#include "device_manager.h"
#include "fdm_common.h"
#include "fdm_log.h"
#include "fdm_server_session.h"
#include "fdm_register.h"

/***************************************************************************************************
 * Definitions
 **************************************************************************************************/

//! @cond Doxygen_Suppress
#define QUERY_TIMEOUT 5000

#define COAP_TIMEOUT 10000

#define POLLING_SLEEP_SECONDS 2
//! @endcond

/***************************************************************************************************
 * Typedef
 **************************************************************************************************/

/**
 * Struct to hold paths for flow objects and resources
 */
typedef struct
{
    //! \{
    char flowObjectInstancePath[URL_PATH_SIZE];
    // FlowObject resources
    char fcapPath[URL_PATH_SIZE];
    char deviceTypePath[URL_PATH_SIZE];
    char licenseeIDPath[URL_PATH_SIZE];
    char parentIDPath[URL_PATH_SIZE];
    //! \}

} Paths;

/***************************************************************************************************
 * Globals
 **************************************************************************************************/

//! @cond Doxygen_Suppress
static Paths pathStore;
static bool pathsMade = false;
//! @endcond

/***************************************************************************************************
 * Implementation
 **************************************************************************************************/

/**
 * @brief Generate Object and Resource paths.
 * @return true if object and resource paths are generated successfully, else false.
 */
static bool MakePaths(void)
{
    memset(&pathStore, 0, sizeof(Paths));
    if (AwaAPI_MakeObjectInstancePath(pathStore.flowObjectInstancePath, URL_PATH_SIZE,
            Lwm2mObjectId_FlowObject, 0) != AwaError_Success ||
        // FlowObject resources
        AwaAPI_MakeResourcePath(pathStore.fcapPath, URL_PATH_SIZE, Lwm2mObjectId_FlowObject, 0,
            FlowObjectResourceId_Fcap) != AwaError_Success ||
        AwaAPI_MakeResourcePath(pathStore.deviceTypePath, URL_PATH_SIZE,
            Lwm2mObjectId_FlowObject, 0,
            FlowObjectResourceId_DeviceType) != AwaError_Success ||
        AwaAPI_MakeResourcePath(pathStore.licenseeIDPath, URL_PATH_SIZE,
            Lwm2mObjectId_FlowObject, 0,
            FlowObjectResourceId_LicenseeId) != AwaError_Success ||
        AwaAPI_MakeResourcePath(pathStore.parentIDPath, URL_PATH_SIZE,
            Lwm2mObjectId_FlowObject, 0,
            FlowObjectResourceId_ParentId) != AwaError_Success)
    {
        LOG(LOG_ERR, "Couldn't generate all object and resource paths");
        return false;
    }

    return true;
}

/**
 * @brief Check if flow access instance is registered or not.
 * @param[in] session Holds server session.
 * @param[in] clientListResponse List of responses from client.
 * @return true if flow access instance found, else false.
 */
bool IsFlowAccessInstanceRegistered(const AwaServerSession *session, const AwaServerListClientsResponse *clientListResponse)
{
    bool found = false;
    AwaRegisteredEntityIterator *objectIterator = AwaServerListClientsResponse_NewRegisteredEntityIterator(clientListResponse);
    while (AwaRegisteredEntityIterator_Next(objectIterator))
    {
        const char *path = AwaRegisteredEntityIterator_GetPath(objectIterator);
        AwaObjectID objectID = AWA_INVALID_ID;
        AwaObjectInstanceID objectInstanceID = AWA_INVALID_ID;
        AwaServerSession_PathToIDs(session, path, &objectID, &objectInstanceID, NULL);
        if (objectID == Lwm2mObjectId_FlowAccess && objectInstanceID == 0)
        {
            LOG(LOG_DBG, "Flow Access Instance Found");
            found = true;
            break;
        }
    }
    AwaRegisteredEntityIterator_Free(&objectIterator);
    return found;
}

/**
 * @brief Check if flow object instance is registered or not.
 * @param[in] session Holds server session.
 * @param[in] clientListResponse List of responses from client.
 * @return true if flow object instance found, else false.
 */
bool IsFlowObjectInstanceRegistered(const AwaServerSession *session, const AwaServerListClientsResponse *clientListResponse)
{
    bool found = false;
    AwaRegisteredEntityIterator *objectIterator = AwaServerListClientsResponse_NewRegisteredEntityIterator(clientListResponse);
    while (AwaRegisteredEntityIterator_Next(objectIterator))
    {
        const char *path = AwaRegisteredEntityIterator_GetPath(objectIterator);
        AwaObjectID objectID = AWA_INVALID_ID;
        AwaObjectInstanceID objectInstanceID = AWA_INVALID_ID;
        AwaServerSession_PathToIDs(session, path, &objectID, &objectInstanceID, NULL);
        if (objectID == Lwm2mObjectId_FlowObject && objectInstanceID == 0)
        {
            LOG(LOG_DBG, "Flow Object Instance Found");
            found = true;
            break;
        }
    }
    AwaRegisteredEntityIterator_Free(&objectIterator);
    return found;
}

/**
 * @brief Get device's flow object and flow access instance status.
 * @param[in] session Holds server session.
 * @param[in] clientID Holds ID of registered client.
 * @param[in] deviceStatus Pointer to structure holding device status information.
 * @return true if status is retrieved successfully, else false.
 */
static bool GetDeviceStatus(const AwaServerSession *session, const char *clientID, DeviceStatus *deviceStatus)
{
    AwaError error = AwaError_Unspecified;
    bool result = true;

    if (session == NULL || clientID == NULL || deviceStatus == NULL)
    {
        LOG(LOG_ERR, "Null arguments to %s()", __func__);
        return false;
    }

    AwaServerListClientsOperation *clientListOperation = AwaServerListClientsOperation_New(session);
    if (clientListOperation == NULL)
    {
        LOG(LOG_ERR, "Failed to create new client list operation");
        return false;
    }

    error = AwaServerListClientsOperation_Perform(clientListOperation, QUERY_TIMEOUT);
    if (error == AwaError_Success)
    {
        const AwaServerListClientsResponse *response = AwaServerListClientsOperation_GetResponse(clientListOperation, clientID);
        deviceStatus->isDevicePresent = (response != NULL);
        if (deviceStatus->isDevicePresent)
        {
            deviceStatus->isFlowAccessInstanceRegistered = IsFlowAccessInstanceRegistered(session, response);
            deviceStatus->isFlowObjectInstanceRegistered = IsFlowObjectInstanceRegistered(session, response);
        }
        else
        {
            deviceStatus->isFlowObjectInstanceRegistered = false;
            deviceStatus->isFlowAccessInstanceRegistered = false;
        }
    }
    else
    {
        LOG(LOG_ERR, "Failed to perform list clients operation");
    }
    AwaServerListClientsOperation_Free(&clientListOperation);
    return true;
}

/**
 * @brief Write to the parent ID resource of flow object of constrained device.
 * @param[in] session Holds server session.
 * @param[in] clientID Holds ID of registered client.
 * @param[in] parentID Parent ID to be assigned.
 * @return true if parent ID written successfully, else false.
 */
static bool WriteParentID (const AwaServerSession *session, const char *clientID, const char *parentID)
{
    unsigned char i, gatewayDeviceID[DEVICE_ID_SIZE];
    AwaOpaque parentIDOpaque;
    AwaError error = AwaError_Success;
    bool result = false;
    // 3 because two is for hex letters and one for space
    if (strlen(parentID)/3 != DEVICE_ID_SIZE)
    {
        LOG(LOG_ERR, "ParentID is not of %u bytes", DEVICE_ID_SIZE);
        return false;
    }

    AwaServerWriteOperation *writeOp = AwaServerWriteOperation_New(session, AwaWriteMode_Update);
    if (writeOp != NULL)
    {
        for (i = 0; i < DEVICE_ID_SIZE; i++)
        {
            // 3 because two is for hex letters and one for space
            sscanf(&parentID[i*3], "%02hhX", &gatewayDeviceID[i]);
        }
        parentIDOpaque.Data = gatewayDeviceID;
        parentIDOpaque.Size = sizeof(gatewayDeviceID);
        error = AwaServerWriteOperation_AddValueAsOpaque(writeOp, pathStore.parentIDPath, parentIDOpaque);
        if (error == AwaError_Success)
        {
            error = AwaServerWriteOperation_Perform(writeOp, clientID, COAP_TIMEOUT);
            if (error == AwaError_Success)
            {
                result = true;
            }
            else
            {
                LOG(LOG_ERR, "Failed to write parentID\nerror: %s", AwaError_ToString(error));
            }
        }
        AwaServerWriteOperation_Free(&writeOp);
    }
    return result;
}

/**
 * @brief Write provisioning information e.g. device type, licensee ID and fcap to device.
 * @param[in] session Holds server session.
 * @param[in] clientID Holds ID of registered client.
 * @param[in] fcapCode Pointer to fcap code.
 * @param[in] deviceType Pointer to device type.
 * @param[in] licenseeID Licensee ID.
 * @param[in] isFlowObjectInstanceRegistered States if flow object instance is registered or not.
 * @return true if provisioning information is written successfully to device, else false.
 */
static bool WriteProvisioningInformationToDevice (const AwaServerSession *session,
    const char *clientID, const char *fcapCode, const char *deviceType, int licenseeID, bool isFlowObjectInstanceRegistered)
{
    bool result = false;
    AwaError error = AwaError_Success;
    AwaServerWriteOperation *writeOp = AwaServerWriteOperation_New(session, AwaWriteMode_Update);
    if (writeOp != NULL)
    {
        if (!isFlowObjectInstanceRegistered)
        {
            AwaServerWriteOperation_CreateObjectInstance(writeOp, pathStore.flowObjectInstancePath);
        }

        error = AwaServerWriteOperation_AddValueAsCString(writeOp, pathStore.fcapPath, fcapCode);
        if (error == AwaError_Success)
        {
            error = AwaServerWriteOperation_AddValueAsCString(writeOp, pathStore.deviceTypePath, deviceType);
        }
        if (error == AwaError_Success)
        {
            error = AwaServerWriteOperation_AddValueAsInteger(writeOp, pathStore.licenseeIDPath, licenseeID);
        }
        if (error == AwaError_Success)
        {
            error = AwaServerWriteOperation_Perform(writeOp, clientID, COAP_TIMEOUT);
            if (error == AwaError_Success)
            {
                result = true;
            }
            else
            {
                LOG(LOG_ERR, "Failed to perform write operation\nerror: %s", AwaError_ToString(error));
            }
        }
        else
        {
            LOG(LOG_ERR, "Failed to create write request\nerror: %s", AwaError_ToString(error));
        }
        AwaServerWriteOperation_Free(&writeOp);
    }
    return result;
}

/**
 * @brief Wait for specified timeout until provisioning is done.
 * @param[in] serverSession Holds server session.
 * @param[in] clientID Holds ID of registered client.
 * @param[in] timeout Time to wait for provisioning to complete.
 * @return true if provisioning is successful, else false.
 */
static bool WaitForProvisioning(AwaServerSession *serverSession, const char *clientID, int timeout)
{
    DeviceStatus deviceStatus;
    while (timeout-- != 0)
    {
        GetDeviceStatus(serverSession, clientID, &deviceStatus);
        if (deviceStatus.isFlowAccessInstanceRegistered)
        {
            return true;
        }
        sleep(POLLING_SLEEP_SECONDS);
    }
    LOG(LOG_ERR, "Failed to provision device");
    return false;
}

bool IsConstrainedDeviceProvisioned(const char *clientID)
{
    DeviceStatus deviceStatus;
    if (clientID == NULL)
    {
        LOG(LOG_ERR, "Null arguments to %s()", __func__);
        return false;
    }

    AwaServerSession *serverSession = Server_EstablishSession(SERVER_ADDRESS, SERVER_PORT);

    if (serverSession == NULL)
    {
        LOG(LOG_ERR, "Failed to establish session with server");
        return false;
    }
    GetDeviceStatus(serverSession, clientID, &deviceStatus);
    Server_ReleaseSession(&serverSession);
    return deviceStatus.isFlowAccessInstanceRegistered;
}

ProvisionStatus ProvisionConstrainedDevice(const char *clientID, const char*fcap,
    const char *deviceType, int licenseeID, const char *parentID, int timeout)
{
    ProvisionStatus result = PROVISION_FAIL;
    OBJECT_T flowObjects[] = {flowObject, flowAccessObject};
    DeviceStatus deviceStatus;
    LOG(LOG_INFO, "Provision constrained device:\n"
        "\n%-11s\t = %s\n%-11s\t = %s\n%-11s\t = %d\n%-11s\t = %s", "Client ID", clientID, "Device Type",
        deviceType, "Licensee ID", licenseeID, "Parent ID", parentID);
    if (clientID == NULL || fcap == NULL || parentID == NULL)
    {
        LOG(LOG_ERR, "Null arguments to %s()", __func__);
        return PROVISION_FAIL;
    }

    AwaServerSession *serverSession = Server_EstablishSession(SERVER_ADDRESS, SERVER_PORT);
    if (serverSession == NULL)
    {
        LOG(LOG_ERR, "Failed to establish session with server");
        return PROVISION_FAIL;
    }

    if (!pathsMade)
    {
        if (!MakePaths())
        {
            LOG(LOG_ERR, "Failed to create paths");
            return false;
        }
        pathsMade = true;
    }

    if (DefineObjectsAtServer(serverSession, flowObjects, ARRAY_SIZE(flowObjects)))
    {
        GetDeviceStatus(serverSession, clientID, &deviceStatus);
        if (!deviceStatus.isDevicePresent)
        {
            LOG(LOG_ERR, "Device not present");
        }
        else if (deviceStatus.isFlowAccessInstanceRegistered)
        {
            LOG(LOG_INFO, "Device already provisioned");
            result = ALREADY_PROVISIONED;
        }
        else
        {
            if (!WriteProvisioningInformationToDevice(serverSession, clientID, fcap, deviceType, licenseeID, deviceStatus.isFlowObjectInstanceRegistered) ||
                !WriteParentID(serverSession, clientID, parentID))
            {
                LOG(LOG_ERR, "Writing of device provisioning information failed");
            }
            else if (WaitForProvisioning(serverSession, clientID, timeout))
            {
                result = PROVISION_OK;
            }
        }
    }
    else
    {
        LOG(LOG_ERR, "Failed to register flow objects' definitions at the server");
    }
    Server_ReleaseSession(&serverSession);
    LOG(LOG_INFO, "status = %d", result);
    return result;
}
