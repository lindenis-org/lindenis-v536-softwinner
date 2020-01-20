/* *******************************************************************************
 * Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 * *******************************************************************************/
/**
 * @file tutk_util.cpp
 * @brief tutk 实用函数
 * @author id:826
 * @version v0.3
 * @date 2016-08-26
 */

#include "tutk_util.h"

#include <stdio.h>

void PrintErrHandling(int nErr, const char *func)
{
    fprintf(stderr, "Func: %s\n", func);
    switch (nErr)
    {
        case IOTC_ER_SERVER_NOT_RESPONSE :
            //-1 IOTC_ER_SERVER_NOT_RESPONSE
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_SERVER_NOT_RESPONSE );
            fprintf(stderr, "Master doesn't respond.\n");
            fprintf(stderr, "Please check the network wheather it could connect to the Internet.\n");
            break;
        case IOTC_ER_FAIL_RESOLVE_HOSTNAME :
            //-2 IOTC_ER_FAIL_RESOLVE_HOSTNAME
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_FAIL_RESOLVE_HOSTNAME);
            fprintf(stderr, "Can't resolve hostname.\n");
            break;
        case IOTC_ER_ALREADY_INITIALIZED :
            //-3 IOTC_ER_ALREADY_INITIALIZED
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_ALREADY_INITIALIZED);
            fprintf(stderr, "Already initialized.\n");
            break;
        case IOTC_ER_FAIL_CREATE_MUTEX :
            //-4 IOTC_ER_FAIL_CREATE_MUTEX
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_FAIL_CREATE_MUTEX);
            fprintf(stderr, "Can't create mutex.\n");
            break;
        case IOTC_ER_FAIL_CREATE_THREAD :
            //-5 IOTC_ER_FAIL_CREATE_THREAD
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_FAIL_CREATE_THREAD);
            fprintf(stderr, "Can't create thread.\n");
            break;
        case IOTC_ER_UNLICENSE :
            //-10 IOTC_ER_UNLICENSE
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_UNLICENSE);
            fprintf(stderr, "This UID is unlicense.\n");
            fprintf(stderr, "Check your UID.\n");
            break;
        case IOTC_ER_NOT_INITIALIZED :
            //-12 IOTC_ER_NOT_INITIALIZED
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_NOT_INITIALIZED);
            fprintf(stderr, "Please initialize the IOTCAPI first.\n");
            break;
        case IOTC_ER_TIMEOUT :
            //-13 IOTC_ER_TIMEOUT
            fprintf(stderr, "[Error code : %d], timeout\n", IOTC_ER_TIMEOUT);
            break;
        case IOTC_ER_INVALID_SID :
            //-14 IOTC_ER_INVALID_SID
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_INVALID_SID);
            fprintf(stderr, "This SID is invalid.\n");
            fprintf(stderr, "Please check it again.\n");
            break;
        case IOTC_ER_EXCEED_MAX_SESSION :
            //-18 IOTC_ER_EXCEED_MAX_SESSION
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_EXCEED_MAX_SESSION);
            fprintf(stderr, "[Warning]\n");
            fprintf(stderr, "The amount of session reach to the maximum.\n");
            fprintf(stderr, "It cannot be connected unless the session is released.\n");
            break;
        case IOTC_ER_CAN_NOT_FIND_DEVICE :
            //-19 IOTC_ER_CAN_NOT_FIND_DEVICE
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_CAN_NOT_FIND_DEVICE);
            fprintf(stderr, "Device didn't register on server, so we can't find device.\n");
            fprintf(stderr, "Please check the device again.\n");
            fprintf(stderr, "Retry...\n");
            break;
        case IOTC_ER_SESSION_CLOSE_BY_REMOTE :
            //-22 IOTC_ER_SESSION_CLOSE_BY_REMOTE
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_SESSION_CLOSE_BY_REMOTE);
            fprintf(stderr, "Session is closed by remote so we can't access.\n");
            fprintf(stderr, "Please close it or establish session again.\n");
            break;
        case IOTC_ER_REMOTE_TIMEOUT_DISCONNECT :
            //-23 IOTC_ER_REMOTE_TIMEOUT_DISCONNECT
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_REMOTE_TIMEOUT_DISCONNECT);
            fprintf(stderr, "We can't receive an acknowledgement character within a TIMEOUT.\n");
            fprintf(stderr, "It might that the session is disconnected by remote.\n");
            fprintf(stderr, "Please check the network wheather it is busy or not.\n");
            fprintf(stderr, "And check the device and user equipment work well.\n");
            break;
        case IOTC_ER_DEVICE_NOT_LISTENING :
            //-24 IOTC_ER_DEVICE_NOT_LISTENING
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_DEVICE_NOT_LISTENING);
            fprintf(stderr, "Device doesn't listen or the sessions of device reach to maximum.\n");
            fprintf(stderr, "Please release the session and check the device wheather it listen or not.\n");
            break;
        case IOTC_ER_CH_NOT_ON :
            //-26 IOTC_ER_CH_NOT_ON
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_CH_NOT_ON);
            fprintf(stderr, "Channel isn't on.\n");
            fprintf(stderr, "Please open it by IOTC_Session_Channel_ON() or IOTC_Session_Get_Free_Channel()\n");
            fprintf(stderr, "Retry...\n");
            break;
        case IOTC_ER_SESSION_NO_FREE_CHANNEL :
            //-31 IOTC_ER_SESSION_NO_FREE_CHANNEL
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_SESSION_NO_FREE_CHANNEL);
            fprintf(stderr, "All channels are occupied.\n");
            fprintf(stderr, "Please release some channel.\n");
            break;
        case IOTC_ER_TCP_TRAVEL_FAILED :
            //-32 IOTC_ER_TCP_TRAVEL_FAILED
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_TCP_TRAVEL_FAILED);
            fprintf(stderr, "Device can't connect to Master.\n");
            fprintf(stderr, "Don't let device use proxy.\n");
            fprintf(stderr, "Close firewall of device.\n");
            fprintf(stderr, "Or open device's TCP port 80, 443, 8080, 8000, 21047.\n");
            break;
        case IOTC_ER_TCP_CONNECT_TO_SERVER_FAILED :
            //-33 IOTC_ER_TCP_CONNECT_TO_SERVER_FAILED
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_TCP_CONNECT_TO_SERVER_FAILED);
            fprintf(stderr, "Device can't connect to server by TCP.\n");
            fprintf(stderr, "Don't let server use proxy.\n");
            fprintf(stderr, "Close firewall of server.\n");
            fprintf(stderr, "Or open server's TCP port 80, 443, 8080, 8000, 21047.\n");
            fprintf(stderr, "Retry...\n");
            break;
        case IOTC_ER_NO_PERMISSION :
            //-40 IOTC_ER_NO_PERMISSION
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_NO_PERMISSION);
            fprintf(stderr, "This UID's license doesn't support TCP.\n");
            break;
        case IOTC_ER_NETWORK_UNREACHABLE :
            //-41 IOTC_ER_NETWORK_UNREACHABLE
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_NETWORK_UNREACHABLE);
            fprintf(stderr, "Network is unreachable.\n");
            fprintf(stderr, "Please check your network.\n");
            fprintf(stderr, "Retry...\n");
            break;
        case IOTC_ER_FAIL_SETUP_RELAY :
            //-42 IOTC_ER_FAIL_SETUP_RELAY
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_FAIL_SETUP_RELAY);
            fprintf(stderr, "Client can't connect to a device via Lan, P2P, and Relay mode\n");
            break;
        case IOTC_ER_NOT_SUPPORT_RELAY :
            //-43 IOTC_ER_NOT_SUPPORT_RELAY
            fprintf(stderr, "[Error code : %d]\n", IOTC_ER_NOT_SUPPORT_RELAY);
            fprintf(stderr, "Server doesn't support UDP relay mode.\n");
            fprintf(stderr, "So client can't use UDP relay to connect to a device.\n");
            break;
        default :
            fprintf(stderr, "[Error code : %d]\n", nErr);
            break;
    }
}
