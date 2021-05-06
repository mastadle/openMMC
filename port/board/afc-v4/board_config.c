/*
 *   openMMC -- Open Source modular IPM Controller firmware
 *
 *   Copyright (C) 2015  Piotr Miedzik  <P.Miedzik@gsi.de>
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

/* Project Includes */
#include "port.h"
#include "idt_8v54816.h"

uint8_t clock_switch_default_config(void) {
    clock_switch_set_single_channel(0,  IDT_DIR_OUT | IDT_POL_P | IDT_TERM_OFF |  5); // AMC_CLK / RTM_CLK
    clock_switch_set_single_channel(1,  IDT_DIR_IN  | IDT_POL_P | IDT_TERM_OFF |  0); // FMC2_CLK1 / FMC2_CLK3
    clock_switch_set_single_channel(2,  IDT_DIR_IN  | IDT_POL_P | IDT_TERM_OFF |  0); // FMC1_CLK1 / FMC1_CLK3
    clock_switch_set_single_channel(3,  IDT_DIR_IN  | IDT_POL_P | IDT_TERM_OFF |  0); // FMC1_CLK1
    clock_switch_set_single_channel(4,  IDT_DIR_IN  | IDT_POL_P | IDT_TERM_OFF |  0); // FMC1_CLK2
    clock_switch_set_single_channel(5,  IDT_DIR_IN  | IDT_POL_P | IDT_TERM_ON  |  0); // SI57X
    clock_switch_set_single_channel(6,  IDT_DIR_IN  | IDT_POL_P | IDT_TERM_OFF |  0); // FMC2_CLK0
    clock_switch_set_single_channel(7,  IDT_DIR_OUT | IDT_POL_P | IDT_TERM_OFF |  5); // FMC2_CLK2
    clock_switch_set_single_channel(8,  IDT_DIR_IN  | IDT_POL_P | IDT_TERM_OFF |  0); // TCLKD / FPGA_CLK3
    clock_switch_set_single_channel(9,  IDT_DIR_IN  | IDT_POL_P | IDT_TERM_OFF |  0); // TCLKC / FPGA_CLK2
    clock_switch_set_single_channel(10, IDT_DIR_IN  | IDT_POL_P | IDT_TERM_ON  |  0); // TCLKA
    clock_switch_set_single_channel(11, IDT_DIR_IN  | IDT_POL_P | IDT_TERM_OFF |  0); // TCLKB
    clock_switch_set_single_channel(12, IDT_DIR_IN  | IDT_POL_P | IDT_TERM_OFF |  0); // FLEX_CLK0
    clock_switch_set_single_channel(13, IDT_DIR_OUT | IDT_POL_P | IDT_TERM_OFF |  5); // FLEX_CLK1
    clock_switch_set_single_channel(14, IDT_DIR_IN  | IDT_POL_P | IDT_TERM_OFF |  0); // FLEX_CLK2
    clock_switch_set_single_channel(15, IDT_DIR_OUT | IDT_POL_P | IDT_TERM_OFF |  5); // FLEX_CLK3
    return 0;
}

void board_init() {
    /* I2C MUX Init */
    gpio_set_pin_state(PIN_PORT(GPIO_I2C_MUX_ADDR1), PIN_NUMBER(GPIO_I2C_MUX_ADDR1), GPIO_LEVEL_LOW);
    gpio_set_pin_state(PIN_PORT(GPIO_I2C_MUX_ADDR2), PIN_NUMBER(GPIO_I2C_MUX_ADDR2), GPIO_LEVEL_LOW);
    gpio_set_pin_state(PIN_PORT(GPIO_I2C_SW_RESETn), PIN_NUMBER(GPIO_I2C_SW_RESETn), GPIO_LEVEL_HIGH);
}

void board_config() {
	//clock_switch_default_config();
}
