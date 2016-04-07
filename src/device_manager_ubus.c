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
 * @file device_manager_ubus.c
 * @brief Provides provisioning operations.
 */

/***************************************************************************************************
 * Includes
 **************************************************************************************************/

#include <libubus.h>
#include <libubox/blobmsg_json.h>
#include <unistd.h>
#include <stdlib.h>

#include "device_manager.h"
#include "fdm_log.h"

/***************************************************************************************************
 * Enums
 **************************************************************************************************/

/**
 * Provision device arguments enum.
 */
enum {
    ARG_DEVICE_NAME,
    ARG_DEVICE_TYPE,
    ARG_LICENSEE_ID,
    ARG_FCAP,
    ARG_LICENSEE_SECRET,
    PROVISION_GATEWAY_DEVICE_MAX
};

enum {
    ARG_CONSTRAINED_CLIENT_ID,
    ARG_CONSTRAINED_DEVICE_TYPE,
    ARG_CONSTRAINED_LICENSEE_ID,
    ARG_CONSTRAINED_FCAP,
    ARG_CONSTRAINED_PARENT_ID,
    PROVISION_CONSTRAINED_DEVICE_MAX
};

enum {
    ARG_CLIENT_ID,
    IS_CONSTRAINED_DEVICE_PROVISIONED_MAX
};

/***************************************************************************************************
 * Typedefs
 **************************************************************************************************/

/**
 * A structure to store command line arguments.
 */
typedef struct
{
    //! \{
    const char *logFile;
    unsigned int debugLevel;
    //! \}
} CmdOpts;

/***************************************************************************************************
 * Globals
 **************************************************************************************************/

/** Provision gateway device arguments and their type. */
static const struct blobmsg_policy provisionGatewayDevicePolicy[PROVISION_GATEWAY_DEVICE_MAX] =
{
    [ARG_DEVICE_NAME] = {.name = "device_name", .type = BLOBMSG_TYPE_STRING},
    [ARG_DEVICE_TYPE] = {.name = "device_type", .type = BLOBMSG_TYPE_STRING},
    [ARG_LICENSEE_ID] = {.name = "licensee_id", .type = BLOBMSG_TYPE_INT32},
    [ARG_FCAP] = {.name = "fcap", .type = BLOBMSG_TYPE_STRING},
    [ARG_LICENSEE_SECRET] = {.name = "licensee_secret", .type = BLOBMSG_TYPE_STRING},
};

/** Provision constrained device arguments and their type. */
static const struct blobmsg_policy
    provisionConstrainedDevicePolicy[PROVISION_CONSTRAINED_DEVICE_MAX] =
{
    [ARG_CONSTRAINED_CLIENT_ID] = {.name = "client_id", .type = BLOBMSG_TYPE_STRING},
    [ARG_CONSTRAINED_DEVICE_TYPE] = {.name = "device_type", .type = BLOBMSG_TYPE_STRING},
    [ARG_CONSTRAINED_LICENSEE_ID] = {.name = "licensee_id", .type = BLOBMSG_TYPE_INT32},
    [ARG_CONSTRAINED_FCAP] = {.name = "fcap", .type = BLOBMSG_TYPE_STRING},
    [ARG_CONSTRAINED_PARENT_ID] = {.name = "parent_id", .type = BLOBMSG_TYPE_STRING}
};

/** IsConstrainedDeviceProvisioned arguments and their type. */
static const struct blobmsg_policy
    isConstrainedDeviceProvisionedPolicy[IS_CONSTRAINED_DEVICE_PROVISIONED_MAX] =
{
    [ARG_CLIENT_ID] = {.name = "client_id", .type = BLOBMSG_TYPE_STRING},
};

/***************************************************************************************************
 * Methods
 **************************************************************************************************/

//! \{
static void PrintUsage(const char *program)
{
    printf("Usage: %s [options]\n\n"
            " -l : Log filename\n"
            " -v : Debug level from 1 to 5\n"
            "      fatal(1), error(2), warning(3), info(4), debug(5)\n"
            "      default is info\n"
            " -h : Print help and exit\n\n",
            program);
}

static int ParseCommandArgs(int argc, char *argv[], CmdOpts *cmdOpts)
{
    int opt, tmp;
    opterr = 0;

    /* default values */
    cmdOpts->logFile = NULL;
    cmdOpts->debugLevel = LOG_INFO;

    while (1)
    {
        opt = getopt(argc, argv, "l:v:");
        if (opt == -1)
        {
            break;
        }

        switch (opt)
        {
            case 'l':
                cmdOpts->logFile = optarg;
                break;
            case 'v':
                tmp = strtoul(optarg, NULL, 0);
                if (tmp >= LOG_FATAL && tmp <= LOG_DBG)
                {
                    cmdOpts->debugLevel = tmp;
                }
                else
                {
                    LOG(LOG_ERR, "Invalid debug level");
                    PrintUsage(argv[0]);
                    return -1;
                }
                break;
            case 'h':
                PrintUsage(argv[0]);
                return 0;
            default:
                PrintUsage(argv[0]);
                return -1;
        }
    }
    return 1;
}

static int ProvisionGatewayDeviceHandler(struct ubus_context *ctx, struct ubus_object *obj,
    struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
    struct blob_attr *args[PROVISION_GATEWAY_DEVICE_MAX];
    struct blob_buf b = {0};

    blobmsg_parse(provisionGatewayDevicePolicy, PROVISION_GATEWAY_DEVICE_MAX, args, blob_data(msg),
        blob_len(msg));
    if (!args[ARG_DEVICE_NAME] || !args[ARG_DEVICE_TYPE] || !args[ARG_LICENSEE_ID] ||
        !args[ARG_FCAP] || !args[ARG_LICENSEE_SECRET])
        return UBUS_STATUS_INVALID_ARGUMENT;

    char *deviceName = blobmsg_get_string(args[ARG_DEVICE_NAME]);
    char *deviceType = blobmsg_get_string(args[ARG_DEVICE_TYPE]);
    char *fcap = blobmsg_get_string(args[ARG_FCAP]);
    int licenseeID = blobmsg_get_u32(args[ARG_LICENSEE_ID]);
    char *licenseeSecret = blobmsg_get_string(args[ARG_LICENSEE_SECRET]);

    if (!deviceName || !deviceType || !fcap || !licenseeSecret)
        return UBUS_STATUS_UNKNOWN_ERROR;

    ProvisionStatus status = ProvisionGatewayDevice(deviceName, deviceType, licenseeID, fcap,
        licenseeSecret);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "provision_status", status);
    ubus_send_reply(ctx, req, b.head);
    blob_buf_free(&b);

    return UBUS_STATUS_OK;
}

static int IsGatewayDeviceProvisionedHandler(struct ubus_context *ctx, struct ubus_object *obj,
    struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
    struct blob_buf b = {0};
    blob_buf_init(&b, 0);
    bool ret = IsGatewayDeviceProvisioned();
    blobmsg_add_u8(&b, "provision_status", ret);
    ubus_send_reply(ctx, req, b.head);
    blob_buf_free(&b);
    return UBUS_STATUS_OK;
}

static int GetClientListHandler(struct ubus_context *ctx, struct ubus_object *obj,
    struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
    struct blob_buf b = {0};
    blob_buf_init(&b, 0);
    json_object *respObj = json_object_new_object();
    GetClientList(respObj);
    blobmsg_add_json_from_string(&b, json_object_get_string(respObj));
    ubus_send_reply(ctx, req, b.head);
    blob_buf_free(&b);
    json_object_put(respObj);
    return UBUS_STATUS_OK;
}


static int ProvisionConstrainedDeviceHandler(struct ubus_context *ctx, struct ubus_object *obj,
    struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
    struct blob_attr *args[PROVISION_CONSTRAINED_DEVICE_MAX];
    struct blob_buf b = {0};

    blobmsg_parse(provisionConstrainedDevicePolicy, PROVISION_CONSTRAINED_DEVICE_MAX, args,
        blob_data(msg), blob_len(msg));
    if (!args[ARG_CONSTRAINED_DEVICE_TYPE] || !args[ARG_CONSTRAINED_LICENSEE_ID] ||
        !args[ARG_CONSTRAINED_CLIENT_ID] || !args[ARG_CONSTRAINED_FCAP] ||
        !args[ARG_CONSTRAINED_PARENT_ID])
        return UBUS_STATUS_INVALID_ARGUMENT;

    char *deviceType = blobmsg_get_string(args[ARG_CONSTRAINED_DEVICE_TYPE]);
    int licenseeID = blobmsg_get_u32(args[ARG_CONSTRAINED_LICENSEE_ID]);
    char *clientID = blobmsg_get_string(args[ARG_CONSTRAINED_CLIENT_ID]);
    char *fcap = blobmsg_get_string(args[ARG_CONSTRAINED_FCAP]);
    char *parentID = blobmsg_get_string(args[ARG_CONSTRAINED_PARENT_ID]);

    if (!deviceType || !clientID || !fcap || !parentID)
        return UBUS_STATUS_UNKNOWN_ERROR;

    ProvisionStatus status = ProvisionConstrainedDevice(clientID, fcap, deviceType, licenseeID,
        parentID);

    blob_buf_init(&b, 0);
    blobmsg_add_u32(&b, "status", status);
    ubus_send_reply(ctx, req, b.head);
    blob_buf_free(&b);

    return UBUS_STATUS_OK;
}

static int IsConstrainedDeviceProvisionedHandler(struct ubus_context *ctx, struct ubus_object *obj,
    struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
    struct blob_attr *args[IS_CONSTRAINED_DEVICE_PROVISIONED_MAX];
    struct blob_buf b = {0};

    blobmsg_parse(isConstrainedDeviceProvisionedPolicy, IS_CONSTRAINED_DEVICE_PROVISIONED_MAX, args,
        blob_data(msg), blob_len(msg));
    if (!args[ARG_CLIENT_ID])
        return UBUS_STATUS_INVALID_ARGUMENT;
    char *clientID = blobmsg_get_string(args[ARG_CLIENT_ID]);
    if (!clientID)
        return UBUS_STATUS_UNKNOWN_ERROR;

    bool ret = IsConstrainedDeviceProvisioned(clientID);

    blob_buf_init(&b, 0);
    blobmsg_add_u8(&b, "provision_status", ret);
    ubus_send_reply(ctx, req, b.head);
    blob_buf_free(&b);
    return UBUS_STATUS_OK;
}
//! \}

/**
* @brief Entry point of application.
*/
int main(int argc, char **argv)
{
    int ret;
    CmdOpts cmdOpts;
    FILE *logFile = NULL;
    struct ubus_context *ctx;
    const char *path = NULL;

    struct ubus_method flowDeviceManagerMethods[] =
    {
        UBUS_METHOD("provision_gateway_device", ProvisionGatewayDeviceHandler,
            provisionGatewayDevicePolicy),
        UBUS_METHOD("provision_constrained_device", ProvisionConstrainedDeviceHandler,
            provisionConstrainedDevicePolicy),
        UBUS_METHOD("is_constrained_device_provisioned", IsConstrainedDeviceProvisionedHandler,
            isConstrainedDeviceProvisionedPolicy),
        UBUS_METHOD_NOARG("is_gateway_device_provisioned", IsGatewayDeviceProvisionedHandler),
        UBUS_METHOD_NOARG("get_client_list", GetClientListHandler)
    };
    struct ubus_object_type flowDeviceManagerObjectType = \
        UBUS_OBJECT_TYPE("device_manager", flowDeviceManagerMethods);
    struct ubus_object ubusObject =
    {
        .type = &flowDeviceManagerObjectType,
        .name = flowDeviceManagerObjectType.name,
        .methods = flowDeviceManagerObjectType.methods,
        .n_methods = flowDeviceManagerObjectType.n_methods
    };

    ret = ParseCommandArgs(argc, argv, &cmdOpts);
    if (ret <= 0)
        return ret;
    if (cmdOpts.logFile)
        logFile = SetLogFile(cmdOpts.logFile);
    SetDebugLevel(cmdOpts.debugLevel);

    if (!EstablishSession())
        return -1;

    uloop_init();
    ctx = ubus_connect(path);
    if (!ctx)
    {
        LOG(LOG_ERR, "Failed to connect to ubus");
        return -1;
    }
    if (ret = ubus_add_object(ctx, &ubusObject))
    {
        LOG(LOG_ERR, "Couldn't add object : %s", ubus_strerror(ret));
        ubus_free(ctx);
        return -1;
    }
    ubus_add_uloop(ctx);
    uloop_run();

    ReleaseSession();
    if (logFile)
        fclose(logFile);

    ubus_remove_object(ctx, &ubusObject);
    ubus_free(ctx);
    uloop_done();
    return 0;
}
