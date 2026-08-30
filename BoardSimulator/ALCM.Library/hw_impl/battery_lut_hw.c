/*
 * BoardSimulator - Windows hardware layer for the battery LUT block
 *
 * There's no real flash to read from in simulation, and no P/Invoke hook
 * (yet) to inject a patched block from the WPF app - so this always
 * reports "unpatched", the same safe default a real unpatched device
 * falls back to. battery_lut_validate_block() treats NULL as invalid.
 */

#include "battery_lut_hw.h"

const battery_lut_block_t *battery_lut_hw_get_block(void) {
    return (const battery_lut_block_t *)0;
}
