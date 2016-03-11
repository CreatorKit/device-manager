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
 * @file fdm_subscribe.h
 * @brief Header file for exposing subscribe and unsubscribe operations of lwm2m objects.
 */

#ifndef FDM_SUBSCRIBE_H
#define FDM_SUBSCRIBE_H

#include "fdm_common.h"

/**
 * @brief Send a request to the Awa LWM2M Core to create a Change Subscription for flow and
 *        flow access to the specified subscriptions parameter.
 * @param[in] session A pointer to a valid session.
 * @param[in] subscriptions A pointer to valid Change Subscriptions.
 * @param[in] verificationData Licensee verification data.
 * @return true for success otherwise false.
 */
bool SubscribeToFlowObjects(AwaClientSession *session, FlowSubscriptions *subscriptions,
	Verification *verificationData);

/**
 * @brief Send a request to the Awa LWM2M Core to remove the Change Subscriptions for flow and
 *        flow access objects represented by the subscriptions parameter and shut down the change
 *        change subscriptions, freeing any allocated memory.
 * @param[in] session A pointer to a valid session.
 * @param[in] subscriptions A pointer to valid Change Subscriptions.
 */
void UnSubscribeFromFlowObjects(AwaClientSession *session, FlowSubscriptions *subscriptions);

#endif	/* FDM_SUBSCRIBE_H */
