#ifndef FUNCTION_GENERATOR_INTERNAL_H
#define FUNCTION_GENERATOR_INTERNAL_H

#include "function_generator.h"

// Normally private, but needed for testing
lcm_status_t function_generator_increment_phase(function_generator_t *fg, bool repeat);
lcm_status_t calculate_sample(const uint16_t phase, const function_generator_t *fg, fixed16_t *sample);

#endif /* FUNCTION_GENERATOR_INTERNAL_H */