/*
 *   openMMC -- Open Source modular IPM Controller firmware
 *
 *   Copyright (C) 2015-2016  Henrique Silva <henrique.silva@lnls.br>
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

/**
 * @defgroup MCP9808 - Temperature Sensor
 * @ingroup SENSORS
 *
 * The MCP9808 is a temperature sensor <br>
 * The open-drain temperature alert (ALERT) output becomes active when the temperature exceeds a programmable limit.
 * This pin can operate in either “Comparator” or “Interrupt” mode.
 */

/**
 * @file lm75.h
 * @author Henrique Silva <henrique.silva@lnls.br>, LNLS
 *
 * @brief Definitions for MCP9808 I2C Temperature Sensor
 *
 * @ingroup MCP9808
 */

#ifndef MCP9808_H_
#define MCP9808_H_

/**
 * @brief Rate at which the MCP9808 sensors are read (in ms)
 */
#define MCP9808_UPDATE_RATE        500

extern TaskHandle_t vTaskMCP9808_Handle;


/**
 * @brief Initializes MCP9808 monitoring task
 *
 * @return None
 */
void MCP9808_init( void );

/**
 * @brief Monitoring task for MCP9808 sensor
 *
 * This task unblocks after every #MCP9808_UPDATE_RATE ms and updates the read from all the MCP9808 sensors listed in this module's SDR table
 *
 * @param Parameters Pointer to parameter list passed to task upon initialization (not used here)
 */
void vTaskMCP9808( void* Parameters );

#endif
