/***************************************************************************************************
 * Copyright (c) 2015, Imagination Technologies Limited
 * All rights reserved.
 *
 * Redistribution and use of the Software in source and binary forms, with or
 * without modification, are permitted provided that the following conditions are met:
 *
 *     1. The Software (including after any modifications that you make to it) must
 *        support the FlowCloud Web Service API provided by Licensor and accessible
 *        at  http://ws-uat.flowworld.com and/or some other location(s) that we specify.
 *
 *     2. Redistributions of source code must retain the above copyright notice, this
 *        list of conditions and the following disclaimer.
 *
 *     3. Redistributions in binary form must reproduce the above copyright notice, this
 *        list of conditions and the following disclaimer in the documentation and/or
 *        other materials provided with the distribution.
 *
 *     4. Neither the name of the copyright holder nor the names of its contributors may
 *        be used to endorse or promote products derived from this Software without
 *        specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 **************************************************************************************************/

 /**
  * @file onboarding.c
  * @brief Provides 6lowpan onboarding operations.
  */

/***************************************************************************************************
 * Includes
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "onboarding.h"

/***************************************************************************************************
 * Definitions
 **************************************************************************************************/

//! @cond Doxygen_Suppress
#define MAX_STR_SIZE        (8)
#define MAX_CMD_SIZE        (64)
#define PHYNAME             "phy0"
#define PAGE                (0)
#define WPAN                "wpan"
//! @endcond

/***************************************************************************************************
 * Globals
 **************************************************************************************************/

/** wpan interface number. */
static unsigned int interfaceNum;

/***************************************************************************************************
 * Implementation
 **************************************************************************************************/

/**
 * @brief Show 6lowpan channel.
 */
void ShowChannel(void)
{
	system("iwpan phy");
}

/**
 * @brief Show 6lowpan interface.
 */
 void ShowInterface(void)
{
	printf("\n 6lowpan interface : " WPAN "%u\n", interfaceNum);
}

/**
 * @brief Show 6lowpan pan ID.
 */
void ShowPanID(void)
{
	system("iwpan dev");
}

/**
 * @brief Set 6lowpan channel.
 * @param channel 6lowpan channel number.
 */
void SetChannel(unsigned int channel)
{
	char command[MAX_CMD_SIZE];
	snprintf(command, MAX_CMD_SIZE, "iwpan phy "PHYNAME" set channel %d %u", PAGE, channel);
	system(command);
}

/**
 * @brief Set 6lowpan interface.
 * @param interface wpan interface.
 */
void SetInterface(unsigned int interface)
{
	interfaceNum = interface;
}

/**
 * @brief Set 6lowpan pan ID.
 * @param id wpan pan id for wpan interface.
 */
void SetPanID(char *id)
{
	char command[MAX_CMD_SIZE];
	snprintf(command, MAX_CMD_SIZE, "ifconfig "WPAN"%d down", interfaceNum);
	system(command);
	memset(command, 0, MAX_CMD_SIZE);
	snprintf(command, MAX_CMD_SIZE, "iwpan dev "WPAN"%d set pan_id %s", interfaceNum, id);
	system(command);
	memset(command, 0, MAX_CMD_SIZE);
	snprintf(command, MAX_CMD_SIZE, "ifconfig "WPAN"%d up", interfaceNum);
	system(command);
}
