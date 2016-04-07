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

#define QUERY_TIMEOUT 5000

#define COAP_TIMEOUT 10000

#define POLLING_TIMEOUT_SECONDS 30

#define POLLING_SLEEP_SECONDS 2

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
    char flowAccessObjectPath[URL_PATH_SIZE];
    char flowAccessObjectInstancePath[URL_PATH_SIZE];

    // FlowObject resources
    char fcapPath[URL_PATH_SIZE];
    char deviceTypePath[URL_PATH_SIZE];
    char licenseeIDPath[URL_PATH_SIZE];
    char parentIDPath[URL_PATH_SIZE];


    // FlowAccess object resources
    char flowCloudUrlPath[URL_PATH_SIZE];
    char customerKeyPath[URL_PATH_SIZE];
    char customerSecretPath[URL_PATH_SIZE];
    char rememberMeTokenPath[URL_PATH_SIZE];
    char rememberMeTokenExpiryPath[URL_PATH_SIZE];
    //! \}

} Paths;

/***************************************************************************************************
 * Globals
 **************************************************************************************************/

static Paths pathStore;
static bool isProvisioned = false;
static bool pathsMade = false;

/***************************************************************************************************
 * Implementation
 **************************************************************************************************/

static bool MakePaths(void)
{
    memset(&pathStore, 0, sizeof(Paths));
    if (AwaAPI_MakeObjectInstancePath(pathStore.flowObjectInstancePath, URL_PATH_SIZE,
            Lwm2mObjectId_FlowObject, 0) != AwaError_Success ||
        AwaAPI_MakeObjectPath(pathStore.flowAccessObjectPath, URL_PATH_SIZE,
                Lwm2mObjectId_FlowAccess) != AwaError_Success ||
        AwaAPI_MakeObjectInstancePath(pathStore.flowAccessObjectInstancePath, URL_PATH_SIZE,
            Lwm2mObjectId_FlowAccess, 0) != AwaError_Success ||
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
            FlowObjectResourceId_ParentId) != AwaError_Success ||
        // FlowAccess resources
        AwaAPI_MakeResourcePath(pathStore.flowCloudUrlPath, URL_PATH_SIZE,
            Lwm2mObjectId_FlowAccess, 0, FlowAccessResourceId_Url) != AwaError_Success ||
        AwaAPI_MakeResourcePath(pathStore.customerKeyPath, URL_PATH_SIZE,
            Lwm2mObjectId_FlowAccess, 0, FlowAccessResourceId_CustomerKey) !=
            AwaError_Success ||
        AwaAPI_MakeResourcePath(pathStore.customerSecretPath, URL_PATH_SIZE,
            Lwm2mObjectId_FlowAccess, 0, FlowAccessResourceId_CustomerSecret) !=
            AwaError_Success ||
        AwaAPI_MakeResourcePath(pathStore.rememberMeTokenPath, URL_PATH_SIZE,
            Lwm2mObjectId_FlowAccess, 0, FlowAccessResourceId_RememberMeToken) !=
            AwaError_Success ||
        AwaAPI_MakeResourcePath(pathStore.rememberMeTokenExpiryPath, URL_PATH_SIZE,
            Lwm2mObjectId_FlowAccess, 0, FlowAccessResourceId_RememberMeTokenExpiry) !=
            AwaError_Success)
    {
        LOG(LOG_ERR, "Couldn't generate all object and resource paths");
        return false;
    }

    return true;
}

bool IsDeviceProvisioned(const AwaServerSession *session, const char *clientID)
{
    AwaError error = AwaError_Unspecified;
    bool result = false;
    const char * tempStr = NULL;
    const AwaTime *tempTimePtr = NULL;

    if (!pathsMade)
    {
        if (!MakePaths())
        {
            LOG(LOG_ERR, "Failed to create paths");
            return false;
        }
        pathsMade = true;
    }

    AwaServerReadOperation *readOp = AwaServerReadOperation_New(session);
    if (readOp == NULL)
    {
        LOG(LOG_ERR, "Couldn't create new ServerReadOperation for reading client resources");
        return false;
    }
    error = AwaServerReadOperation_AddPath(readOp, clientID, pathStore.flowAccessObjectInstancePath);
    if (error != AwaError_Success)
    {
        LOG(LOG_ERR, "Couldn't add  %s path to the read operation\nerror: %s",
            pathStore.flowAccessObjectInstancePath, AwaError_ToString(error));
        goto read_fail;
    }

    error = AwaServerReadOperation_Perform(readOp, COAP_TIMEOUT);
    if (error != AwaError_Success)
    {
        LOG(LOG_ERR, "Couldn't perform read operation\nerror: %s", AwaError_ToString(error));
        goto read_fail;
    }
    const AwaServerReadResponse *readResponse = AwaServerReadOperation_GetResponse(readOp, clientID);
    if (readResponse == NULL)
    {
        LOG(LOG_ERR, "Failed to retrieve read response");
        goto read_fail;
    }

    if (!AwaServerReadResponse_ContainsPath(readResponse, pathStore.flowAccessObjectInstancePath))
    {
        LOG(LOG_ERR, "Read response does not contain FlowAccess Object");
        goto read_fail;
    }

    error = AwaServerReadResponse_GetValueAsCStringPointer(readResponse, pathStore.flowCloudUrlPath, &tempStr);
    if (error != AwaError_Success || tempStr == NULL)
    {
        LOG(LOG_ERR, "Failed to retrieve FlowCloudUrl\nerror: %s", AwaError_ToString(error));
        goto read_fail;
    }

    error = AwaServerReadResponse_GetValueAsCStringPointer(readResponse, pathStore.customerKeyPath, &tempStr);
    if (error != AwaError_Success || tempStr == NULL)
    {
        LOG(LOG_ERR, "Failed to retrieve CustomerKey\nerror: %s", AwaError_ToString(error));
        goto read_fail;
    }

    error = AwaServerReadResponse_GetValueAsCStringPointer(readResponse, pathStore.customerSecretPath, &tempStr);
    if (error != AwaError_Success || tempStr == NULL)
    {
        LOG(LOG_ERR, "Failed to retrieve CustomerSecret\nerror: %s", AwaError_ToString(error));
        goto read_fail;
    }

    error = AwaServerReadResponse_GetValueAsCStringPointer(readResponse, pathStore.rememberMeTokenPath, &tempStr);
    if (error != AwaError_Success || tempStr == NULL)
    {
        LOG(LOG_ERR, "Failed to retrieve RememberMeToken\nerror: %s", AwaError_ToString(error));
        goto read_fail;
    }

    error = AwaServerReadResponse_GetValueAsTimePointer(readResponse, pathStore.rememberMeTokenExpiryPath, &tempTimePtr);
    if (error != AwaError_Success || *tempTimePtr == 0)
    {
        LOG(LOG_ERR, "Failed to retrieve RememberMeTokenExpiry\nerror: %s", AwaError_ToString(error));
        goto read_fail;
    }
    else
    {
        result = true;
    }

    read_fail:
    AwaServerReadOperation_Free(&readOp);
    return result;
}

static bool IsDevicePresent(const AwaServerSession *session, const char *clientID)
{

    AwaError error = AwaError_Unspecified;
    bool result = false;
    if (session == NULL || clientID == NULL)
        return false;

    AwaServerListClientsOperation *clientListOperation = AwaServerListClientsOperation_New(session);
    if (clientListOperation == NULL)
    {
        return false;
    }

    error = AwaServerListClientsOperation_Perform(clientListOperation, QUERY_TIMEOUT);
    if (error == AwaError_Success)
    {
        const AwaServerListClientsResponse *response = AwaServerListClientsOperation_GetResponse(clientListOperation, clientID);
        result = (response != NULL);
    }
    AwaServerListClientsOperation_Free(&clientListOperation);
    return result;
}


static bool IsFlowObjectInstancePresent(const AwaServerSession *session, const char *clientID)
{
    AwaServerReadOperation *readOp = AwaServerReadOperation_New(session);
    if (readOp == NULL)
    {
        LOG(LOG_ERR, "Couldn't create new ServerReadOperation for reading client resources");
        return false;
    }
    AwaError error = AwaServerReadOperation_AddPath(readOp, clientID, pathStore.flowObjectInstancePath);
    if (error != AwaError_Success)
    {
        LOG(LOG_ERR, "Couldn't add  %s path to the read operation\nerror: %s",
            pathStore.flowObjectInstancePath, AwaError_ToString(error));
        return false;
    }

    error = AwaServerReadOperation_Perform(readOp, COAP_TIMEOUT);
    if (error != AwaError_Success)
    {
        LOG(LOG_ERR, "Couldn't perform read operation\nerror: %s", AwaError_ToString(error));
        return false;
    }

    const AwaServerReadResponse *readResponse = AwaServerReadOperation_GetResponse(readOp, clientID);
    if (readResponse == NULL)
    {
        LOG(LOG_ERR, "Failed to retrieve read response");
        return false;
    }

    if (!AwaServerReadResponse_ContainsPath(readResponse, pathStore.flowObjectInstancePath))
    {
        LOG(LOG_ERR, "Read response does not contain FlowAccess Object");
        return false;
    }
    return true;
}

static void FlowAccessObjectUpdated(const AwaChangeSet *changeSet, void *context)
{
    LOG(LOG_DBG, "In FlowAccessObjectUpdated");
    if (changeSet != NULL)
    {
        if (!pathsMade)
        {
            if (!MakePaths())
            {
                LOG(LOG_ERR, "Failed to create paths");
                return;
            }
            pathsMade = true;
        }

        const char *url=NULL, *key=NULL, *secret=NULL, *token=NULL;
        const AwaTime *tokenExpiry = NULL;
        if (AwaChangeSet_ContainsPath(changeSet, pathStore.flowCloudUrlPath) &&
            AwaChangeSet_HasValue(changeSet, pathStore.flowCloudUrlPath))
        {
            AwaChangeSet_GetValueAsCStringPointer(changeSet, pathStore.flowCloudUrlPath, &url);
            LOG(LOG_DBG, "Retrieved flowcloudUrl");
        }
        else
        {
            LOG(LOG_DBG, "Failed to retrieve flowcloudUrl");
        }

        if (AwaChangeSet_ContainsPath(changeSet, pathStore.customerKeyPath) &&
            AwaChangeSet_HasValue(changeSet, pathStore.customerKeyPath))
        {
            AwaChangeSet_GetValueAsCStringPointer(changeSet, pathStore.customerKeyPath, &key);
            LOG(LOG_DBG, "Retrieved customerKey");
        }
        else
        {
            LOG(LOG_DBG, "Failed to retrieve customerKey");
        }

        if (AwaChangeSet_ContainsPath(changeSet, pathStore.customerSecretPath) &&
            AwaChangeSet_HasValue(changeSet, pathStore.customerSecretPath))
        {
            AwaChangeSet_GetValueAsCStringPointer(changeSet, pathStore.customerSecretPath, &secret);
            LOG(LOG_DBG, "Retrieved customerSecret");
        }
        else
        {
            LOG(LOG_DBG, "Failed to retrieve customerSecret");
        }

        if (AwaChangeSet_ContainsPath(changeSet, pathStore.rememberMeTokenPath) &&
            AwaChangeSet_HasValue(changeSet, pathStore.rememberMeTokenPath))
        {
            AwaChangeSet_GetValueAsCStringPointer(changeSet, pathStore.rememberMeTokenPath, &token);
            LOG(LOG_DBG, "Retrieved rememberMeToken");
        }
        else
        {
            LOG(LOG_DBG, "Failed to retrieve rememberMeToken");
        }

        if (AwaChangeSet_ContainsPath(changeSet, pathStore.rememberMeTokenExpiryPath) &&
            AwaChangeSet_HasValue(changeSet, pathStore.rememberMeTokenExpiryPath))
        {
            AwaChangeSet_GetValueAsTimePointer(changeSet, pathStore.rememberMeTokenExpiryPath, &tokenExpiry);
            LOG(LOG_DBG, "Retrieved rememberMeTokenExpiry");
        }
        else
        {
            LOG(LOG_DBG, "Failed to retrieve rememberMeTokenExpiry");
        }

        if ((url != NULL) && (key != NULL) && (secret != NULL) && (token != NULL) &&
            (*tokenExpiry != 0))
        {
            LOG(LOG_INFO, "Provisioned");
            isProvisioned = true;
        }
        else
        {
            LOG(LOG_INFO, "Not Provisioned yet");
        }
    }
    else
    {
        LOG(LOG_ERR, "Invalid FlowAccess object changeset");
    }
}

static bool ObserveDeviceFlowAccessObject(const AwaServerSession *session, const char *clientID,
    AwaServerObserveOperation **flowAccessObjectChange,
    AwaServerObservation **flowAccessObjectChangeObservation)
{
    if ((session == NULL) || (clientID == NULL))
    {
        LOG(LOG_ERR, "Null arguments to %s()", __func__);
        return false;
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

    AwaError error = AwaError_Unspecified;
    *flowAccessObjectChange = AwaServerObserveOperation_New(session);
    if (*flowAccessObjectChange == NULL)
    {
        LOG(LOG_ERR, "Could not create new observe operation to subscribe to constrained device's FlowAccess object");
        return false;
    }

    *flowAccessObjectChangeObservation = AwaServerObservation_New(clientID,
        pathStore.flowAccessObjectPath, FlowAccessObjectUpdated, NULL);
    if (*flowAccessObjectChangeObservation == NULL)
    {
        LOG(LOG_ERR, "Could not create new observation to subscribe to constrained device's FlowAccess object");
        AwaServerObserveOperation_Free(flowAccessObjectChange);
        *flowAccessObjectChange = NULL;
        return false;
    }

    error = AwaServerObserveOperation_AddObservation(*flowAccessObjectChange, *flowAccessObjectChangeObservation);
    if (error == AwaError_Success)
    {
        error = AwaServerObserveOperation_Perform(*flowAccessObjectChange, COAP_TIMEOUT);
        if (error == AwaError_Success)
        {
            LOG(LOG_INFO, "Subscribed to FlowAccess object");
            return true;
        }
        else
        {
            LOG(LOG_ERR, "Error failed to observe client FlowAccess object\nerror: %s", AwaError_ToString(error));
        }
    }
    else
    {
        LOG(LOG_ERR, "Error failed to add observation to observe client FlowAccess operation\nerror: %s", AwaError_ToString(error));
    }

    AwaServerObservation_Free(flowAccessObjectChangeObservation);
    *flowAccessObjectChangeObservation = NULL;
    AwaServerObserveOperation_Free(flowAccessObjectChange);
    *flowAccessObjectChange = NULL;
    return false;
}

static bool RemoveDeviceObservations(const char *clientID,
    AwaServerObserveOperation **flowAccessObjectChange,
    AwaServerObservation **flowAccessObjectChangeObservation)
{
    bool result = false;
    if ((clientID == NULL) ||
        (flowAccessObjectChange == NULL) ||
        (flowAccessObjectChangeObservation == NULL))
    {
        LOG(LOG_ERR, "Null arguments to %s()", __func__);
        return false;
    }

    AwaError error = AwaError_Unspecified;
    AwaServerObservation_Free(flowAccessObjectChangeObservation);
    *flowAccessObjectChangeObservation = NULL;

    AwaServerObservation *cancelObservation = AwaServerObservation_New(clientID, pathStore.flowAccessObjectPath, FlowAccessObjectUpdated, NULL);
    if (cancelObservation != NULL)
    {
        error = AwaServerObserveOperation_AddCancelObservation(*flowAccessObjectChange, cancelObservation);
        if (error == AwaError_Success)
        {
            error = AwaServerObserveOperation_Perform(*flowAccessObjectChange, COAP_TIMEOUT);
            if (error == AwaError_Success)
            {
                result = true;
            }
            else
            {
                LOG(LOG_ERR, "Failed to cancel observation of constrained device's FlowAccess object\nerror: %s", AwaError_ToString(error));
            }
        }
        else
        {
            LOG(LOG_ERR, "Failed to add cancel observation of constrained device's FlowAccess object\nerror: %s", AwaError_ToString(error));
        }
        AwaServerObservation_Free(&cancelObservation);
    }
    else
    {
        LOG(LOG_ERR, "Couldn't create a cancel observation for constrained device's FlowAccess object");
    }

    AwaServerObserveOperation_Free(flowAccessObjectChange);
    *flowAccessObjectChange = NULL;

    return result;
}

static bool WriteProvisioningInformationToDevice (const AwaServerSession *session,
    const char *clientID, const char *fcapCode, const char *deviceType, int licenseeID,
    const char *parentID)
{
    bool result = false;
    AwaError error = AwaError_Success;
    unsigned char i, gatewayDeviceID[DEVICE_ID_SIZE];
    AwaOpaque parentIDOpaque;

    // 3 because two is for hex letters and one for space
    if (strlen(parentID)/3 != DEVICE_ID_SIZE)
    {
        LOG(LOG_ERR, "ParentID is not of %u bytes", DEVICE_ID_SIZE);
        return false;
    }

    AwaServerWriteOperation *writeOp = AwaServerWriteOperation_New(session, AwaWriteMode_Update);
    if (writeOp != NULL)
    {
        if (!IsFlowObjectInstancePresent(session, clientID))
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
            for (i = 0; i < DEVICE_ID_SIZE; i++)
            {
                // 3 because two is for hex letters and one for space
                sscanf(&parentID[i*3], "%02hhX", &gatewayDeviceID[i]);
            }
            parentIDOpaque.Data = gatewayDeviceID;
            parentIDOpaque.Size = sizeof(gatewayDeviceID);
            error = AwaServerWriteOperation_AddValueAsOpaque(writeOp, pathStore.parentIDPath, parentIDOpaque);
        }
        if (error == AwaError_Success)
        {
            error = AwaServerWriteOperation_Perform(writeOp, clientID, COAP_TIMEOUT);
            if (error == AwaError_Success)
            {
                result = true;
            }
        }
        else
        {
            LOG(LOG_ERR, "Failed to create write request\nerror: %s",
                AwaError_ToString(error));
        }

        AwaServerWriteOperation_Free(&writeOp);
    }
    return result;
}

static bool WaitForNotification(AwaServerSession *session)
{
    int timeout = POLLING_TIMEOUT_SECONDS;
    while (timeout-- != 0)
    {
        AwaServerSession_Process(session, COAP_TIMEOUT);
        AwaServerSession_DispatchCallbacks(session);
        if (isProvisioned)
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
    bool status = false;
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

    if (IsDevicePresent(serverSession, clientID))
    {
        status = IsDeviceProvisioned(serverSession, clientID);
    }

    Server_ReleaseSession(&serverSession);
    return status;
}

ProvisionStatus ProvisionConstrainedDevice(const char *clientID, const char*fcap,
    const char *deviceType, int licenseeID, const char *parentID)
{
    ProvisionStatus result = PROVISION_FAIL;
    OBJECT_T flowObjects[] = {flowObject, flowAccessObject};
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

    if (DefineObjectsAtServer(serverSession, flowObjects, ARRAY_SIZE(flowObjects)))
    {
        if (!IsDevicePresent(serverSession, clientID))
        {
            LOG(LOG_ERR, "Device not present");
        }
        else if (IsDeviceProvisioned(serverSession, clientID))
        {
            LOG(LOG_INFO, "Device already provisioned");
            result = ALREADY_PROVISIONED;
        }
        else
        {
            AwaServerObserveOperation *flowAccessObjectChange = NULL;
            AwaServerObservation *flowAccessObjectChangeObservation = NULL;
            LOG(LOG_INFO, "Provisioning constrained device");
            isProvisioned = false;

            if (ObserveDeviceFlowAccessObject(serverSession, clientID, &flowAccessObjectChange, &flowAccessObjectChangeObservation))
            {
                if (!WriteProvisioningInformationToDevice(serverSession, clientID, fcap, deviceType, licenseeID, parentID))
                {
                    LOG(LOG_ERR, "Writing of device provisioning information failed");
                }
                else if (WaitForNotification(serverSession))
                {
                    result = PROVISION_OK;
                }
                RemoveDeviceObservations(clientID, &flowAccessObjectChange, &flowAccessObjectChangeObservation);
            }
            else
            {
                LOG(LOG_ERR, "Failed to observe device flow access object");
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
