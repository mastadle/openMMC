/*
 * ipmi.c
 *
 *   AFCIPMI  --
 *
 *   Copyright (C) 2015  Henrique Silva  <henrique.silva@lnls.br>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* C Standard includes */
#include "string.h"

/* Project includes */
#include "ipmb.h"
#include "ipmi.h"
#include "board_defs.h"
#include "led.h"

/* Local variables */
QueueHandle_t ipmi_rxqueue = NULL;

void IPMITask ( void * pvParameters )
{
    static ipmi_msg recv_req;
    for ( ;; ){
        xQueueReceive( ipmi_rxqueue, &recv_req, portMAX_DELAY );

        switch ( recv_req.netfn ){
        case NETFN_CHASSIS:
            ipmi_parse_chassis(&recv_req);
            break;
        case NETFN_BRIDGE:
            ipmi_parse_bridge(&recv_req);
            break;
        case NETFN_SE:
            ipmi_parse_se(&recv_req);
            break;
        case NETFN_APP:
            ipmi_parse_app(&recv_req);
            break;
        case NETFN_FIRMWARE:
            ipmi_parse_firmware(&recv_req);
            break;
        case NETFN_STORAGE:
            ipmi_parse_storage(&recv_req);
            break;
        case NETFN_TRANSPORT:
            ipmi_parse_transport(&recv_req);
            break;
        case NETFN_GRPEXT:
            ipmi_parse_picmg(&recv_req);
            break;
        case NETFN_CUSTOM:
            ipmi_parse_custom(&recv_req);
            break;
        default:
            break;
        }
        prvToggleLED(LED_GREEN);
    }
}

/* Initializes the IPMI Dispatcher:
 * -> Initializes the IPMB Layer
 * -> Registers the RX queue for incoming requests
 * -> Creates the IPMI task
 */
void ipmi_init ( void )
{
    ipmb_init();
    ipmb_register_rxqueue( &ipmi_rxqueue );
    xTaskCreate( IPMITask, (const char*)"IPMI Dispatcher", configMINIMAL_STACK_SIZE*2, ( void * ) NULL, IPMI_TASK_PRIORITY, ( TaskHandle_t * ) NULL );
}

/* Parsing Functions */

void ipmi_parse_chassis ( ipmi_msg * req )
{

}

void ipmi_parse_bridge ( ipmi_msg * req )
{

}

void ipmi_parse_se ( ipmi_msg * req )
{
    ipmi_msg resp;
    /* Initialize to 0xFF so we know if an unsupported command couldn't be parsed */
    resp.completion_code = 0xFF;
    uint8_t len = 0;

    switch (req->cmd) {
    case IPMI_SET_EVENT_RECEIVER_CMD:
        /* No data to return */
        /* TODO: We should send the sensors readings as events now */
        resp.data_len = 0;
        resp.completion_code = IPMI_CC_OK;
        break;

    default:
        break;
    }
    if ( resp.completion_code != 0xFF ) {
        ipmb_send_response(req, &resp);
    }
}

void ipmi_parse_app ( ipmi_msg * req )
{
    ipmi_msg resp;
    /* Initialize to 0xFF so we know if an unsupported command couldn't be parsed */
    resp.completion_code = 0xFF;
    uint8_t len = 0;

    switch (req->cmd) {
    case IPMI_GET_DEVICE_ID_CMD:
        /* Call a different function ? */
        resp.data[len++] = 0x0A; /* Dev ID */
        resp.data[len++] = 0x02; /* Dev Rev */
        resp.data[len++] = 0x05; /* Dev FW Rev UPPER */
        resp.data[len++] = 0x50; /* Dev FW Rev LOWER */
        resp.data[len++] = 0x02; /* IPMI Version 2.0 */
        resp.data[len++] = 0x1F; /* Dev Support */
        resp.data[len++] = 0x5A; /* Manufacturer ID LSB */
        resp.data[len++] = 0x31; /* Manufacturer ID MSB */
        resp.data[len++] = 0x00; /* ID MSB */
        resp.data[len++] = 0x01; /* Product ID LSB */
        resp.data[len++] = 0x01; /* Product ID MSB */
        resp.data_len = len;
        resp.completion_code = IPMI_CC_OK;
        break;

    default:
        break;
    }
    if ( resp.completion_code != 0xFF ) {
        ipmb_send_response(req, &resp);
    }
}

void ipmi_parse_firmware ( ipmi_msg * req )
{

}

void ipmi_parse_storage ( ipmi_msg * req )
{

}

void ipmi_parse_transport ( ipmi_msg * req )
{

}

void ipmi_parse_picmg ( ipmi_msg * req )
{
    ipmi_msg resp;
    /* Initialize to 0xFF so we know if an unsupported command couldn't be parsed */
    resp.completion_code = 0xFF;
    uint8_t len = 0;

    switch (req->cmd) {
    case IPMI_PICMG_CMD_GET_PROPERTIES:
        resp.data[len++] = IPMI_PICMG_GRP_EXT;
        resp.data[len++] = 0x23;
        resp.data[len++] = 0x01;
        resp.data[len++] = 0x00;
        resp.data_len = len;
        resp.completion_code = IPMI_CC_OK;
        break;

    case IPMI_PICMG_CMD_SET_FRU_LED_STATE:
        resp.data[len++] = IPMI_PICMG_GRP_EXT;
        resp.data_len = len;
        resp.completion_code = IPMI_CC_OK;
        break;

    default:
        break;
    }
    if ( resp.completion_code != 0xFF ) {
        ipmb_send_response(req, &resp);
    }
}

void ipmi_parse_custom ( ipmi_msg * req )
{

}
