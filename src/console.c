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
 * @file console.c
 * @brief Starts console prompt to take commands from user and parse them.
 */

/***************************************************************************************************
 * Includes
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "flow_device_manager.h"
#include "onboarding.h"
#include "log.h"
#include "utils.h"

/***************************************************************************************************
 * Definitions
 **************************************************************************************************/

//! @cond Doxygen_Suppress
#define DECIMAL                 (10)
#define MAX_STR_SIZE            (64)
#define ARRAY_SIZE(arr)         (sizeof(arr)/sizeof(arr[0]))

/***************************************************************************************************
 * Typedef
 **************************************************************************************************/

typedef enum
{
	START_PROVISION,
	SHOW_PROVISION_STATUS,
	SHOW_ACCESS_DETAILS,
	SHOW_INPUT_DETAILS,
	SET_LICENSEE_SECRET,
	SET_LICENSEE_ID,
	SET_IPC_PORT,
	SET_FCAP,
	SET_DEVICE_TYPE,

	SHOW_PAN_ID,
	SHOW_INTERFACE,
	SHOW_CHANNEL,
	SET_PAN_ID,
	SET_INTERFACE,
	SET_CHANNEL,

	HELP,
	EXIT
}Command;
//! @endcond

/**
 * A structure to contain command information.
 */
typedef struct {
	/*@{*/
	const char* name; /**< command name*/
	Command ID; /**< command ID */
	const char* doc; /**< command description */
	/*@}*/
}CmdTable;

/**
 * A structure to contain input details.
 */
typedef struct {
	/*@{*/
	int ipcPort; /**< ipc port */
	char deviceType[MAX_STR_SIZE]; /**< device type */
	int licenseeID; /**< licensee ID */
	char fcap[MAX_STR_SIZE]; /**< fcap token */
	char licenseeSecret[MAX_STR_SIZE]; /**< licensee secret */
	/*@}*/
}ProvisionData;

/***************************************************************************************************
 * Globals
 **************************************************************************************************/

/** Initialise provision data. */
ProvisionData provisionData =
{
	.ipcPort = 12345,
	.deviceType = "FlowGateway",
	.licenseeID = 17,
	.fcap = {0},
	.licenseeSecret = {0},
};

/** Initialise command table. */
static CmdTable DeviceManagerCmdTable[] =
{
/** Provisioning commands. */
	{ "start provision",		START_PROVISION,		"Start provision" 									},
	{ "show provision_status",	SHOW_PROVISION_STATUS,	"Show provision status"								},
	{ "show input_details",		SHOW_INPUT_DETAILS,		"Show input details or default (ipc port, device type, licensee ID, fcap, licensee secret entered by user" },
	{ "show access_details",	SHOW_ACCESS_DETAILS,	"Show Flow Access details"							},
	{ "set licensee_secret",	SET_LICENSEE_SECRET,	"Set licensee secret (ex: set licensee_secret, getATT)"	},
	{ "set licensee_id",		SET_LICENSEE_ID,		"Set licensee ID (ex: set licensee_id, 2)"			},
	{ "set ipc_port",			SET_IPC_PORT,			"Set IPC port (ex: set ipc_port, 34567)"			},
	{ "set fcap",				SET_FCAP,				"Set fcap (ex: set fcap, abcd)"						},
	{ "set device_type",		SET_DEVICE_TYPE,		"Set device type (ex: set device_type, FlowGateway)"},

/** Onboarding commands for 6lowpan interface. */
	{ "show pan_id",			SHOW_PAN_ID,			"Show 6lowpan pan id"								},
	{ "show interface",			SHOW_INTERFACE,			"Show 6lowpan interface"							},
	{ "show channel",			SHOW_CHANNEL,			"Show 6lowpan channel"								},
	{ "set pan_id",				SET_PAN_ID,				"Set 6lowpan pan id (ex: set pan_id, 0xbeef)"		},
	{ "set interface",			SET_INTERFACE,			"Set 6lowpan interface (ex: set interface, 1)"		},
	{ "set channel",			SET_CHANNEL,			"Set 6lowpan channel (ex: set channel, 13)"			},

/** General commands. */
	{ "help",					HELP,					"Show available commands"							},
	{ "exit",					EXIT,					"Exits the interpreter"								},
};

/***************************************************************************************************
 * Implementation
 **************************************************************************************************/

/**
 * @brief Handler to catch ctrl+c.
 * @param sig signal ID.
 */
static void INThandler(int sig)
{
	StopWaitingForServerResponse();
	signal(sig, SIG_IGN);
}

/**
 * @brief Process command entered by user.
 * @param cmd command id.
 * @param arg command argument.
 */
static void ProcessCommand(Command cmd, char *arg)
{
	unsigned int i;
	ProvisionStatus status;
	char *endptr;
	unsigned long int val;
	CmdTable command;

	switch(cmd)
	{
		case START_PROVISION:
			if (strlen(provisionData.fcap) == 0 ||
				strlen(provisionData.licenseeSecret) == 0)
			{
				LOG(LOG_INFO, "fcap or licensee secret is not set");
			}
			else
			{
				status = ProvisionDevice(provisionData.deviceType,
													provisionData.licenseeID,
													provisionData.fcap,
													provisionData.licenseeSecret);
				switch (status)
				{
					case PROVISION_OK:
						LOG(LOG_INFO, "Provision OK");
						break;

					case ALREADY_PROVISIONED:
						LOG(LOG_INFO, "Device already provisioned");
						break;

					case PROVISION_FAIL:
						LOG(LOG_INFO, "Provision FAIL");
						break;

					default:
						LOG(LOG_ERR, "Unknown return value");
						break;
				}
			}
			break;

		case SHOW_PROVISION_STATUS:
			if (IsDeviceProvisioned())
			{
				LOG(LOG_INFO, "Device is provisioned\n");
			}
			else
			{
				LOG(LOG_INFO, "Device is not provisioned\n");
			}
			break;

		case SHOW_ACCESS_DETAILS:
			ShowFlowAccessDetails();
			break;

		case SHOW_INPUT_DETAILS:
			LOG(LOG_INFO, "Details entered by user or default:\n"
				"\nIPC port = %d\nDevice type = %s\nLicensee ID = %d\nFCAP = %s"
				"\nLicensee secret = %s", provisionData.ipcPort, provisionData.deviceType,
				provisionData.licenseeID, provisionData.fcap, provisionData.licenseeSecret);
			break;

		case SET_LICENSEE_SECRET:
			if (arg)
			{
				snprintf(provisionData.licenseeSecret, sizeof(provisionData.licenseeSecret), "%s",
							arg);
			}
			else
			{
				LOG(LOG_INFO, "licensee ID not given, Please check command");
			}
			break;

		case SET_LICENSEE_ID:
			if (arg)
			{
				val = strtoul(arg, &endptr, DECIMAL);
				if (endptr == arg)
				{
					LOG(LOG_ERR, "No digits were found in argument");
					break;
				}
				provisionData.licenseeID = val;
			}
			else
			{
				LOG(LOG_INFO, "licensee ID not given, Please check command");
			}
			break;

		case SET_IPC_PORT:
			if (arg)
			{
				val = strtoul(arg, &endptr, DECIMAL);
				if (endptr == arg)
				{
					LOG(LOG_ERR, "No digits were found in argument");
					break;
				}
				provisionData.ipcPort = val;
			}
			else
			{
				LOG(LOG_INFO, "port not given, Please check command");
			}
			break;

		case SET_FCAP:
			if (arg)
			{
				snprintf(provisionData.fcap, sizeof(provisionData.fcap), "%s", arg);
			}
			else
			{
				LOG(LOG_INFO, "fcap token not given, Please check command");
			}
			break;

		case SET_DEVICE_TYPE:
			if (arg)
			{
				snprintf(provisionData.deviceType, sizeof(provisionData.deviceType), "%s", arg);
			}
			else
			{
				LOG(LOG_INFO, "device_type not given, Please check command");
			}
			break;

		case SHOW_PAN_ID:
			ShowPanID();
			break;

		case SHOW_INTERFACE:
			ShowInterface();
			break;

		case SHOW_CHANNEL:
			ShowChannel();
			break;

		case SET_PAN_ID:
			if (arg)
			{
				SetPanID(arg);
			}
			else
			{
				LOG(LOG_INFO, "pan ID not given, Please check command");
			}
			break;

		case SET_INTERFACE:
			if (arg)
			{
				val = strtoul(arg, &endptr, DECIMAL);
				if (endptr == arg)
				{
					LOG(LOG_ERR, "No digits were found in argument");
					break;
				}
				SetInterface(val);
			}
			else
			{
				LOG(LOG_INFO, "interface not given, Please check command");
			}
			break;

		case SET_CHANNEL:
			if (arg)
			{
				val = strtoul(arg, &endptr, DECIMAL);
				if (endptr == arg)
				{
					LOG(LOG_ERR, "No digits were found in argument");
					break;
				}
				SetChannel(val);
			}
			else
			{
				LOG(LOG_INFO, "channel number not given, Please check command");
			}
			break;

		case HELP:
			LOG(LOG_INFO, "Commands:");
			for (i = 0; i < ARRAY_SIZE(DeviceManagerCmdTable); i++)
			{
				command = DeviceManagerCmdTable[i];
				LOG(LOG_INFO, "%-25s\t- %s", command.name, command.doc);
			}
			break;

		case EXIT:
			DestroyFlowDM();
			LOG(LOG_INFO, "Exiting Interactive Mode\n\n");
			exit(0);
			break;

		default:
			LOG(LOG_ERR, "Control should not reach here");
			break;
	}
}
/**
 * @brief Parse command from input and compare it with available commands.
 * @param cmd command to parse.
 */
static void parseCommand(char *cmd)
{
	unsigned int i;
	char *value;
	char arg[MAX_STR_SIZE];
	CmdTable command;
	const char *tok = strtok(cmd, "\n(,);");

	if (tok)
	{
		for (i = 0; i < ARRAY_SIZE(DeviceManagerCmdTable); i++)
		{
			command = DeviceManagerCmdTable[i];
			if (!strcmp(tok, command.name))
			{
				value = strtok(NULL, "\n");
				if (value)
				{
					CopyStringWithoutSpace(arg, value);
				}
				ProcessCommand(command.ID, arg);
				return;
			}
		}
		LOG(LOG_INFO, "Command Not Found");
	}
}

/**
 * @brief Start console.
 */
void StartConsole(void)
{
	LOG(LOG_INFO, "Entering Interactive Mode");
	if (InitialiseFlowDM(provisionData.ipcPort) == false)
	{
		exit(1);
	}
	signal(SIGINT, INThandler);
	while (1)
	{
		char cmd[MAX_STR_SIZE] = {0};
		unsigned int i = 0;

		LOG(LOG_INFO, "> ");

		while (1)
		{
			char c;
			c = fgetc(stdin);

			if (c != EOF)
			{
				if ((c != '\n') && (c != '\r') && (i < MAX_STR_SIZE - 1))
				{
					cmd[i] = c;
					i++;
				}
				else
				{
					break;
				}
			}
		}
		parseCommand(cmd);
	}
}
