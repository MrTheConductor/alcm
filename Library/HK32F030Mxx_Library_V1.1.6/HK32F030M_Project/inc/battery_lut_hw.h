/*
 * Copyright (c) 2024-2026, Mitchell White <mitchell.n.white@gmail.com>
 *
 * This file is part of Advanced LCM (ALCM) project.
 *
 * ALCM is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * ALCM is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with ALCM. If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file battery_lut_hw.h
 * @brief Hardware access for the patchable battery LUT block
 */
#ifndef BATTERY_LUT_HW_H
#define BATTERY_LUT_HW_H

#include "battery_lut.h"

/**
 * @brief Returns a pointer to the flash-resident battery LUT block.
 *
 * The block lives at a fixed address reserved by the linker scatter file
 * (Project/MDK5/battery_lut.sct) so the battery_lut_patch.py tool can
 * target it reliably across firmware versions. This function does not
 * validate the block's contents - call battery_lut_validate_block() on
 * the result before trusting it.
 */
const battery_lut_block_t *battery_lut_hw_get_block(void);

#endif
