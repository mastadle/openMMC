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
#include "eeprom_24xx02.h"
#include "pin_mapping.h"

/* Project Includes */
#include "cli_commands.h"

#include "ipmi.h"
#include "port.h"
#include "portable.h"
#include "portmacro.h"
#ifdef MODULE_PAYLOAD
#include "payload.h"
#endif
#ifdef MODULE_HOTSWAP
#include "hotswap.h"
#endif
#include "GitSHA1.h"
#include "adn4604.h"
#include "adn4604_usercfg.h"
#include "clock_config.h"
#include "flash_spi.h"
#include "fru.h"
#include "i2c_mapping.h"
#include "ina220.h"
#include "led.h"
#include "utils.h"

/* C Standard includes */
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdlib.h>

#define MAX_GPIO_NAME_LENGTH 20
const char cursor_save[] = "\e[s";    // Save cursor position
const char cursor_restore[] = "\e[u"; // Move cursor to saved position

static BaseType_t GpioReadCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    char name[MAX_GPIO_NAME_LENGTH];
    name[0] = '\0';
    uint8_t value, port, pin;
    BaseType_t lParameterStringLength;

    // Get GPIO name
    const char* param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &lParameterStringLength);
    strncat(name, param, MAX_GPIO_NAME_LENGTH-1);

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

    /* Low Jitter Clock Bypass */
    else if (strcmp(name, "tclka_tclkc_sel") == 0) {
        value = gpio_read_pin(PIN_PORT(GPIO_TCLKA_TCLKC_SEL), PIN_NUMBER(GPIO_TCLKA_TCLKC_SEL));
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
            snprintf(pcWriteBuffer, xWriteBufferLen, "GPIO name <%s> not recognized.", name);
            return pdFALSE;
        }
        value = gpio_read_pin(port, pin);
    }

    // Write GPIO value to the output buffer
    itoa(value, pcWriteBuffer, 10);

    return pdFALSE;
}

static BaseType_t GpioWriteCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    char name[MAX_GPIO_NAME_LENGTH];
    name[0] = '\0';
    BaseType_t param_1_length, param_2_length;

    // Get GPIO value
    uint8_t value = atoi(FreeRTOS_CLIGetParameter(pcCommandString, 2, &param_2_length));

    // Get GPIO name
    const char * param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &param_1_length);
    if (param_1_length >= MAX_GPIO_NAME_LENGTH-1) {
        param_1_length = MAX_GPIO_NAME_LENGTH-1;
    }
    strncat(name, param, param_1_length);

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

    /* Low Jitter Clock Bypass */
    else if (strcmp(name, "tclka_tclkc_sel") == 0) {
        gpio_set_pin_state(PIN_PORT(GPIO_TCLKA_TCLKC_SEL), PIN_NUMBER(GPIO_TCLKA_TCLKC_SEL), value & 0x1);
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
        snprintf(pcWriteBuffer, xWriteBufferLen, "GPIO name <%s> not recognized.", name);
        return pdFALSE;
    }

    strcpy(pcWriteBuffer, "OK");

    return pdFALSE;
}

static BaseType_t gpioReadDirectionCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                           const char *pcCommandString) {
    char name[MAX_GPIO_NAME_LENGTH];
    name[0] = '\0';
    uint8_t direction, port, pin;
    BaseType_t lParameterStringLength;

    // Get GPIO name
    const char *param =
        FreeRTOS_CLIGetParameter(pcCommandString, 1, &lParameterStringLength);
    strncat(name, param, MAX_GPIO_NAME_LENGTH - 1);

    // Convert GPIO name to lower-case
    for (int i = 0; name[i]; i++) {
        name[i] = tolower((int)name[i]);
    }

    /* Low Jitter Clock Bypass */
    if (strcmp(name, "tclka_tclkc_sel") == 0) {
        direction = gpio_get_pin_dir(PIN_PORT(GPIO_TCLKA_TCLKC_SEL),
                                     PIN_NUMBER(GPIO_TCLKA_TCLKC_SEL));
    }

    else {
        port = atoi(FreeRTOS_CLIGetParameter((const char *)pcCommandString, 1,
                                             &lParameterStringLength));
        pin = atoi(FreeRTOS_CLIGetParameter((const char *)pcCommandString, 2,
                                            &lParameterStringLength));
        if (!lParameterStringLength) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "GPIO name <%s> not recognized.",
                     name);
            return pdFALSE;
        }
        direction = gpio_get_pin_dir(port, pin);
    }

    // Write GPIO direction to the output buffer
    itoa(direction, pcWriteBuffer, 10);

    return pdFALSE;
}

static BaseType_t gpioWriteDirectionCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                            const char *pcCommandString) {
    char name[MAX_GPIO_NAME_LENGTH];
    name[0] = '\0';
    BaseType_t param_1_length, param_2_length;

    // Get GPIO direction
    uint8_t direction =
        atoi(FreeRTOS_CLIGetParameter(pcCommandString, 2, &param_2_length));

    // Get GPIO name
    const char *param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &param_1_length);
    if (param_1_length >= MAX_GPIO_NAME_LENGTH - 1) {
        param_1_length = MAX_GPIO_NAME_LENGTH - 1;
    }
    strncat(name, param, param_1_length);

    // Convert GPIO name to lower-case
    for (int i = 0; name[i]; i++) {
        name[i] = tolower((int)name[i]);
    }

    /* Low Jitter Clock Bypass */
    if (strcmp(name, "tclka_tclkc_sel") == 0) {
        gpio_set_pin_dir(PIN_PORT(GPIO_TCLKA_TCLKC_SEL),
                         PIN_NUMBER(GPIO_TCLKA_TCLKC_SEL), direction & 0x1);
    }

    else {
        snprintf(pcWriteBuffer, xWriteBufferLen, "GPIO name <%s> not recognized.",
                 name);
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
    uint8_t i2c_interf, i2c_addr, i2c_bus_id, cmd, read_len, received_len = 0;
    uint8_t read_data[256] = { 0xFF };

    /* Ensure there is always a null terminator after each character written. */
    memset(pcWriteBuffer, 0x00, xWriteBufferLen);

    i2c_bus_id = atoi(FreeRTOS_CLIGetParameter((const char *) pcCommandString, 1, &param_1_length));
    i2c_addr = atoi(FreeRTOS_CLIGetParameter((const char *) pcCommandString, 2, &param_2_length));
    cmd = atoi(FreeRTOS_CLIGetParameter((const char *) pcCommandString, 3, &param_3_length));
    read_len = atoi(FreeRTOS_CLIGetParameter((const char *) pcCommandString, 4, &param_4_length));

    if (i2c_bus_id >= I2C_BUS_CNT) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "I2C bus id %d is larger than bus count %d", i2c_bus_id, I2C_BUS_CNT);
    }

    if (i2c_take_by_busid(i2c_bus_id, &i2c_interf, (TickType_t) 10)) {
        received_len = xI2CMasterWriteRead(i2c_interf, i2c_addr, &cmd, sizeof(cmd), read_data, read_len);
        i2c_give(i2c_interf);
    }

    if (received_len != read_len) {
        strcpy(pcWriteBuffer, "ERROR, try again");
    }

    for (uint8_t i = 0; i < received_len; i++) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "0x%02x ", read_data[i]);
        pcWriteBuffer += 5;
        xWriteBufferLen -= 5;
    }
    return pdFALSE;
}

static BaseType_t I2cWriteCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    uint8_t i2c_interf, i2c_addr, i2c_bus_id, write_len = 0;
    uint8_t tx_data[256] = { 0 };
    BaseType_t lParameterStringLength;

    /* Ensure there is always a null terminator after each character written. */
    memset(pcWriteBuffer, 0x00, xWriteBufferLen);

    i2c_bus_id = atoi(FreeRTOS_CLIGetParameter((const char *) pcCommandString, 1, &lParameterStringLength));
    i2c_addr = atoi(FreeRTOS_CLIGetParameter((const char *) pcCommandString, 2, &lParameterStringLength));
    BaseType_t tx_num = 0;
    do {
        tx_data[tx_num] = atoi(
                FreeRTOS_CLIGetParameter((const char *) pcCommandString, tx_num + 3, &lParameterStringLength));
        tx_num++;
    } while (lParameterStringLength);

    if (i2c_bus_id >= I2C_BUS_CNT) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "I2C bus id %d is larger than bus count %d", i2c_bus_id, I2C_BUS_CNT);
    }

    if (i2c_take_by_busid(i2c_bus_id, &i2c_interf, (TickType_t) 10)) {
        write_len = xI2CMasterWrite(i2c_interf, i2c_addr, tx_data, tx_num - 1);
        i2c_give(i2c_interf);
    }

    if (write_len == tx_num - 1) {
        strcpy(pcWriteBuffer, "OK");
    } else {
        strcpy(pcWriteBuffer, "ERROR, try again");
    }
    return pdFALSE;
}

unsigned flash_number_of_pages = 0;
uint8_t flash_idx = 0;
static BaseType_t FlashInitCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    pcWriteBuffer[0] = '\0';
    BaseType_t lParameterStringLength;
    if (flash_number_of_pages == 0) {
        flash_idx = atoi(FreeRTOS_CLIGetParameter(pcCommandString, 1, &lParameterStringLength));
        flash_number_of_pages = atoi(FreeRTOS_CLIGetParameter(pcCommandString, 2, &lParameterStringLength));
        gpio_set_pin_state(PIN_PORT(GPIO_FLASH_CS_MUX), PIN_NUMBER(GPIO_FLASH_CS_MUX), flash_idx & 0x1);
        /* Wait a millisecond just in case */
        vTaskDelay(1 / portTICK_PERIOD_MS);
        uint8_t status = payload_hpm_prepare_comp_pages(flash_number_of_pages);
        if (status == IPMI_CC_OUT_OF_SPACE) {
            strncat(pcWriteBuffer, "MMC is out of memory. Abort.", xWriteBufferLen);
            return pdFALSE;
        } else if (status == IPMI_CC_TIMEOUT) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "timeout while waiting for flash. Flash status: 0x%02x", flash_read_status_reg());
            return pdFALSE;
        } else if (status == IPMI_CC_UNSPECIFIED_ERROR) {
            /* Check flash id */
            uint8_t flash_id[3];
            flash_read_id(flash_id, sizeof(flash_id));
            uint32_t full_id = (flash_id[0] << 16)|(flash_id[1] << 8)|(flash_id[2]);
            snprintf(pcWriteBuffer, xWriteBufferLen, "Unknown flash id: 0x%06" PRIx32, full_id);
            return pdFALSE;
        }
        strncat(pcWriteBuffer, "Initialising flash write", xWriteBufferLen);
    } else {
        payload_hpm_finish_upload(flash_number_of_pages);
        snprintf(pcWriteBuffer, xWriteBufferLen, "flash writing in progress. %d pages to go. Aborting", flash_number_of_pages);
        flash_number_of_pages = 0;
    }

    return pdFALSE;
}

static BaseType_t FlashUploadBlockCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    pcWriteBuffer[0] = '\0';
    BaseType_t lParameterStringLength;
    const char * arg = FreeRTOS_CLIGetParameter(pcCommandString, 1, &lParameterStringLength);
    bool repeat = arg != NULL && arg[0] == 'r';
    if (repeat) {
        ++flash_number_of_pages;
    }
    uint8_t *block = (uint8_t *) pvPortMalloc(PAYLOAD_HPM_PAGE_SIZE);
    uart_read_blocking(UART_DEBUG, block, PAYLOAD_HPM_PAGE_SIZE);
    uart_send(UART_DEBUG, block, PAYLOAD_HPM_PAGE_SIZE);
    unsigned number_of_checks = 0;
    while ( is_flash_busy() ) {
        if( ++number_of_checks > FLASH_BUSY_MAX_POLLS ) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "timeout while waiting for flash. Flash status: 0x%02x", flash_read_status_reg());
            return pdFALSE;
        }
        vTaskDelay(FLASH_BUSY_POLL_PERIOD_MS / portTICK_PERIOD_MS);
    }
    payload_hpm_upload_block_repeat(repeat, block, PAYLOAD_HPM_PAGE_SIZE);
    vPortFree(block);
    --flash_number_of_pages;
    
    return pdFALSE;
}

static BaseType_t FlashFinaliseCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    pcWriteBuffer[0] = '\0';
    unsigned number_of_checks = 0;
    while ( is_flash_busy() ) {
        if( ++number_of_checks > FLASH_BUSY_MAX_POLLS ) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "timeout while waiting for flash. Flash status: 0x%02x", flash_read_status_reg());
            return pdFALSE;
        }
        vTaskDelay(FLASH_BUSY_POLL_PERIOD_MS / portTICK_PERIOD_MS);
    }
    if (flash_number_of_pages > 0) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Finalising incomplete flash. Missing %d pages.", flash_number_of_pages);
        flash_number_of_pages = 0;
    } else {
        strncat(pcWriteBuffer, "Finalising flash", xWriteBufferLen);
    }
    BaseType_t lParameterStringLength;
    uint32_t image_size = atoi(FreeRTOS_CLIGetParameter(pcCommandString, 1, &lParameterStringLength));
    payload_hpm_finish_upload(image_size);

    return pdFALSE;
}

static BaseType_t FlashActivateFirmwareCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    pcWriteBuffer[0] = '\0';
    BaseType_t lParameterStringLength;
    flash_idx = atoi(FreeRTOS_CLIGetParameter(pcCommandString, 1, &lParameterStringLength));
    gpio_set_pin_state(PIN_PORT(GPIO_FLASH_CS_MUX), PIN_NUMBER(GPIO_FLASH_CS_MUX), flash_idx & 0x1);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    payload_hpm_activate_firmware();
    return pdFALSE;
}

static BaseType_t FlashReadCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    pcWriteBuffer[0] = '\0';
    BaseType_t lParameterStringLength;
    flash_idx = atoi(FreeRTOS_CLIGetParameter(pcCommandString, 1, &lParameterStringLength));
    gpio_set_pin_state(PIN_PORT(GPIO_FLASH_CS_MUX), PIN_NUMBER(GPIO_FLASH_CS_MUX), flash_idx & 0x1);
    int page_index = atoi(FreeRTOS_CLIGetParameter(pcCommandString, 2, &lParameterStringLength));
    int wait_for_flash = atoi(FreeRTOS_CLIGetParameter(pcCommandString, 3, &lParameterStringLength));
    uint8_t block[PAYLOAD_HPM_PAGE_SIZE];
    if (wait_for_flash) {
        unsigned number_of_checks = 0;
        while ( is_flash_busy() ) {
            if( ++number_of_checks > FLASH_BUSY_MAX_POLLS ) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "timeout while waiting for flash. Flash status: 0x%02x", flash_read_status_reg());
                return pdFALSE;
            }
            vTaskDelay(FLASH_BUSY_POLL_PERIOD_MS / portTICK_PERIOD_MS);
        }
    }
    flash_fast_read_data(sizeof(block)*page_index, block, sizeof(block));
    uart_send(UART_DEBUG, block, PAYLOAD_HPM_PAGE_SIZE);
    return pdFALSE;
}

static BaseType_t FlashReadIdCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    pcWriteBuffer[0] = '\0';
    BaseType_t lParameterStringLength;
    flash_idx = atoi(FreeRTOS_CLIGetParameter(pcCommandString, 1, &lParameterStringLength));
    gpio_set_pin_state(PIN_PORT(GPIO_FLASH_CS_MUX), PIN_NUMBER(GPIO_FLASH_CS_MUX), flash_idx & 0x1);
    int n_chars = snprintf(pcWriteBuffer, xWriteBufferLen, "Set flash cs mux to %d, ", flash_idx);
    uint8_t flash_id[3];
    flash_read_id(flash_id, sizeof(flash_id));
    uint32_t full_id = (flash_id[0] << 16)|(flash_id[1] << 8)|(flash_id[2]);

    n_chars += snprintf(pcWriteBuffer + n_chars, xWriteBufferLen - n_chars, "SPI flash id: 0x%06" PRIx32, full_id);
    return pdFALSE;
}

static BaseType_t FPGAResetCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    pcWriteBuffer[0] = '\0';
    payload_send_message(FRU_AMC, PAYLOAD_MESSAGE_REBOOT);
    return pdFALSE;
}

static BaseType_t PrintVoltagesAndTemperatures(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    pcWriteBuffer[0] = '\0';
    BaseType_t lParameterStringLength;
    bool repeat = FreeRTOS_CLIGetParameter(pcCommandString, 1, &lParameterStringLength);
    if (repeat) {
        unsigned lines = 0;
        for (sensor_t * sensor = sdr_head; sensor != NULL; sensor = sensor->next) {
            if ( sensor->sdr_type != TYPE_01 ) {
                continue;
            }

            SDR_type_01h_t * sdr_entry = (SDR_type_01h_t *) sensor->sdr;
            lines += (sdr_entry->sensortype == SENSOR_TYPE_TEMPERATURE ||
                    sdr_entry->sensortype == SENSOR_TYPE_VOLTAGE ||
                    sdr_entry->sensortype == SENSOR_TYPE_CURRENT);
            uart_send(UART_DEBUG, "\r\n", 2);
        }
        /* Move cursor to the previous position */
        printf("\e[%dA", lines);
        uart_send(UART_DEBUG, cursor_save, sizeof(cursor_save)-1);
    }
    do {
        for (sensor_t * sensor = sdr_head; sensor != NULL; sensor = sensor->next) {
            if ( sensor->sdr_type != TYPE_01 ) {
                continue;
            }

            SDR_type_01h_t * sdr_entry = (SDR_type_01h_t *) sensor->sdr;
            if ( sdr_entry->sensortype == SENSOR_TYPE_TEMPERATURE) {
                if (strcmp(sdr_entry->IDstring, "TEMP FPGA") == 0) {
                    printf("Sensor %s : %2d °C\n", sdr_entry->IDstring, sensor->readout_value );
                } else {
                    printf("Sensor %s : %2d.%1d °C\n", sdr_entry->IDstring, sensor->readout_value >> 1, 5*(sensor->readout_value & 0x1));
                }
            } else if (sdr_entry->sensortype == SENSOR_TYPE_VOLTAGE) {
                /* Undo truncation to 1 byte and take into account that 1 LSB is 4 mV */
                printf("Sensor %s : %5d mV\n", sdr_entry->IDstring, sensor->readout_value << 6 );
            } else if (sdr_entry->sensortype == SENSOR_TYPE_CURRENT) {
                /* Undo truncation and print signed */
                printf("Sensor %s : %4d mA\n", sdr_entry->IDstring, (int16_t) (sensor->readout_value << 5));
            }
        }
        if (repeat) {
            if (uart_read(UART_DEBUG, pcWriteBuffer, 1)) {
                repeat = false;
                /* Discard byte that was read */
                pcWriteBuffer[0] = '\0';
            } else {
                uart_send(UART_DEBUG, cursor_restore, sizeof(cursor_restore)-1);
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        }
    } while (repeat);
    return pdFALSE;
}

static BaseType_t clockConfigReadCommand(char *pcWriteBuffer, size_t xWriteBufferLength,
                                         const char *pcCommandString) {
    // Initialize the output buffer with an empty string
    pcWriteBuffer[0] = '\0';

    // Define the number of clocks to configure
    const uint8_t numberOfClocks = 16;

    // Initialize variables to store the extracted enable mask and output map
    uint16_t enableMaskValue = 0;
    uint64_t outputMapValue = 0;

    // Print the clock configuration table header
    printf("| Output          | Enabled | Input                   |\r\n");
    printf("|-----------------|---------|-------------------------|\r\n");

    // Iterate over each clock output to read the current configuration
    for (uint8_t i = 0; i < numberOfClocks; ++i) {
        // Read the configuration for the current clock output
        uint8_t config = clock_config[i];

        // Extract the enable bit from the 7th position and
        // update the enable mask by setting the corresponding bit
        uint8_t enableOutput = (config >> 7) & 0x1;
        enableMaskValue |= (enableOutput << i);

        // Extract the 4-bit selected input source from the lower bits and
        // update the output map by setting the corresponding 4-bit value
        uint8_t selectedInput = config & 0xF;
        outputMapValue |= ((uint64_t)selectedInput << (4 * i));

        // Print the current clock configuration
        printf("| %-2u %-12s | %-7u | %-2u %-20s |\r\n", i, ADN4604_OUTPUT[i],
               enableOutput, selectedInput, ADN4604_INPUT[selectedInput]);
    }

    // Print the current enable mask
    printf("\r\nCurrent enable mask: 0x%04X\r\n", enableMaskValue);

    // Print the current clock output map
    printf("Current clock output map: 0x");
    for (uint8_t i = numberOfClocks - 1; i < numberOfClocks; i--) {
        // Extract each 4-bit segment and print as a hexadecimal digit
        uint8_t segment = (outputMapValue >> (4 * i)) & 0xF;
        printf("%X", segment);
    }
    printf("\r\n");

    return pdFALSE;
}

static BaseType_t clockConfigWriteCommand(char *pcWriteBuffer,
                                          size_t xWriteBufferLength,
                                          const char *pcCommandString) {
    // Initialize the output buffer with an empty string
    pcWriteBuffer[0] = '\0';

    // Define the number of clocks to configure
    const uint8_t numberOfClocks = 16;

    // Retrieve the enable mask, which is the first parameter in the command string
    BaseType_t enableMaskStringLength;
    const char *enableMaskString =
        FreeRTOS_CLIGetParameter(pcCommandString, 1, &enableMaskStringLength);

    // Validate that the enable mask parameter is provided and is not an empty string
    if (enableMaskString == NULL || enableMaskStringLength == 0) {
        snprintf(pcWriteBuffer, xWriteBufferLength,
                 "Error: Missing enable mask parameter.\n");
        return pdFALSE;
    }

    // Validate that the "0x" is present
    if (enableMaskString[0] != '0' || enableMaskString[1] != 'x') {
        snprintf(pcWriteBuffer, xWriteBufferLength,
                 "Error: Enable mask must be of the form 0x1234. Received "
                 "enable mask string: %s\n",
                 enableMaskString);
        return pdFALSE;
    }

    // Validate the enable mask length
    const uint8_t expectedEnableMaskStringLength = ceil(numberOfClocks / 4);
    if (enableMaskStringLength != expectedEnableMaskStringLength + 2) { // +2 for "0x"
        snprintf(pcWriteBuffer, xWriteBufferLength,
                 "Error: Enable mask must be %u digits (e.g., 0x1234). Received "
                 "enable mask string: %s\n",
                 expectedEnableMaskStringLength, enableMaskString);
        return pdFALSE;
    }

    // Convert the enable mask hexadecimal string to integer and validate
    char *endPtr;
    const uint16_t enableMaskValue = (uint16_t)strtoul(enableMaskString, &endPtr, 16);
    if (*endPtr != '\0' && *endPtr != ' ') {
        snprintf(pcWriteBuffer, xWriteBufferLength,
                 "Error: Invalid characters in enable mask string.\n");
        return pdFALSE;
    }

    // Retrieve the output map, which is the second parameter in the command string
    BaseType_t outputMapStringLength;
    const char *outputMapString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &outputMapStringLength);

    // Validate that the output map parameter is provided and is not an empty string
    if (outputMapString == NULL || outputMapStringLength == 0) {
        snprintf(pcWriteBuffer, xWriteBufferLength,
                 "Error: Missing output map parameter.\n");
        return pdFALSE;
    }

    // Validate that the "0x" is present
    if (outputMapString[0] != '0' || outputMapString[1] != 'x') {
        snprintf(pcWriteBuffer, xWriteBufferLength,
                 "Error: Output map must be of the form 0x123456479ABCDEF. Received "
                 "output map string: %s\n",
                 outputMapString);
        return pdFALSE;
    }

    // Validate the output map length
    const uint8_t expectedOutputMapStringLength = numberOfClocks;
    if (outputMapStringLength != expectedOutputMapStringLength + 2) { // +2 for "0x"
        snprintf(
            pcWriteBuffer, xWriteBufferLength,
            "Error: Output map must be %u digits (e.g., 0x123456789ABCDEF). Received "
            "output map string: %s\n",
            expectedOutputMapStringLength, outputMapString);
        return pdFALSE;
    }

    // Convert the output map hexadecimal string to integer and validate
    const uint64_t outputMapValue = strtoull(outputMapString, &endPtr, 16);
    if (*endPtr != '\0' && *endPtr != ' ') {
        snprintf(pcWriteBuffer, xWriteBufferLength,
                 "Error: Invalid characters in output map string.\n");
        return pdFALSE;
    }

    // Get the persistent storage preference, the third parameter in the command string
    BaseType_t writeToEEPROMStringLength;
    const char *writeToEEPROMString =
        FreeRTOS_CLIGetParameter(pcCommandString, 3, &writeToEEPROMStringLength);

    // Validate the writeToEEPROM parameter (only allow '0' or '1')
    if (writeToEEPROMString != NULL && writeToEEPROMStringLength != 0) {
        if (writeToEEPROMString[0] != '0' && writeToEEPROMString[0] != '1') {
            snprintf(pcWriteBuffer, xWriteBufferLength,
                     "Error: EEPROM write flag must be '0' or '1'. Received "
                     "EEPROM write flag: %s\n",
                     writeToEEPROMString ? writeToEEPROMString : "NULL");
            return pdFALSE;
        }
    }

    // Determine if writing to EEPROM is enabled
    bool writeToEEPROMValue = false;
    if (writeToEEPROMString != NULL && writeToEEPROMStringLength != 0) {
        writeToEEPROMValue = (writeToEEPROMString[0] == '1');
    }

    // Iterate over each clock output to configure them according to provided parameters
    for (uint8_t i = 0; i < numberOfClocks; ++i) {
        // Extract a 4-bit value from the output map to determine which input source
        // should be assigned to this clock output
        uint8_t selectedInput = (outputMapValue >> (4 * i)) & 0xF;

        // Extract a 1-bit value from the enable mask to determine if the current clock
        // output should be enabled or disabled
        uint8_t enableOutput = (enableMaskValue >> i) & 0x1;

        // Set the clock configuration for the current clock output
        clock_config[i] = (enableOutput << 7) | selectedInput;
    }

    // Check the EEPROM write flag and write to EEPROM
    if (writeToEEPROMValue) {
        printf("Writing to EEPROM...\n");
        eeprom_24xx02_write(CHIP_ID_RTC_EEPROM, 0x0, clock_config, 16, 10);
    }

    // Reset and configure
    printf("Resetting and configuring clocks...\n");
    adn4604_reset();
    clock_configuration(clock_config);

    // Print the current configuration
    printf("Clock configuration set successfully.\n");
    clockConfigReadCommand(pcWriteBuffer, xWriteBufferLength, pcCommandString);
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

static const CLI_Command_Definition_t GpioReadDirectionCommandDefinition = {
    "gpio_direction_read",
    "\r\ngpio_direction_read <gpio>:\r\nReads <gpio> direction. Returns 0 for input "
    "and 1 for output\r\n",
    gpioReadDirectionCommand, 1};

static const CLI_Command_Definition_t GpioWriteDirectionCommandDefinition = {
    "gpio_direction_write",
    "\r\ngpio_direction_write <gpio> <direction>:\r\nSets <direction> for the <gpio>. "
    "Use 0 for input and 1 for output\r\n",
    gpioWriteDirectionCommand, 2};

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

static const CLI_Command_Definition_t FlashInitCommandDefinition = {
    "flash_init",
    "\r\nflash_init <flash index> <number of pages>:\r\n Initialise FPGA boot flash\r\n",
    FlashInitCommand,
    2
};

static const CLI_Command_Definition_t FlashUploadBlockCommandDefinition = {
    "flash_upload",
    "\r\nflash_upload <r>:\r\n Write FPGA boot flash block. Optionally repeat previous block\r\n",
    FlashUploadBlockCommand,
    1
};

static const CLI_Command_Definition_t FlashFinaliseCommandDefinition = {
    "flash_final",
    "\r\nflash_final <image_size>:\r\n Finalise FPGA boot flash and reboot FPGA\r\n",
    FlashFinaliseCommand,
    1
};

static const CLI_Command_Definition_t FlashActivateFirmwareCommandDefinition = {
    "flash_activate_firmware",
    "\r\nflash_activate_firmware <flash index>:\r\n Reboot the FPGA from flash 0 or 1\r\n",
    FlashActivateFirmwareCommand,
    1
};

static const CLI_Command_Definition_t FlashReadCommandDefinition = {
    "flash_read",
    "\r\nflash_read <flash index> <page index> <wait for flash>:\r\n Read one block from SPI\r\n",
    FlashReadCommand,
    3
};

static const CLI_Command_Definition_t FlashReadIdCommandDefinition = {
    "flash_read_id",
    "\r\nflash_read_id <flash index>:\r\n Read ID of SPI flash\r\n",
    FlashReadIdCommand,
    1
};

static const CLI_Command_Definition_t FPGAResetCommandDefinition = {
    "fpga_reset",
    "\r\nfpga_reset:\r\n Assert and deassert FPGA reset signal\r\n",
    FPGAResetCommand,
    0
};

static const CLI_Command_Definition_t PrintSensorReadoutCommandDefinition = {
    "print_sensor_readout",
    "\r\nprint_sensor_readout <continuous>\r\n Print voltages and temperatures of all sensors\r\n",
    PrintVoltagesAndTemperatures,
    -1
};

static const CLI_Command_Definition_t ClockConfigReadDefinition = {
    "clock_config_read",
    "\r\nclock_config_read\r\n Read the current configuration of the clock "
    "crossbar.\r\n",
    clockConfigReadCommand, 0};

static const CLI_Command_Definition_t ClockConfigWriteDefinition = {
    "clock_config_write",
    "\r\nclock_config_write <enable mask> <output map> <eeprom write flag>\r\n Set the "
    "configuration of the clock crossbar.\r\n",
    clockConfigWriteCommand, -1};

/**
 * @brief Registers all the defined CLI commands.
 */
void RegisterCLICommands(void) {
    // GPIO
    FreeRTOS_CLIRegisterCommand(&GpioReadCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&GpioWriteCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&GpioReadDirectionCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&GpioWriteDirectionCommandDefinition);

    // Others
    FreeRTOS_CLIRegisterCommand(&ReadMagicValueCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&ReadVersionCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&ResetCommandDefinition);

    // I2C
    FreeRTOS_CLIRegisterCommand(&I2cReadCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&I2cWriteCommandDefinition);

    // Flash
    FreeRTOS_CLIRegisterCommand(&FlashInitCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&FlashUploadBlockCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&FlashFinaliseCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&FlashActivateFirmwareCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&FlashReadCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&FlashReadIdCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&FPGAResetCommandDefinition);
    FreeRTOS_CLIRegisterCommand(&PrintSensorReadoutCommandDefinition);

    // Clock
    FreeRTOS_CLIRegisterCommand(&ClockConfigReadDefinition);
    FreeRTOS_CLIRegisterCommand(&ClockConfigWriteDefinition);
}
