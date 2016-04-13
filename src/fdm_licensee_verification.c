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
 * @file fdm_licensee_verification.c
 * @brief Provides licensee verification operations.
 */

/***************************************************************************************************
 * Includes
 **************************************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "awa/client.h"
#include "fdm_hmac.h"
#include "fdm_register.h"
#include "fdm_common.h"
#include "fdm_log.h"

/***************************************************************************************************
 * Methods
 **************************************************************************************************/

/**
* @brief Calculate the Licensee Hash based-on challenge.
* @param[out] hash Hash of licensee data calculated from challenge and hash iterations.
* @param[in] challenge Licensee challenge.
* @param[in] challengeLength length of challenge.
* @param[in] iterations Hash iterations.
* @param[in] licenseeSecret Licensee Secret.
* @return true for success otherwise false.
*/
static bool CalculateLicenseeHash(uint8_t hash[SHA256_HASH_LENGTH], const char *challenge,
    int challengeLength, int iterations, const char *licenseeSecret)
{
    unsigned int i;
    uint8_t key[MAX_STR_SIZE];
    int keyLength;

    if (challenge == NULL || licenseeSecret == NULL)
    {
        LOG(LOG_ERR, "Null params passed to %s()", __func__);
        return false;
    }

    LOG(LOG_DBG, "Calculating licensee hash");

    keyLength = b64Decode((char *)key, sizeof(key), licenseeSecret, strlen(licenseeSecret));

    if (keyLength == -1)
    {
        LOG(LOG_ERR, "Failed to decode a base64 encoded value");
        return false;
    }

    HmacSha256_ComputeHash(hash, (const uint8_t *) challenge, challengeLength, key, keyLength);
    for (i = 1; i < iterations; i++)
    {
        HmacSha256_ComputeHash(hash, hash, SHA256_HASH_LENGTH, key, keyLength);
    }
    return true;
}

bool PerformFlowLicenseeVerification(AwaClientSession *session, Verification *verificationData, const char *licenseeSecret)
{
    uint8_t licenseeHash[SHA256_HASH_LENGTH] = {0};
    char licenseeHashResourcePath[URL_PATH_SIZE] = {0};
    AwaError error;

    if (session == NULL || verificationData == NULL)
    {
        LOG(LOG_ERR, "Null params passed to %s()", __func__);
        return false;
    }

    LOG(LOG_INFO, "Performing flow license verification");

    // Calculate the LicenseeHash based-on challenge
    if (!CalculateLicenseeHash(licenseeHash, verificationData->challenge.Data,
        verificationData->challenge.Size, verificationData->iterations, licenseeSecret))
    {
        LOG(LOG_ERR, "Failed to calculate licensee hash");
        verificationData->waitForServerResponse = false;
        return false;
    }

    if (verificationData->licenseeHash.Data != NULL)
    {
        free(verificationData->licenseeHash.Data);
    }

    verificationData->licenseeHash.Data = malloc(SHA256_HASH_LENGTH);

    if (verificationData->licenseeHash.Data != NULL)
    {
        memcpy(verificationData->licenseeHash.Data , licenseeHash, SHA256_HASH_LENGTH);
    }

    verificationData->licenseeHash.Size = SHA256_HASH_LENGTH;

    // Write the hash to the Flow object
    if ((error = MAKE_FLOW_OBJECT_RESOURCE_PATH(licenseeHashResourcePath, FlowObjectResourceId_LicenseeHash)) == AwaError_Success)
    {
        if(!SetResource(session, licenseeHashResourcePath, (void *)&verificationData->licenseeHash, AwaResourceType_Opaque))
        {
            LOG(LOG_ERR, "Failed to set licensee hash");
            return false;
        }
    }
    else
    {
        LOG(LOG_ERR, "Failed to create licensee hash resource path\nerror: %s", AwaError_ToString(error));
        return false;
    }
    return true;
}
