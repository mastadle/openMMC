/*
 *   openMMC -- Open Source modular IPM Controller firmware
 *
 *   Copyright (C) 2021  Wojciech Ruclo <wojciech.ruclo@creotech.pl>
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
 *
 *   @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 */

/* FreeRTOS Includes */
#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "cli.h"
#include "pin_mapping.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"

/* Project Includes */
#include "cli_commands.h"

#include "port.h"
#include "ipmi.h"
#include "task_priorities.h"
#ifdef MODULE_PAYLOAD
#include "payload.h"
#endif
#ifdef MODULE_HOTSWAP
#include "hotswap.h"
#endif
#include "utils.h"
#include "fru.h"
#include "led.h"
#include "GitSHA1.h"
#include "i2c_mapping.h"

/* C Standard includes */
#include <stdlib.h>
#include <ctype.h>

static BaseType_t GpioReadCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    char *name;
    uint8_t value, port, pin;
    BaseType_t lParameterStringLength;

    // Get GPIO name
    name = (char *) FreeRTOS_CLIGetParameter((const char *) pcCommandString, 1, &lParameterStringLength);

    // Convert GPIO name to lower-case
    for (int i = 0; name[i]; i++) {
        name[i] = tolower((int) name[i]);
    }

    // Power supply
    if (strcmp(name, "dcdc_pgood") == 0) {
        value = gpio_read_pin(PIN_PORT(GPIO_DCDC_PGOOD), PIN_NUMBER(GPIO_DCDC_PGOOD));
    }

    // Sensors
    else if (strcmp(name, "overtemp_n") == 0) {
        value = gpio_read_pin(PIN_PORT(GPIO_OVERTEMPn), PIN_NUMBER(GPIO_OVERTEMPn));
    }


    // FPGA
    else if (strcmp(name, "fpga_done_b") == 0) {
        value = gpio_read_pin(PIN_PORT(GPIO_FPGA_DONE_B), PIN_NUMBER(GPIO_FPGA_DONE_B));
    }
    else if (strcmp(name, "fpga_init_b") == 0) {
        value = gpio_read_pin(PIN_PORT(GPIO_FPGA_INITB), PIN_NUMBER(GPIO_FPGA_INITB));
    }
    else if (strcmp(name, "fpga_reset") == 0) {
        value = gpio_read_pin(PIN_PORT(GPIO_FPGA_RESET), PIN_NUMBER(GPIO_FPGA_RESET));
    }

    // Front panel button
    else if (strcmp(name, "front_button") == 0) {
        value = gpio_read_pin(PIN_PORT(GPIO_FRONT_BUTTON), PIN_NUMBER(GPIO_FRONT_BUTTON));
    }

    // Hot swap handle
    else if (strcmp(name, "hot_swap_handle") == 0) {
        value = gpio_read_pin(PIN_PORT(GPIO_HOT_SWAP_HANDLE), PIN_NUMBER(GPIO_HOT_SWAP_HANDLE));
    }


    // AMC
    else if (strcmp(name, "amc_ga") == 0) {
        value = (gpio_read_pin(PIN_PORT(GPIO_GA2), PIN_NUMBER(GPIO_GA2)) & 0x1) << 0x2 |
                (gpio_read_pin(PIN_PORT(GPIO_GA1), PIN_NUMBER(GPIO_GA1)) & 0x1) << 0x1 |
                (gpio_read_pin(PIN_PORT(GPIO_GA0), PIN_NUMBER(GPIO_GA0)) & 0x1);
    }
    else if (strcmp(name, "amc_en") == 0) {
        value = gpio_read_pin(PIN_PORT(GPIO_MMC_ENABLE), PIN_NUMBER(GPIO_MMC_ENABLE));
    }

    // RTM
    else if (strcmp(name, "rtm_ps") == 0) {
        value = gpio_read_pin(PIN_PORT(GPIO_RTM_PS), PIN_NUMBER(GPIO_RTM_PS));
    }

    // FMC
    else if (strcmp(name, "fmc1_prsnt_m2c") == 0) {
        value = gpio_read_pin(PIN_PORT(GPIO_FMC1_PRSNT_M2C), PIN_NUMBER(GPIO_FMC1_PRSNT_M2C));
    }
    else if (strcmp(name, "fmc2_prsnt_m2c") == 0) {
        value = gpio_read_pin(PIN_PORT(GPIO_FMC2_PRSNT_M2C), PIN_NUMBER(GPIO_FMC2_PRSNT_M2C));
    }
    else if (strcmp(name, "fmc1_pg_m2c") == 0) {
        value = gpio_read_pin(PIN_PORT(GPIO_FMC1_PG_M2C), PIN_NUMBER(GPIO_FMC1_PG_M2C));
    }
    else if (strcmp(name, "fmc2_pg_m2c") == 0) {
        value = gpio_read_pin(PIN_PORT(GPIO_FMC2_PG_M2C), PIN_NUMBER(GPIO_FMC2_PG_M2C));
    }

    else {
        port = atoi(FreeRTOS_CLIGetParameter((const char *) pcCommandString, 1, &lParameterStringLength));
        pin = atoi(FreeRTOS_CLIGetParameter((const char *) pcCommandString, 2, &lParameterStringLength));
        if (!lParameterStringLength) {
            sprintf((char *) pcWriteBuffer, "GPIO name <%s> not recognized.", name);
            return pdFALSE;
        }
        value = gpio_read_pin(port, pin);
    }

    // Write GPIO value to the output buffer
    itoa(value, (char *) pcWriteBuffer, 10);

    return pdFALSE;
}

static BaseType_t GpioWriteCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    char name[MAX_INPUT_LENGTH];
    uint8_t value;
    BaseType_t param_1_length, param_2_length;

    // Get GPIO value
    value = atoi(FreeRTOS_CLIGetParameter(pcCommandString, 2, &param_2_length));

    // Get GPIO name
    strcpy(name, FreeRTOS_CLIGetParameter(pcCommandString, 1, &param_1_length));

    // Convert GPIO name to lower-case
    for (int i = 0; name[i]; i++) {
        name[i] = tolower((int) name[i]);
    }

    // Clock MUX
    if (strcmp(name, "adn_resetn") == 0) {
        gpio_set_pin_state(PIN_PORT(GPIO_ADN_RESETN), PIN_NUMBER(GPIO_ADN_RESETN), value & 0x1);
    }
    else if (strcmp(name, "adn_update") == 0) {
        gpio_set_pin_state(PIN_PORT(GPIO_ADN_UPDATE), PIN_NUMBER(GPIO_ADN_UPDATE), value & 0x1);
    }

    // LEDs
    else if (strcmp(name, "led_blue") == 0) {
        gpio_set_pin_state(PIN_PORT(GPIO_LEDBLUE), PIN_NUMBER(GPIO_LEDBLUE), value & 0x1);
    }
    else if (strcmp(name, "led_green") == 0) {
        gpio_set_pin_state(PIN_PORT(GPIO_LEDGREEN), PIN_NUMBER(GPIO_LEDGREEN), value & 0x1);
    }
    else if (strcmp(name, "led_red") == 0) {
        gpio_set_pin_state(PIN_PORT(GPIO_LEDRED), PIN_NUMBER(GPIO_LEDRED), value & 0x1);
    }

    // FPGA
    else if (strcmp(name, "fpga_program_b") == 0) {
        gpio_set_pin_state(PIN_PORT(GPIO_FPGA_PROGRAM_B), PIN_NUMBER(GPIO_FPGA_PROGRAM_B), value & 0x1);
    }
    else if (strcmp(name, "fpga_reset_n") == 0) {
        gpio_set_pin_state(PIN_PORT(GPIO_FPGA_RESET), PIN_NUMBER(GPIO_FPGA_RESET), value & 0x1);
    }

    // FMC
    else if (strcmp(name, "fmc1_pg_c2m") == 0) {
        gpio_set_pin_state(PIN_PORT(GPIO_FMC1_PG_C2M), PIN_NUMBER(GPIO_FMC1_PG_C2M), value & 0x1);
    }
    else if (strcmp(name, "fmc2_pg_c2m") == 0) {
        gpio_set_pin_state(PIN_PORT(GPIO_FMC2_PG_C2M), PIN_NUMBER(GPIO_FMC2_PG_C2M), value & 0x1);
    }

    else {
        sprintf(pcWriteBuffer, "GPIO name <%s> not recognized.", name);
        return pdFALSE;
    }

    strcpy(pcWriteBuffer, "OK");

    return pdFALSE;
}

static BaseType_t ReadMagicValueCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    strcpy(pcWriteBuffer, "4D41474943");
    return pdFALSE;
}

static BaseType_t ReadVersionCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    strcpy(pcWriteBuffer, g_GIT_TAG);
    return pdFALSE;
}

static BaseType_t ResetCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    NVIC_SystemReset();
    return pdFALSE;
}

static BaseType_t I2cReadCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    BaseType_t param_1_length, param_2_length, param_3_length, param_4_length;
    uint8_t i2c_interf, i2c_addr, i2c_mux_bus, cmd, read_len, received_len = 0;
    uint8_t read_data[256] = { 0xFF };

    /* Ensure there is always a null terminator after each character written. */
    memset(pcWriteBuffer, 0x00, xWriteBufferLen);

    i2c_mux_bus = atoi(FreeRTOS_CLIGetParameter((const char *) pcCommandString, 1, &param_1_length));
    i2c_addr = atoi(FreeRTOS_CLIGetParameter((const char *) pcCommandString, 2, &param_2_length));
    cmd = atoi(FreeRTOS_CLIGetParameter((const char *) pcCommandString, 3, &param_3_length));
    read_len = atoi(FreeRTOS_CLIGetParameter((const char *) pcCommandString, 4, &param_4_length));

    uint8_t bus_id = 0;
    for (uint8_t i = 0; i < I2C_BUS_CNT; i++) {
        if (i2c_bus_map[i].mux_bus == i2c_mux_bus) {
            bus_id = i;
            break;
        }
    }

    if (i2c_take_by_busid(bus_id, &i2c_interf, (TickType_t) 10)) {
        received_len = xI2CMasterWriteRead(i2c_interf, i2c_addr, cmd, read_data, read_len);
        i2c_give(i2c_interf);
    }

    if (received_len != read_len) {
        strcpy(pcWriteBuffer, "\r\nERROR, try again");
    }

    strcpy(pcWriteBuffer, "\r\n");
    pcWriteBuffer += 2;
    for (uint8_t i = 0; i < received_len; i++) {
        itoa(read_data[i], pcWriteBuffer, 10);
        pcWriteBuffer = &pcWriteBuffer[strlen(pcWriteBuffer)];
        *pcWriteBuffer = ' ';
        pcWriteBuffer++;
    }
    strcat(pcWriteBuffer, "\r\n");
    return pdFALSE;
}

static BaseType_t I2cWriteCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    uint8_t i2c_interf, i2c_addr, i2c_mux_bus, write_len = 0;
    uint8_t tx_data[256] = { 0 };
    BaseType_t lParameterStringLength;

    /* Ensure there is always a null terminator after each character written. */
    memset(pcWriteBuffer, 0x00, xWriteBufferLen);

    i2c_mux_bus = atoi(FreeRTOS_CLIGetParameter((const char *) pcCommandString, 1, &lParameterStringLength));
    i2c_addr = atoi(FreeRTOS_CLIGetParameter((const char *) pcCommandString, 2, &lParameterStringLength));
    BaseType_t tx_num = 0;
    do {
        tx_data[tx_num] = atoi(
                FreeRTOS_CLIGetParameter((const char *) pcCommandString, tx_num + 3, &lParameterStringLength));
        tx_num++;
    } while (lParameterStringLength);

    uint8_t bus_id = 0;
    for (uint8_t i = 0; i < I2C_BUS_CNT; i++) {
        if (i2c_bus_map[i].mux_bus == i2c_mux_bus) {
            bus_id = i;
            break;
        }
    }

    if (i2c_take_by_busid(bus_id, &i2c_interf, (TickType_t) 10)) {
        write_len = xI2CMasterWrite(i2c_interf, i2c_addr, tx_data, tx_num - 1);
        i2c_give(i2c_interf);
    }

    if (write_len == tx_num - 1) {
        strcpy(pcWriteBuffer, "\r\nOK");
    } else {
        strcpy(pcWriteBuffer, "\r\nERROR, try again");
    }
    return pdFALSE;
}


static const CLI_Command_Definition_t GpioReadCommandDefinition = {
    "gpio_read",
    "\r\ngpio_read <gpio>:\r\n Reads <gpio> value\r\n",
    GpioReadCommand,
    1
};

static const CLI_Command_Definition_t GpioWriteCommandDefinition = {
    "gpio_write",
    "\r\ngpio_write <gpio> <value>:\r\n Writes <value> to the selected <gpio>\r\n",
    GpioWriteCommand,
    2
};

static const CLI_Command_Definition_t ReadMagicValueCommandDefinition = {
    "magic",
    "\r\nmagic:\r\n Reads magic value\r\n",
    ReadMagicValueCommand,
    0
};

static const CLI_Command_Definition_t ReadVersionCommandDefinition = {
    "version",
    "\r\nversion:\r\n Returns firmware version\r\n",
    ReadVersionCommand,
    0
};

static const CLI_Command_Definition_t ResetCommandDefinition = {
    "reset",
    "\r\nreset:\r\n Resets MMC\r\n",
    ResetCommand,
    0
};

static const CLI_Command_Definition_t I2cReadCommandDefinition = {
    "i2c_read",
    "\r\ni2c_read <MUX port> <chip address> <chip register> <RX data length>:\r\n Reads data from I2C chip\r\n",
    I2cReadCommand,
    4
};

static const CLI_Command_Definition_t I2cWriteCommandDefinition = {
    "i2c_write",
    "\r\ni2c_write <MUX port> <chip address> <chip register> <bytes to write>:\r\n Writes data to I2C chip\r\n",
    I2cWriteCommand,
    -1
};

/**
 * @brief Registers all the defined CLI commands.
 */
void RegisterCLICommands(void)
{
    // GPIO
    FreeRTOS_CLIRegisterCommand(&GpioReadCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&GpioWriteCommandDefinition);

    // Others
    FreeRTOS_CLIRegisterCommand(&ReadMagicValueCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&ReadVersionCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&ResetCommandDefinition);

    // I2C
    FreeRTOS_CLIRegisterCommand(&I2cReadCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&I2cWriteCommandDefinition);
}
