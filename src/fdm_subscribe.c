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
 * @file fdm_subscribe.c
 * @brief Provides subscribe, unsubscribe operations and callbacks for object change notifications.
 */

/***************************************************************************************************
 * Includes
 **************************************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "fdm_common.h"
#include "fdm_log.h"

/***************************************************************************************************
 * Methods
 **************************************************************************************************/

/**
* @brief A user-specified callback handler for a Change Subscription which will be fired on
*        AwaClientSession_DispatchCallbacks if the subscribed entity (flow object) has changed
*        since the subscription's session callbacks were last dispatched.
* @param[in] changeSet A pointer to a valid ChangeSet.
* @param[in] context A pointer to user-specified data passed to AwaClientChangeSubscription_New.
*/
static void flowObjectCallback(const AwaChangeSet *changeSet, void *context)
{
    static bool done = false;
    char licenseeChallengeResourcePath[URL_PATH_SIZE] = {0};
    char hashIterationsResourcePath[URL_PATH_SIZE] = {0};
    AwaOpaque licenseeChallenge = {0};
    Verification *verificationData = (Verification *)context;
    AwaError error;

    if (changeSet == NULL || context == NULL)
    {
        LOG(LOG_DBG, "Flow object change notification doesn't contain any data");
        return;
    }

    LOG(LOG_INFO, "Flow object updated");

    // Extract and store licensee challenge
    if ((error = MAKE_FLOW_OBJECT_RESOURCE_PATH(licenseeChallengeResourcePath, FlowObjectResourceId_LicenseeChallenge)) == AwaError_Success)
    {
        if (AwaChangeSet_ContainsPath(changeSet, licenseeChallengeResourcePath))
        {
            if ((error = AwaChangeSet_GetValueAsOpaque(changeSet, licenseeChallengeResourcePath,
                &licenseeChallenge)) == AwaError_Success && licenseeChallenge.Data != NULL && licenseeChallenge.Size > 0)
            {
                if (verificationData->challenge.Data != NULL)
                {
                    free(verificationData->challenge.Data);
                }

                verificationData->challenge.Data = malloc(licenseeChallenge.Size);

                if (verificationData->challenge.Data != NULL)
                {
                    memcpy(verificationData->challenge.Data, licenseeChallenge.Data, licenseeChallenge.Size);
                    verificationData->challenge.Size = licenseeChallenge.Size;
                    verificationData->hasChallenge = true;
                }
                else
                {
                    LOG(LOG_ERR, "Failed to allocate memory for storing licensee challenge");
                }
            }
            else
            {
                LOG(LOG_ERR, "Failed to get licensee challenge\nerror: %s", AwaError_ToString(error));
            }
        }
        else
        {
            LOG(LOG_DBG, "Flow object change notification doesn't contain licensee challenge resource");
        }
    }
    else
    {
        LOG(LOG_DBG, "Failed to create licensee challenge resource path\nerror: %s", AwaError_ToString(error));
    }

    if ((error = MAKE_FLOW_OBJECT_RESOURCE_PATH(hashIterationsResourcePath, FlowObjectResourceId_HashIterations)) == AwaError_Success)
    {
        if (AwaChangeSet_ContainsPath(changeSet, hashIterationsResourcePath))
        {
            const AwaInteger *iterationsValue = NULL;
            if (AwaChangeSet_GetValueAsIntegerPointer(changeSet, hashIterationsResourcePath, &iterationsValue) == AwaError_Success)
            {
                verificationData->iterations = *iterationsValue;
                verificationData->hasIterations = true;
            }
            else
            {
                LOG(LOG_ERR, "Failed to get hash iterations");
            }
        }
        else
        {
            LOG(LOG_DBG, "Flow object change notification doesn't contain hash iterations resource");
        }
    }
    else
    {
        LOG(LOG_DBG, "Failed to create hash iterations resource path\nerror: %s", AwaError_ToString(error));
    }

    // no errors yet, check to see if we have what we need for provisioning
    if (verificationData->waitForServerResponse && verificationData->hasChallenge && verificationData->hasIterations && !done)
    {
        verificationData->verifyLicensee = true;
        done = true;    // do this step once, because setting an object that we are observing will cause an infinite loop
    }
}

/**
* @brief Check whether ChangeSet contains specified resource and also whether that resource contains any value.
* @param[in] changeSet A pointer to a valid ChangeSet.
* @param[in] resourcePath The resource path with which to query the Get Response.
* @return true for success otherwise false.
*/
static bool HasResource(const AwaChangeSet *changeSet, const char *resourcePath)
{
    if (changeSet == NULL || resourcePath == NULL)
    {
        LOG(LOG_ERR, "Null params passed to %s()", __func__);
        return false;
    }

    return (AwaChangeSet_ContainsPath(changeSet, resourcePath) && AwaChangeSet_HasValue(changeSet, resourcePath));
}

/**
* @brief A user-specified callback handler for a Change Subscription which will be fired on
*        AwaClientSession_DispatchCallbacks if the subscribed entity (flow access object) has
*        changed since the subscription's session callbacks were last dispatched.
* @param[in] changeSet A pointer to a valid ChangeSet.
* @param[in] context A pointer to user-specified data passed to AwaClientChangeSubscription_New.
 */
static void flowAccessCallback(const AwaChangeSet *changeSet, void *context)
{
    char urlPath[URL_PATH_SIZE]         = {0};
    char keyPath[URL_PATH_SIZE]         = {0};
    char secretPath[URL_PATH_SIZE]      = {0};
    char tokenPath[URL_PATH_SIZE]       = {0};
    char tokenExpiryPath[URL_PATH_SIZE] = {0};
    Verification *verificationData = (Verification *)context;
    AwaError error;

    if (changeSet == NULL || context == NULL)
    {
        return;
    }

    LOG(LOG_INFO, "Flow access object updated");

    if ((error = MAKE_FLOW_ACCESS_OBJECT_RESOURCE_PATH(urlPath, FlowAccessResourceId_Url))
            != AwaError_Success ||
        (error = MAKE_FLOW_ACCESS_OBJECT_RESOURCE_PATH(keyPath, FlowAccessResourceId_CustomerKey))
            != AwaError_Success ||
        (error = MAKE_FLOW_ACCESS_OBJECT_RESOURCE_PATH(secretPath,
            FlowAccessResourceId_CustomerSecret)) != AwaError_Success ||
        (error = MAKE_FLOW_ACCESS_OBJECT_RESOURCE_PATH(tokenPath,
            FlowAccessResourceId_RememberMeToken)) != AwaError_Success ||
        (error = MAKE_FLOW_ACCESS_OBJECT_RESOURCE_PATH(tokenExpiryPath,
            FlowAccessResourceId_RememberMeTokenExpiry)) != AwaError_Success)
    {
        LOG(LOG_ERR, "Failed to generate resource path for all Flow access resources\nerror: %s", AwaError_ToString(error));
        return;
    }

    if (!HasResource(changeSet, urlPath) ||
        !HasResource(changeSet, keyPath) ||
        !HasResource(changeSet, secretPath) ||
        !HasResource(changeSet, tokenPath) ||
        !HasResource(changeSet, tokenExpiryPath))
    {
        LOG(LOG_ERR, "Flow access notification doesn't have all the resources");
        verificationData->waitForServerResponse = false;
        return;
    }

    LOG(LOG_INFO, "Gateway device provisioned successfully");
    verificationData->waitForServerResponse = false;
    verificationData->isProvisionSuccess = true;
}

bool SubscribeToFlowObjects(AwaClientSession *session, FlowSubscriptions *subscriptions, Verification *verificationData)
{
    char flowObjectInstancePath[URL_PATH_SIZE] = {0};
    char flowAccessInstancePath[URL_PATH_SIZE] = {0};
    const AwaClientSubscribeResponse *response;
    const AwaPathResult *flowObjectSubscribeResult;
    const AwaPathResult *flowAccessSubscribeResult;
    bool result = false;
    AwaError error;

    if(session == NULL || subscriptions == NULL || verificationData == NULL)
    {
        LOG(LOG_ERR, "Null params passed to %s()", __func__);
        return false;
    }

    LOG(LOG_INFO, "Subscribing to Flow and Flow Access object change notifications");

    if ((error = MAKE_FLOW_OBJECT_INSTANCE_PATH(flowObjectInstancePath)) != AwaError_Success ||
        (error = MAKE_FLOW_ACCESS_OBJECT_PATH(flowAccessInstancePath)) != AwaError_Success)
    {
        LOG(LOG_ERR, "Failed to generate path for %u or %u objects\nerror: %s",
            Lwm2mObjectId_FlowObject, Lwm2mObjectId_FlowAccess, AwaError_ToString(error));
        return false;
    }

    // Subscribe to Flow and Flow access object change notifications and the specified callback
    // function will be fired on AwaClientSession_DispatchCallbacks if the subscribed entity has
    // changed since the session callbacks were last dispatched.
    subscriptions->flowObjectChange = AwaClientChangeSubscription_New(flowObjectInstancePath, flowObjectCallback, verificationData);
    if (subscriptions->flowObjectChange == NULL)
    {
        LOG(LOG_ERR, "Failed to create flow subscription object");
        return false;
    }
    subscriptions->flowAccessObjectChange = AwaClientChangeSubscription_New(flowAccessInstancePath, flowAccessCallback, verificationData);
    if (subscriptions->flowAccessObjectChange == NULL)
    {
        LOG(LOG_ERR, "Failed to create flow access subscription object");
        return false;
    }

    AwaClientSubscribeOperation *operation = AwaClientSubscribeOperation_New(session);
    if (operation == NULL)
    {
        LOG(LOG_ERR, "Failed to create subscribe operation from session");
        return false;
    }

    if ((error = AwaClientSubscribeOperation_AddChangeSubscription(operation, subscriptions->flowObjectChange)) == AwaError_Success &&
        (error = AwaClientSubscribeOperation_AddChangeSubscription(operation, subscriptions->flowAccessObjectChange)) == AwaError_Success)
    {
        if ((error = AwaClientSubscribeOperation_Perform(operation, IPC_TIMEOUT)) == AwaError_Success)
        {
            response = AwaClientSubscribeOperation_GetResponse(operation);
            if (((flowObjectSubscribeResult = AwaClientSubscribeResponse_GetPathResult(response, flowObjectInstancePath)) != NULL) &&
                ((flowAccessSubscribeResult = AwaClientSubscribeResponse_GetPathResult(response, flowAccessInstancePath)) != NULL))
            {
                if ((error = AwaPathResult_GetError(flowObjectSubscribeResult)) == AwaError_Success &&
                    (error = AwaPathResult_GetError(flowAccessSubscribeResult)) == AwaError_Success)
                {
                    result = true;
                }
                else
                {
                    LOG(LOG_ERR, "Failed to get error from subscribe result for flow object or "
                        "flow access object or both\nerror: %s", AwaError_ToString(error));
                }
            }
            else
            {
                LOG(LOG_ERR, "Failed to get flow object or flow access object path in subscribe operation response");
            }
        }
        else
        {
            LOG(LOG_ERR, "Failed to perform subscribe operation\nerror: %s", AwaError_ToString(error));
        }
    }
    else
    {
        LOG(LOG_ERR, "Failed to add change subscription to subscribe operation of flow object or "
            "flow access object or both\nerror: %s", AwaError_ToString(error));
    }

    if ((error = AwaClientSubscribeOperation_Free(&operation)) != AwaError_Success)
    {
        LOG(LOG_ERR, "Failed to free subscribe operation\nerror: %s", AwaError_ToString(error));
    }
    return result;
}

void UnSubscribeFromFlowObjects(AwaClientSession *session, FlowSubscriptions *subscriptions)
{
    AwaClientSubscribeOperation *operation;
    const AwaClientSubscribeResponse *response;
    const AwaPathResult *flowSubscribeResult;
    const AwaPathResult *flowAccessSubscribeResult;
    char flowObjectInstancePath[URL_PATH_SIZE];
    char flowAccessPath[URL_PATH_SIZE];
    bool result = true;
    AwaError error;

    if(session == NULL || subscriptions == NULL)
    {
        LOG(LOG_ERR, "Null params passed to %s()", __func__);
        return;
    }

    LOG(LOG_INFO, "Unsubscribe from flow and flow access change notifications");

    if ((error = MAKE_FLOW_OBJECT_INSTANCE_PATH(flowObjectInstancePath)) != AwaError_Success ||
        (error = MAKE_FLOW_ACCESS_OBJECT_PATH(flowAccessPath)) != AwaError_Success)
    {
        LOG(LOG_ERR, "Failed to create path for flow object or flow access object\nerror: %s", AwaError_ToString(error));
        result = false;
    }

    if (result && ((operation = AwaClientSubscribeOperation_New(session)) == NULL))
    {
        LOG(LOG_ERR, "Failed to create subscribe operation from session");
        result = false;
    }

    if (result &&
        (error = AwaClientSubscribeOperation_AddCancelChangeSubscription(operation,
        subscriptions->flowObjectChange)) == AwaError_Success &&
        (error = AwaClientSubscribeOperation_AddCancelChangeSubscription(operation,
        subscriptions->flowAccessObjectChange)) == AwaError_Success)
    {
        if ((error = AwaClientSubscribeOperation_Perform(operation, IPC_TIMEOUT)) == AwaError_Success)
        {
            response = AwaClientSubscribeOperation_GetResponse(operation);
            if (((flowSubscribeResult = AwaClientSubscribeResponse_GetPathResult(response, flowObjectInstancePath)) != NULL) &&
                (flowAccessSubscribeResult = AwaClientSubscribeResponse_GetPathResult(response, flowAccessPath)) != NULL)
            {
                if ((error = AwaPathResult_GetError(flowSubscribeResult)) == AwaError_Success &&
                    (error = AwaPathResult_GetError(flowAccessSubscribeResult)) == AwaError_Success)
                {
                    LOG(LOG_DBG, "Successfully cancelled subscription to flow and flow access update events");
                }
                else
                {
                    LOG(LOG_ERR, "Failed to cancel subscription to flow object or flow access "
                        "object update events or both\nerror: %s", AwaError_ToString(error));
                }
            }
            else
            {
                LOG(LOG_ERR, "Failed to get flow object or flow acccess object path from subscribe operation response");
            }
        }
        else
        {
            LOG(LOG_ERR, "Failed to perform subscribe operation for flow object or flow access object or both\nerror: %s", AwaError_ToString(error));
        }
    }
    else
    {
        LOG(LOG_ERR, "Failed to add cancel flag to a change subscription in a specified subscribe"
            "operation for flow object or flow access object or both\nerror: %s",
            AwaError_ToString(error));
    }

    if (subscriptions->flowObjectChange != NULL)
    {
        if ((error = AwaClientChangeSubscription_Free(&subscriptions->flowObjectChange)) != AwaError_Success)
        {
            LOG(LOG_ERR, "Failed to free flow subscription object\nerror: %s", AwaError_ToString(error));
        }
    }

    if (subscriptions->flowAccessObjectChange != NULL)
    {
        if ((error = AwaClientChangeSubscription_Free(&subscriptions->flowAccessObjectChange)) != AwaError_Success)
        {
            LOG(LOG_ERR, "Failed to free flow access subscription object\nerror: %s", AwaError_ToString(error));
        }
    }

    if (operation != NULL)
    {
        if ((error = AwaClientSubscribeOperation_Free(&operation)) != AwaError_Success)
        {
            LOG(LOG_ERR, "Failed to free subscribe operation\nerror: %s", AwaError_ToString(error));
        }
    }
}
