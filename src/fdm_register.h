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
 * @file fdm_register.h
 * @brief Header file for exposing operations for registering flow and flow access objects, get and
 *        set their resources and check their existence.
 */

#ifndef FDM_REGISTER_H
#define FDM_REGISTER_H

#include "awa/client.h"
#include "awa/server.h"
#include "awa/common.h"
#include "fdm_common.h"

/**
 * @brief Define objects and their resources and register them with Awa client.
 * @param[in] session A pointer to a valid session with Awa client.
 * @param[in] objects Pointer to array of objects to be defined and registered
 * @param[in] numObjects Number of objects to define and register.
 * @return true for success otherwise false.
 */
bool DefineObjectsAtClient(AwaClientSession *session, const OBJECT_T *objects,
	unsigned int numObjects);

/**
 * @brief Define objects and their resources and register them with Awa server.
 * @param[in] session A pointer to a valid session with Awa server.
 * @param[in] objects Pointer to array of objects to be defined and registered
 * @param[in] numObjects Number of objects to define and register.
 * @return true for success otherwise false.
 */
bool DefineObjectsAtServer(AwaServerSession *session, const OBJECT_T *objects,
	unsigned int numObjects);

/**
 * @brief Check lwm2m object existence.
 * @param[in] session A pointer to a valid session.
 * @param[in] objectId Identifies the object for which the query is targeted.
 * @param[in] objectInstanceId The numerical object instance id.
 * @return true for success otherwise false.
 */
bool DoesObjectExist(AwaClientSession *session, AwaObjectID objectId,
	AwaObjectInstanceID objectInstanceId);

/**
 * @brief Populate flow object with device type, licensee id and fcap.
 * @param[in] session A pointer to a valid session.
 * @param[in] deviceName User assigned name of device.
 * @param[in] deviceType FlowCloud registered device type.
 * @param[in] licenseeID Licensee.
 * @param[in] fcap FlowCloud Access Provisioning Code.
 * @return true for success otherwise false.
 */
bool PopulateFlowObject(AwaClientSession *session, const char *deviceName,
	const char *deviceType, int64_t licenseeID, const char *fcap);

/**
 * @brief Set specified value to resource.
 * @param[in] session A pointer to a valid session.
 * @param[in] resourcePath Resource path to set value.
 * @param[in] value Resource value to set.
 * @param[in] type Resource type.
 * @return true for success otherwise false.
 */
bool SetResource(AwaClientSession *session, const char *resourcePath, void *value,
	AwaResourceType type);

/**
 * @brief Get value of specified object's resources for which wantToSave parameter is set.
 * @param[in] session A pointer to a valid session.
 * @param[in] objects Flow object's properties.
 * @param[in] numObjects Number of objects to define and register.
 * @param[out] strings Resource value strings.
 * @return resource count for success and 0 for fail.
 */
unsigned int GetResources(AwaClientSession *session, const OBJECT_T objects[],
	unsigned int numObjects, char strings[][MAX_STR_SIZE]);

#endif	/* FDM_REGISTER_H */
