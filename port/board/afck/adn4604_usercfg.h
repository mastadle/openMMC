/*
 *   openMMC -- Open Source modular IPM Controller firmware
 *
 *   Copyright (C) 2015  Henrique Silva <henrique.silva@lnls.br>
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

#ifdef ADN4604_USERCFG_H_
#error "User configuration for ADN4604 Clock switch already defined by other board port, check the build chain!"
#else
#define ADN4604_USERCFG_H_

const char * ADN4604_INPUT[] = {
    "FMC2_CLKIN3_BIDIR",
    "FMC2_CLK1_M2C",
    "FMC2_CLK0_M2C",
    "FMC2_CLK2_BIDIR",
    "TCLKB_IN",
    "TCLKA_IN",
    "TCLKC_IN",
    "TCLKD_IN",
    "FCLKA_IN",
    "FMC1_CLKIN3_BIDIR",
    "FMC1_CLK1_M2C",
    "FMC1_CLK0_M2C",
    "FMC1_CLK2_BIDIR",
    "WR_PLL_CLK1",
    "CLK20_VCX0",
    "SI57X_CLK"
};

const char * ADN4604_OUTPUT[] = {
    "TCLKD_OUT",
    "TCLKC_OUT",
    "TCLKA_OUT",
    "TCLKB_OUT",
    "FPGA_CLK1",
    "FP2_CLK1",
    "FP2_CLK2",
    "LINK01",
    "PCIE_CLK1",
    "LINK23",
    "FIN1_CLK3",
    "FIN1_CLK2",
    "RTM_SYNC_CLK",
    "OP15C",
    "FIN2_CLK3",
    "FIN2_CLK2"
};


#endif
