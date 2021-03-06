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
 * @file fdm_provision_constrained.h
 * @brief Provides provisioning operations.
 */

/***************************************************************************************************
 * Includes
 **************************************************************************************************/

#ifndef FDM_PROVISION_CONSTRAINED_H
#define FDM_PROVISION_CONSTRAINED_H

#include "device_manager.h"
#include "awa/server.h"

/**
 * @brief Provision a Constrained Device that has connected to the Gateway with FlowCloud
 * @param[in] clientID User assigned name of device
 * @param[in] fcap FCAP code
 * @param[in] deviceType registered device type
 * @param[in] licenseeID Licensee ID.
 * @param[in] parentID Device ID of Gateway device.
 * @param[in] timeout time to wait for notification to arrive.
 * @return 0 for PROVISION_OK
           1 for PROVISION_FAIL
           2 for ALREADY_PROVISIONED
 */
ProvisionStatus ProvisionConstrainedDevice(const char *clientID, const char *fcap,
    const char *deviceType, int licenseeID, const char *parentID, int timeout);

/**
 * @brief Poll the contents of the FlowAccessObject of the constrained device.
 *        and check if the device has been provisioned.
 * @param[in] session handle to the session with server
 * @param[in] clientID Name of the device
 * @return true if device is provisioned else false
 */
bool IsDeviceProvisioned(const AwaServerSession *session, const char *clientID);

#endif  /* FDM_PROVISION_CONSTRAINED_H */
