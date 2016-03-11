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
 * @file fdm_register.c
 * @brief Provides operations to register flow and flow access objects, get and set their resources
 *        and check their existence.
 */

/***************************************************************************************************
 * Includes
 **************************************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include "awa/client.h"
#include "awa/server.h"
#include "awa/common.h"
#include "fdm_common.h"
#include "fdm_log.h"

/***************************************************************************************************
 * Macros
 **************************************************************************************************/

//! \{
#define MIN_INSTANCES     (0)
#define MAX_INSTANCES     (1)
#define OPAQUE_VALUE_SIZE (32)
#define DEVICE_ID_SIZE    (16)
//! \}

/***************************************************************************************************
 * Methods
 **************************************************************************************************/

/**
 * @brief Create AwaObjectDefinition from OBJECT_T struct.
 * @param[in] object
 * @return Definition for the object.
 */
static AwaObjectDefinition *CreateObjectDefinition(const OBJECT_T *object)
{
	unsigned int i;
	bool success = true;
	RESOURCE_T *resource;
	AwaError error;
	AwaObjectDefinition *awaObject = AwaObjectDefinition_New(object->id,
		object->name, MIN_INSTANCES, MAX_INSTANCES);

	if (awaObject == NULL)
	{
		LOG(LOG_ERR, "Failed to create %s definition", object->name);
		return NULL;
	}

	// define resources
	for (i = 0; i < object->numResources; i++)
	{
		resource = &object->resources[i];

		switch (resource->type)
		{
			case AwaResourceType_String:
				error = AwaObjectDefinition_AddResourceDefinitionAsString(awaObject,
					resource->id, resource->name, false, AwaResourceOperations_ReadWrite, "");
				break;

			case AwaResourceType_Integer:
				error = AwaObjectDefinition_AddResourceDefinitionAsInteger(awaObject,
					resource->id, resource->name, false, AwaResourceOperations_ReadWrite, 0);
				break;

			case AwaResourceType_Opaque:
				error = AwaObjectDefinition_AddResourceDefinitionAsOpaque(awaObject,
					resource->id, resource->name, false, AwaResourceOperations_ReadWrite,
					((AwaOpaque){0}));
				break;

			default:
				LOG(LOG_ERR, "Unknown resource type found, can't be added to %s definition",
					object->name);
				error = AwaError_Unspecified;
				break;
		}

		if (error != AwaError_Success)
		{
			LOG(LOG_ERR, "Failed to add %s resource to %s object definition\n"
				"error: %s", resource->name, object->name, AwaError_ToString(error));
			success = false;
			break;
		}
	}

	if (!success)
	{
		AwaObjectDefinition_Free(&awaObject);
		return NULL;
	}

	return awaObject;
}

bool DefineObjectsAtServer(AwaServerSession *session, const OBJECT_T *objects,
	unsigned int numObjects)
{
	unsigned int i;
	unsigned int definitionCount = 0;
	bool success = true;
	AwaError error = AwaError_Success;
	const OBJECT_T *object;

	if (session == NULL)
	{
		LOG(LOG_ERR, "Null session passsed to %s()", __func__);
		return false;
	}

	LOG(LOG_INFO, "Registering objects");

	AwaServerDefineOperation *handler = AwaServerDefineOperation_New(session);
	if (handler == NULL)
	{
		LOG(LOG_ERR, "Failed to create define operation for session");
		return false;
	}

	for (i = 0; i < numObjects; i++)
	{
		object = &objects[i];
		if (AwaServerSession_IsObjectDefined(session, object->id))
		{
			LOG(LOG_DBG, "%s object already defined", object->name);
			continue;
		}

		AwaObjectDefinition *awaObject = CreateObjectDefinition(object);
		if (awaObject == NULL)
		{
			LOG(LOG_ERR, "Failed to create %s definition", object->name);
			success = false;
			break;
		}
		if ((error = AwaServerDefineOperation_Add(handler, awaObject))
			!= AwaError_Success)
		{
			LOG(LOG_ERR, "Failed to add %s definition to define operation\n"
				"error: %s", object->name, AwaError_ToString(error));
			success = false;
			AwaObjectDefinition_Free(&awaObject);
			break;
		}
		definitionCount++;
		AwaObjectDefinition_Free(&awaObject);
	}

	if (success && definitionCount != 0)
	{
		if ((error = AwaServerDefineOperation_Perform(handler, IPC_TIMEOUT))
			!= AwaError_Success)
		{
			LOG(LOG_ERR, "Failed to perform define operation\n"
				"error: %s", AwaError_ToString(error));
			success = false;
		}
	}
	if ((error = AwaServerDefineOperation_Free(&handler)) != AwaError_Success)
	{
		LOG(LOG_WARN, "Failed to free define operation object\n"
			"error: %s", AwaError_ToString(error));
	}
	return success;
}


bool DefineObjectsAtClient(AwaClientSession *session, const OBJECT_T *objects,
	unsigned int numObjects)
{
	unsigned int i;
	unsigned int definitionCount = 0;
	bool success = true;
	AwaError error = AwaError_Success;
	const OBJECT_T *object;

	if (session == NULL)
	{
		LOG(LOG_ERR, "Null session passsed to %s()", __func__);
		return false;
	}

	LOG(LOG_INFO, "Registering flow objects");

	AwaClientDefineOperation *handler = AwaClientDefineOperation_New(session);
	if (handler == NULL)
	{
		LOG(LOG_ERR, "Failed to create define operation for session");
		return false;
	}

	for (i = 0; i < numObjects; i++)
	{
		object = &objects[i];

		if (AwaClientSession_IsObjectDefined(session, object->id))
		{
			LOG(LOG_DBG, "%s object already defined", object->name);
			continue;
		}

		AwaObjectDefinition *awaObject = CreateObjectDefinition(object);
		if (awaObject == NULL)
		{
			LOG(LOG_ERR, "Failed to create %s definition", object->name);
			success = false;
			break;
		}

		if ((error = AwaClientDefineOperation_Add(handler, awaObject))
			!= AwaError_Success)
		{
			LOG(LOG_ERR, "Failed to add %s definition to define operation\n"
				"error: %s", object->name, AwaError_ToString(error));
			success = false;
		}
		definitionCount++;
		AwaObjectDefinition_Free(&awaObject);
	}

	if (success && definitionCount != 0)
	{
		if ((error = AwaClientDefineOperation_Perform(handler, IPC_TIMEOUT))
			!= AwaError_Success)
		{
			LOG(LOG_ERR, "Failed to perform define operation\n"
				"error: %s", AwaError_ToString(error));
			success = false;
		}
	}
	if ((error = AwaClientDefineOperation_Free(&handler)) != AwaError_Success)
	{
		LOG(LOG_WARN, "Failed to free define operation object\n"
			"error: %s", AwaError_ToString(error));
	}
	return success;
}

/**
 * @brief Create specified resource with specified value and add it to set operation handler.
 * @param[in] handler A pointer to set operation.
 * @param[in] resourcePath The resource path requested for optional resource creation.
 * @param[in] value Resource value to set.
 * @param[in] type Resource type.
 * @return true for success otherwise false.
 */
static bool AddResourceToHandler(AwaClientSetOperation *handler, const char *resourcePath,
	void *value, AwaResourceType type)
{
	AwaInteger *intValue;
	AwaOpaque *opaqueValue;
	AwaError error;

	if (handler == NULL || resourcePath == NULL || value == NULL)
	{
		LOG(LOG_ERR, "Null params passed to %s()", __func__);
		return false;
	}

	LOG(LOG_DBG, "Add resource %s to set operation handler", resourcePath);

	if (AwaClientSetOperation_CreateOptionalResource(handler, resourcePath)
		!= AwaError_Success)
	{
		LOG(LOG_ERR, "Failed to create %s", resourcePath);
		return false;
	}

	switch (type)
	{
		case AwaResourceType_String:
			error = AwaClientSetOperation_AddValueAsCString(handler, resourcePath,
				(char *)value);
			break;

		case AwaResourceType_Integer:
			intValue = value;
			error = AwaClientSetOperation_AddValueAsInteger(handler, resourcePath, *intValue);
			break;

		case AwaResourceType_Opaque:
			opaqueValue = value;
			error = AwaClientSetOperation_AddValueAsOpaque(handler, resourcePath, *opaqueValue);
			break;

		default:
			LOG(LOG_ERR, "Unknown resource type");
			return false;
	}

	if (error != AwaError_Success)
	{
		LOG(LOG_ERR, "Failed to set value of %s\n"
			"error: %s", resourcePath, AwaError_ToString(error));
		return false;
	}
	return true;
}

bool SetResource(AwaClientSession *session, const char *resourcePath, void *value,
	AwaResourceType type)
{
	AwaError error;
	bool success = false;

	if (session == NULL || resourcePath == NULL || value == NULL)
	{
		LOG(LOG_ERR, "Null params passed to %s()", __func__);
		return false;
	}

	LOG(LOG_DBG, "Setting value of %s", resourcePath);

	AwaClientSetOperation *handler = AwaClientSetOperation_New(session);
	if (handler == NULL)
	{
		LOG(LOG_ERR, "Failed to create set operation for session");
		return false;
	}

	if (AddResourceToHandler(handler, resourcePath, value, type))
	{
		if ((error = AwaClientSetOperation_Perform(handler, IPC_TIMEOUT))
			== AwaError_Success)
		{
			success = true;
		}
		else
		{
			LOG(LOG_ERR, "Failed to perform set operation\nerror: %s",
				AwaError_ToString(error));
		}
	}

	if ((error = AwaClientSetOperation_Free(&handler)) != AwaError_Success)
	{
		LOG(LOG_WARN, "Failed to free set operation handler\nerror: %s",
			AwaError_ToString(error));
	}
	return success;
}

bool DoesObjectExist(AwaClientSession *session, AwaObjectID objectId,
	AwaObjectInstanceID objectInstanceId)
{
	const AwaClientGetResponse *response = NULL;
	char objectInstancePath[URL_PATH_SIZE] = {0};
	bool success = false;
	AwaError error;

	if (session == NULL)
	{
		LOG(LOG_ERR, "Null session passed to %s()", __func__);
		return false;
	}

	LOG(LOG_DBG, "Checking whether object %d exist or not", objectId);

	if ((error = AwaAPI_MakeObjectInstancePath(objectInstancePath, URL_PATH_SIZE, objectId,
		objectInstanceId)) != AwaError_Success)
	{
		LOG(LOG_ERR, "Failed to generate path for %d object\nerror: %s", objectId,
			AwaError_ToString(error));
		return false;
	}

	AwaClientGetOperation *handler = AwaClientGetOperation_New(session);
	if (handler == NULL)
	{
		LOG(LOG_ERR, "Failed to create get operation for session");
		return false;
	}

	if ((error = AwaClientGetOperation_AddPath(handler, objectInstancePath))
		== AwaError_Success)
	{
		if ((error = AwaClientGetOperation_Perform(handler, IPC_TIMEOUT))
			== AwaError_Success)
		{
			response = AwaClientGetOperation_GetResponse(handler);
			if (response != NULL)
			{
				if (AwaClientGetResponse_ContainsPath(response, objectInstancePath))
				{
					success = true;
				}
				else
				{
					LOG(LOG_DBG, "%d object doesn't exist", objectId);
				}
			}
			else
			{
				LOG(LOG_ERR, "Failed to get response from get operation handler");
			}
		}
		else
		{
			LOG(LOG_ERR, "Failed to perform get operation\nerror: %s", AwaError_ToString(error));
		}
	}
	else
	{
		LOG(LOG_ERR, "Failed to add %d object path to get operation handler\nerror: %s", objectId,
			AwaError_ToString(error));
	}

	if ((error = AwaClientGetOperation_Free(&handler)) != AwaError_Success)
	{
		LOG(LOG_WARN, "Failed to free get operation handler\nerror: %s",
			AwaError_ToString(error));
	}
	return success;
}

bool PopulateFlowObject(AwaClientSession *session, const char *deviceName,
	const char *deviceType, int64_t licenseeID, const char *fcap)
{
	char objectInstancePath[URL_PATH_SIZE] = {0};
	char deviceNameResourcePath[URL_PATH_SIZE] = {0};
	char deviceTypeResourcePath[URL_PATH_SIZE] = {0};
	char licenseeIdResourcePath[URL_PATH_SIZE] = {0};
	char fcapResourcePath[URL_PATH_SIZE] = {0};
	bool status = false;
	AwaError error;

	if (session == NULL || deviceName == NULL || deviceType == NULL || fcap == NULL)
	{
		LOG(LOG_ERR, "Null parameters passed to %s()", __func__);
		return false;
	}

	LOG(LOG_INFO, "Populate flow object with device type, licensee id and fcap");

	//Generate all object and resource paths
	if ((error = MAKE_FLOW_OBJECT_INSTANCE_PATH(objectInstancePath) != AwaError_Success  ||
		(error = MAKE_FLOW_OBJECT_RESOURCE_PATH(deviceNameResourcePath,
			FlowObjectResourceId_DeviceName)) != AwaError_Success ||
		(error = MAKE_FLOW_OBJECT_RESOURCE_PATH(deviceTypeResourcePath,
			FlowObjectResourceId_DeviceType)) != AwaError_Success ||
		(error = MAKE_FLOW_OBJECT_RESOURCE_PATH(licenseeIdResourcePath,
			FlowObjectResourceId_LicenseeId)) != AwaError_Success ||
		(error = MAKE_FLOW_OBJECT_RESOURCE_PATH(fcapResourcePath, FlowObjectResourceId_Fcap))
			!= AwaError_Success))
	{
		LOG(LOG_ERR, "Failed to generate all object and resource paths\nerror: %s",
			AwaError_ToString(error));
		return false;
	}

	AwaClientSetOperation *handler = AwaClientSetOperation_New(session);
	if (handler == NULL)
	{
		LOG(LOG_ERR, "Failed to create set operation for session");
		return false;
	}

	if (!DoesObjectExist(session, Lwm2mObjectId_FlowObject, OBJECT_INSTANCE_ID))
	{
		LOG(LOG_DBG, "Flow object instance doesn't exist, so create it");
		if ((error = AwaClientSetOperation_CreateObjectInstance(handler, objectInstancePath))
			!= AwaError_Success)
		{
			LOG(LOG_ERR, "Failed to create flow object instance\nerror: %s",
				AwaError_ToString(error));
		}
	}
	else
	{
		LOG(LOG_DBG, "Flow object instance exist");
	}

	if (AddResourceToHandler(handler, deviceNameResourcePath, (void *)deviceName,
		AwaResourceType_String) &&
		AddResourceToHandler(handler, deviceTypeResourcePath, (void *)deviceType,
		AwaResourceType_String) &&
		AddResourceToHandler(handler, fcapResourcePath, (void *)fcap, AwaResourceType_String) &&
		AddResourceToHandler(handler, licenseeIdResourcePath, &licenseeID,
		AwaResourceType_Integer))
	{
		if ((error = AwaClientSetOperation_Perform(handler, IPC_TIMEOUT))
			== AwaError_Success)
		{
			status = true;
		}
		else
		{
			LOG(LOG_ERR, "Failed to perform set operation\nerror: %s", AwaError_ToString(error));
		}
	}
	else
	{
		LOG(LOG_ERR, "Failed to add flow object's resources(device name, device type, licensee id, "
			"fcap) to set operation handler");
	}

	if ((error = AwaClientSetOperation_Free(&handler)) != AwaError_Success)
	{
		LOG(LOG_WARN, "Failed to free set operation handler\nerror: %s",
			AwaError_ToString(error));
	}
	return status;
}

unsigned int GetResources(AwaClientSession *session, const OBJECT_T objects[],
	unsigned int numObjects, char strings[][MAX_STR_SIZE])
{
	const char *value;
	const AwaInteger *intValue;
	AwaOpaque opaqueValue;
	unsigned int i, j, k, resCount = 0;
	unsigned char *opaqueData;
	char *offset;
	const OBJECT_T *object;
	RESOURCE_T *resource;
	bool success = true;
	AwaError error;
	char objectInstancePath[URL_PATH_SIZE];
	char resourcePath[URL_PATH_SIZE];
	const AwaClientGetResponse *response;
	AwaClientGetOperation *operation;

	if ((operation = AwaClientGetOperation_New(session)) == NULL)
	{
		LOG(LOG_ERR, "Failed to create get operation from session");
		return 0;
	}

	for (i = 0; i < numObjects; i++)
	{
		memset(objectInstancePath, 0, URL_PATH_SIZE);
		memset(resourcePath, 0, URL_PATH_SIZE);

		object = &objects[i];

		if ((error = AwaAPI_MakeObjectInstancePath(objectInstancePath, URL_PATH_SIZE,
			object->id, OBJECT_INSTANCE_ID)) != AwaError_Success)
		{
			LOG(LOG_ERR, "Failed to create path for %s object\nerror: %s", object->name,
				AwaError_ToString(error));
			success = false;
			break;
		}
		if ((error = AwaClientGetOperation_AddPath(operation, objectInstancePath))
			!= AwaError_Success)
		{
			LOG(LOG_ERR, "Failed to add %s object path to get operation\nerror: %s", object->name,
				AwaError_ToString(error));
			success = false;
			break;
		}
		if ((error = AwaClientGetOperation_Perform(operation, IPC_TIMEOUT))
			!= AwaError_Success)
		{
			LOG(LOG_ERR, "Failed to perform get operation for %s object\nerror: %s", object->name,
				AwaError_ToString(error));
			success = false;
			break;
		}
		if ((response = AwaClientGetOperation_GetResponse(operation)) == NULL)
		{
			LOG(LOG_ERR, "Failed to get response from get operation for %s object", object->name);
			success = false;
			break;
		}
		if (!AwaClientGetResponse_ContainsPath(response, objectInstancePath))
		{
			LOG(LOG_ERR, "Response doesn't contain %s object path", object->name);
			success = false;
			break;
		}

		for (j = 0; j < object->numResources; j++)
		{
			resource = &object->resources[j];
			if (!resource->wantToSave)
			{
				continue;
			}
			if ((error = AwaAPI_MakeResourcePath(resourcePath, URL_PATH_SIZE, object->id,
				OBJECT_INSTANCE_ID, resource->id)) != AwaError_Success)
			{
				LOG(LOG_ERR, "Failed to create path for %s resource\nerror: %s", resource->name,
					AwaError_ToString(error));
				success = false;
				break;
			}
			if (!AwaClientGetResponse_HasValue(response, resourcePath))
			{
				LOG(LOG_ERR, "Get operation response doesn't contain path of %s resource",
					resource->name);
				success = false;
				break;
			}

			switch (resource->type)
			{
				case AwaResourceType_String:
					if ((error = AwaClientGetResponse_GetValueAsCStringPointer(response,
						resourcePath, &value)) == AwaError_Success)
					{
						sprintf(strings[resCount++], "%s=\"%s\"", resource->name, value);
					}
					break;

				case AwaResourceType_Integer:
					if ((error = AwaClientGetResponse_GetValueAsIntegerPointer(response,
						resourcePath, &intValue)) == AwaError_Success)
					{
						sprintf(strings[resCount++], "%s=\"%" PRId64 "\"", resource->name,
							*intValue);
					}
					break;

				case AwaResourceType_Opaque:
					if ((error = AwaClientGetResponse_GetValueAsOpaque(response, resourcePath,
						&opaqueValue)) == AwaError_Success)
					{
						opaqueData = (unsigned char *)opaqueValue.Data;
						offset = strings[resCount];
						offset += sprintf(strings[resCount], "%s=\"", resource->name);
						for (k = 0; k < DEVICE_ID_SIZE; k++)
						{
							offset += sprintf(offset, "%02X ", *opaqueData++);
						}
						sprintf(offset, "\"");
						resCount++;
					}
					break;

				default:
					LOG(LOG_ERR, "Unknown resource type");
					error = AwaError_Unspecified;
					break;
			}
			if (error != AwaError_Success)
			{
				LOG(LOG_ERR, "Failed to get %s resource value from response\nerror: %s",
					resource->name, AwaError_ToString(error));
				success = false;
				break;
			}
		}
		if (!success)
		{
			break;
		}
	}

	if ((error = AwaClientGetOperation_Free(&operation)) != AwaError_Success)
	{
		LOG(LOG_WARN, "Failed to free get operation\nerror: %s", AwaError_ToString(error));
	}
	return (success ? resCount : 0);
}
