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
 * @file device_manager.h
 * @brief Header file for exposing device manager functions.
 */

#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <stdbool.h>
#include <json.h>
#include "fdm_common.h"

//! \{
#define MAX_STR_SIZE                (64)
#define DEFAULT_PROVSIONING_TIMEOUT (30)
//! \}

/**
 * Provision status enum.
 */
typedef enum
{
    PROVISION_OK,
    PROVISION_FAIL,
    ALREADY_PROVISIONED,
}ProvisionStatus;

/**
 * @brief Provisioning details.
 */
typedef struct
{
    //! \{
    char deviceType[MAX_STR_SIZE];
    int licenseeID;
    char fcap[MAX_STR_SIZE];
    //! \}
}ProvisioningInfo;

/**
 * @brief Set log file to dump logs.
 * @param[in] file Log file.
 */
FILE* SetLogFile(const char *file);

/**
 * @brief Set debug level.
 * @param[in] level debug level.
 */
void SetDebugLevel(unsigned int level);

/**
 * @brief Establish a session, configure the IPC mechanism and connect the session to Awa LWM2M
 *        Core.
 * @return true for success otherwise false.
 */
bool EstablishSession();

/**
 * @brief Provision Gateway device to access Flow Cloud.
 * @param[in] deviceName User assigned name of device.
 * @param[in] deviceType FlowCloud registered device type.
 * @param[in] licenseeID Licensee.
 * @param[in] fcap FlowCloud Access Provisioning Code.
 * @param[in] licenseeSecret Licensee Secret.
 * @return 0 for PROVISION_OK
           1 for PROVISION_FAIL
           2 for ALREADY_PROVISIONED
 */
ProvisionStatus ProvisionGatewayDevice(const char *deviceName, const char *deviceType,
    int licenseeID, const char *fcap, const char *licenseeSecret);

/**
 * @brief Check whether gateway device is already provisioned or not.
 * @return true for success otherwise false.
 */
bool IsGatewayDeviceProvisioned();

/**
 * @brief Disconnect session from the Awa LWM2M Core and shut down the session, free up any
 *        allocated memory.
 */
void ReleaseSession();

/**
 * @brief Get list of clients registered on Awa LWM2M server.
 * @param[out] respObj Json object to be filled with list of clients.
 */
void GetClientList(json_object *respObj);

/**
 * @brief Provision a Constrained Device that has connected to the Gateway with FlowCloud
 * @param[in] clientID User assigned name of device
 * @param[in] fcap FCAP code
 * @param[in] deviceType registered device type
 * @param[in] licenseeID Licensee ID.
 * @param[in] parentID Device ID of Gateway device.
 * @return 0 for PROVISION_OK
           1 for PROVISION_FAIL
           2 for ALREADY_PROVISIONED
 */
ProvisionStatus ProvisionConstrainedDevice(const char *clientID, const char *fcap,
    const char *deviceType, int licenseeID, const char *parentID, int timeout);

/**
 * @brief Check if constrained device is provisioned or not
 * @param[in] clientID User assigned name of device
 * @return true if device is present and provisioned, false otherwise
 */
bool IsConstrainedDeviceProvisioned(const char* clientID);

#endif  /* DEVICE_MANAGER_H */
