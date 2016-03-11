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
 * @file flow_device_manager.c
 * @brief Provides provisioning operations.
 */

/***************************************************************************************************
 * Includes
 **************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "awa/common.h"
#include "awa/client.h"
#include "flow_device_manager.h"
#include "fdm_register.h"
#include "fdm_subscribe.h"
#include "fdm_licensee_verification.h"
#include "fdm_common.h"
#include "fdm_log.h"

/***************************************************************************************************
 * Macros
 **************************************************************************************************/

//! \{
#define IPC_PORT                 (12345)
#define IPC_ADDRESS              "127.0.0.1"
#define SERVER_RESPONSE_TIMEOUT  (30)
#define MAX_STRINGS              (15)
#define FLOW_ACCESS_CFG          "/etc/lwm2m/flow_access.cfg"
//! \}

/***************************************************************************************************
 * Globals
 **************************************************************************************************/

/**
* Flow object to hold Flow Cloud specific information.
*/
const OBJECT_T flowObject =
{ "FlowObject", Lwm2mObjectId_FlowObject, 11,
	(RESOURCE_T [])
	{
		{ FlowObjectResourceId_DeviceId, "DeviceID", AwaResourceType_Opaque, true },
		{ FlowObjectResourceId_ParentId, "ParentID", AwaResourceType_Opaque, false },
		{ FlowObjectResourceId_DeviceType, "DeviceType", AwaResourceType_String, true },
		{ FlowObjectResourceId_DeviceName, "Name", AwaResourceType_String, false },
		{ FlowObjectResourceId_Description, "Description", AwaResourceType_String, false },
		{ FlowObjectResourceId_Fcap, "FCAP", AwaResourceType_String, true },
		{ FlowObjectResourceId_LicenseeId, "LicenseeID", AwaResourceType_Integer, true },
		{ FlowObjectResourceId_LicenseeChallenge, "LicenseeChallenge",
			AwaResourceType_Opaque, false },
		{ FlowObjectResourceId_HashIterations, "HashIterations", AwaResourceType_Integer,
			false },
		{ FlowObjectResourceId_LicenseeHash, "LicenseeHash", AwaResourceType_Opaque, false},
		{ FlowObjectResourceId_Status, "Status", AwaResourceType_Integer, false }
	}
};

/**
* Flow Access object to hold information for accessing Flow Cloud.
*/
const OBJECT_T flowAccessObject =
{ "FlowAccess", Lwm2mObjectId_FlowAccess, 5,
	(RESOURCE_T [])
	{
		{ FlowAccessResourceId_Url, "URL", AwaResourceType_String, true },
		{ FlowAccessResourceId_CustomerKey, "CustomerKey", AwaResourceType_String, true },
		{ FlowAccessResourceId_CustomerSecret, "CustomerSecret", AwaResourceType_String,
			true },
		{ FlowAccessResourceId_RememberMeToken, "RememberMeToken", AwaResourceType_String,
			true },
		{ FlowAccessResourceId_RememberMeTokenExpiry, "RememberMeTokenExpiry",
			AwaResourceType_Integer, true }
	}
};

/**
* Initialise Awa standard objects.
* Device object: hold device specific information.
*/
static const OBJECT_T deviceObject =
{ "DeviceObject", Lwm2mObjectId_DeviceObject, 2,
	(RESOURCE_T [])
	{
		{ DeviceObjectResourceId_SerialNumber, "SerialNumber", AwaResourceType_String, true },
		{ DeviceObjectResourceId_SoftwareVersion, "SoftwareVersion", AwaResourceType_String,
			true }
	}
};

/**
 * A Session is required for interaction with the Awa LWM2M Core.
 * Operations to interact with Core resources are created in the context of a session.
 * The session is owned by the caller and should eventually be freed with AwaClientSession_Free.
 */
static AwaClientSession *session = NULL;

int debugLevel = LOG_INFO;
FILE *debugStream = NULL;

/***************************************************************************************************
 * Methods
 **************************************************************************************************/

FILE* SetLogFile(const char *file)
{
	FILE *logFile = NULL;
	logFile = fopen(file, "w");
	if (logFile != NULL)
	{
		LOG(LOG_DBG, "Log file set to %s", file);
		debugStream = logFile;
	}
	else
	{
		LOG(LOG_ERR, "Failed to create or open %s file", file);
	}
	return logFile;
}

void SetDebugLevel(unsigned int level)
{
	LOG(LOG_DBG, "Set debug level to %u", level);
	debugLevel = level;
}

bool EstablishSession(void)
{
	AwaError error;
	bool success = false;

	LOG(LOG_INFO, "Establish session with lwm2m client");

	session = AwaClientSession_New();
	if (session == NULL)
	{
		LOG(LOG_ERR, "Failed to create session");
		return false;
	}

	if ((error = AwaClientSession_SetIPCAsUDP(session, IPC_ADDRESS, IPC_PORT))
		== AwaError_Success)
	{
		if ((error = AwaClientSession_Connect(session)) == AwaError_Success)
		{
			success = true;
		}
		else
		{
			LOG(LOG_ERR, "Failed to connect session with lwm2m client\n"
				"error: %s", AwaError_ToString(error));
		}
	}
	else
	{
		LOG(LOG_ERR, "Failed to set IPC as UDP\nerror: %s", AwaError_ToString(error));
	}
	return success;
}

/**
 * @brief Save details which are required to access flow cloud or required by flow_button_gateway
 *        application. This will save value of those resources for which wantToSave member is set.
 * @return true for success otherwise false.
 */
static bool SaveFlowCloudAccessDetails()
{
	char strings[MAX_STRINGS][MAX_STR_SIZE] = {{0}};
	unsigned int i, resCount = 0;
	OBJECT_T objects[] =
	{
		flowObject,
		flowAccessObject,
		deviceObject
	};
	LOG(LOG_INFO, "Saving flow cloud access details...");

	if (!(resCount = GetResources(session, objects, ARRAY_SIZE(objects), strings)))
	{
		LOG(LOG_ERR, "Failed to get objects resource values");
		return false;
	}

	FILE *configFile = fopen(FLOW_ACCESS_CFG, "w");
	if (configFile == NULL)
	{
		LOG(LOG_ERR, "Failed to create or open "FLOW_ACCESS_CFG);
		return false;
	}
	for (i = 0; i < resCount; i++)
	{
		fprintf(configFile, "%s\n", strings[i]);
	}
	if (fclose(configFile))
	{
		LOG(LOG_ERR, "Failed to close "FLOW_ACCESS_CFG);
	}
	return true;
}

ProvisionStatus ProvisionGatewayDevice(const char *deviceName, const char *deviceType,
	int licenseeID, const char *fcap, const char *licenseeSecret)
{
	FlowSubscriptions subscriptions = {0};
	Verification verificationData;
	unsigned int timeout = SERVER_RESPONSE_TIMEOUT;
	unsigned int sleepTime = SLEEP_COUNT;
	OBJECT_T flowObjects[] =
	{
		flowObject,
		flowAccessObject,
	};

	if (deviceName == NULL || deviceType == NULL || fcap == NULL || licenseeSecret == NULL)
	{
		LOG(LOG_ERR, "Null parameters passed to %s()", __func__);
		return PROVISION_FAIL;
	}

	LOG(LOG_INFO, "Provisioning device with following details:\n"
		"\n%-15s\t = %s\n%-15s\t = %s\n%-15s\t = %d\n%-15s\t = %s\n%-15s\t = %s", "Device Name",
		deviceName, "Device Type", deviceType, "Licensee ID", licenseeID, "FCAP", fcap,
		"Licensee Secret", licenseeSecret);

	if (!DefineObjectsAtClient(session, flowObjects, ARRAY_SIZE(flowObjects)))
	{
		LOG(LOG_ERR, "Failed to define Flow objects");
		return PROVISION_FAIL;
	}

	if(IsGatewayDeviceProvisioned())
	{
		return ALREADY_PROVISIONED;
	}

	if (!PopulateFlowObject(session, deviceName, deviceType, licenseeID, fcap))
	{
		LOG(LOG_ERR, "Failed to populate flow object with device type, licensee id and fcap");
		return PROVISION_FAIL;
	}

	memset(&verificationData, 0, sizeof(Verification));
	verificationData.waitForServerResponse = true;
	verificationData.hasChallenge = false;
	verificationData.hasIterations = false;
	verificationData.verifyLicensee = false;
	verificationData.isProvisionSuccess = false;

	if (!SubscribeToFlowObjects(session, &subscriptions, &verificationData))
	{
		LOG(LOG_ERR, "Failed to subscribe flow and flow access objects");
		return PROVISION_FAIL;
	}

	LOG(LOG_INFO, "Waiting for responses from FlowCloud server...");

	while(verificationData.waitForServerResponse && --timeout)
	{
		AwaClientSession_Process(session, IPC_TIMEOUT);
		AwaClientSession_DispatchCallbacks(session);

		if (verificationData.verifyLicensee)
		{
			if (PerformFlowLicenseeVerification(session, &verificationData, licenseeSecret))
			{
				verificationData.verifyLicensee = false;
			}
		}
		sleep(1);
	}

	if (!timeout)
	{
		LOG(LOG_INFO, "No response within timeout");
	}

	// FIXME: Temporary code until status resource is removed from FlowObject so we only get one
	// change notification before canceling the subscription.
	// Right now Message IDs aren't used in IPC, so it is possible to get messages out of order,
	// causing the application to parse a response it is not expecting.
	LOG(LOG_INFO, "Waiting for any residual notifications...");
	while(sleepTime-- > 0)
	{
		AwaClientSession_Process(session, IPC_TIMEOUT);
		AwaClientSession_DispatchCallbacks(session);
		sleep(1);
	}

	// Clean up
	UnSubscribeFromFlowObjects(session, &subscriptions);
	if (verificationData.challenge.Data != NULL)
	{
		free(verificationData.challenge.Data);
	}
	if (verificationData.licenseeHash.Data != NULL)
	{
		free(verificationData.licenseeHash.Data);
	}

	if (!verificationData.isProvisionSuccess)
	{
		return PROVISION_FAIL;
	}

	if (!SaveFlowCloudAccessDetails())
	{
		LOG(LOG_ERR, "Failed to save flow cloud access details");
	}
	return PROVISION_OK;
}

bool IsGatewayDeviceProvisioned(void)
{
	LOG(LOG_INFO, "Checking whether Gateway device is provisioned");
	if (DoesObjectExist(session, Lwm2mObjectId_FlowAccess, OBJECT_INSTANCE_ID))
	{
		LOG(LOG_INFO, "Provisioned");
		return true;
	}
	else
	{
		LOG(LOG_INFO, "Not Provisioned");
		return false;
	}
}

void ReleaseSession()
{
	LOG(LOG_INFO, "Disconnecting session with lwm2m client");

	if (session == NULL)
	{
		return;
	}

	if (AwaClientSession_Disconnect(session) != AwaError_Success)
	{
		LOG(LOG_ERR, "Failed to disconnect session");
	}

	if (AwaClientSession_Free(&session) != AwaError_Success)
	{
		LOG(LOG_WARN, "Failed to free session");
	}
}
