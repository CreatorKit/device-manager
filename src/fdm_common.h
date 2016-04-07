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
 * @file fdm_common.h
 * @brief Header file for exposing structure definitions and enums.
 */

#ifndef FDM_COMMON_H
#define FDM_COMMON_H

#include "awa/client.h"

//! \{
#define URL_PATH_SIZE       (16)
#define MAX_STR_SIZE        (64)
#define IPC_TIMEOUT         (1000)
#define SLEEP_COUNT         (2)
#define OBJECT_INSTANCE_ID  (0)
#define DEVICE_ID_SIZE      (16)
#define ARRAY_SIZE(arr)     (sizeof(arr)/sizeof(arr[0]))

#define SERVER_ADDRESS              "127.0.0.1"
#define SERVER_PORT                 54321


#define MAKE_RESOURCE_PATH(path, objectId, resourceId) \
    AwaAPI_MakeResourcePath(path, URL_PATH_SIZE, objectId, OBJECT_INSTANCE_ID, resourceId)

#define MAKE_FLOW_OBJECT_RESOURCE_PATH(path, resourceId) \
    MAKE_RESOURCE_PATH(path, Lwm2mObjectId_FlowObject, resourceId)

#define MAKE_FLOW_ACCESS_OBJECT_RESOURCE_PATH(path, resourceId) \
    MAKE_RESOURCE_PATH(path, Lwm2mObjectId_FlowAccess, resourceId)

#define MAKE_FLOW_OBJECT_INSTANCE_PATH(path) \
    AwaAPI_MakeObjectInstancePath(path, URL_PATH_SIZE, Lwm2mObjectId_FlowObject, \
        OBJECT_INSTANCE_ID)

#define MAKE_FLOW_ACCESS_OBJECT_PATH(path) \
    AwaAPI_MakeObjectPath(path, URL_PATH_SIZE, Lwm2mObjectId_FlowAccess)
//! \}

/**
 * lwm2m object ids enum.
 */
typedef enum {
    Lwm2mObjectId_DeviceObject = 3,
    Lwm2mObjectId_FlowObject = 20000,
    Lwm2mObjectId_FlowAccess
} Lwm2mObjectId;

/**
 * Flow object resources enum.
 */
typedef enum {
    FlowObjectResourceId_DeviceId,
    FlowObjectResourceId_ParentId,
    FlowObjectResourceId_DeviceType,
    FlowObjectResourceId_DeviceName,
    FlowObjectResourceId_Description,
    FlowObjectResourceId_Fcap,
    FlowObjectResourceId_LicenseeId,
    FlowObjectResourceId_LicenseeChallenge,
    FlowObjectResourceId_HashIterations,
    FlowObjectResourceId_LicenseeHash,
    FlowObjectResourceId_Status
} FlowObjectResourceId;

/**
 * Flow access resources enum.
 */
typedef enum {
    FlowAccessResourceId_Url,
    FlowAccessResourceId_CustomerKey,
    FlowAccessResourceId_CustomerSecret,
    FlowAccessResourceId_RememberMeToken,
    FlowAccessResourceId_RememberMeTokenExpiry
} FlowAccessResourceId;

/**
 * Device object resources enum
 */
typedef enum {
    DeviceObjectResourceId_SerialNumber = 2,
    DeviceObjectResourceId_SoftwareVersion = 19
} DeviceObjectResourceId;

/**
 * A structure to contain resource information.
 */
typedef struct
{
    //! \{
    AwaResourceID id;
    const char *name;
    AwaResourceType type;
    bool isMandatory;
    bool wantToSave;
    //! \}
} RESOURCE_T;

/**
 * A structure to contain object information.
 */
typedef struct
{
    //! \{
    const char *name;
    AwaObjectID id;
    unsigned int numResources;
    RESOURCE_T *resources;
    //! \}
} OBJECT_T;

extern const OBJECT_T flowObject, flowAccessObject;
/**
 * lwm2m objects subscriptions.
 */
typedef struct
{
    //! \{
    AwaClientChangeSubscription *flowObjectChange;
    AwaClientChangeSubscription *flowAccessObjectChange;
    //! \}
} FlowSubscriptions;

/**
 * Verification details.
 */
typedef struct
{
    //! \{
    AwaOpaque challenge;
    AwaInteger iterations;
    AwaOpaque licenseeHash;
    bool hasChallenge;
    bool hasIterations;
    bool waitForServerResponse;
    bool verifyLicensee;
    bool isProvisionSuccess;
    //! \}
} Verification;

#endif  /* FDM_COMMON_H */
