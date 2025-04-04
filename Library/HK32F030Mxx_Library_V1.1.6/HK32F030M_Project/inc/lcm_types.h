#ifndef _LCM_TYPES_H
#define _LCM_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Advanced LCM status type
 *
 * @details This type represents the status of various operations
 *          within the Advanced LCM. The status is used to indicate
 *          whether an operation succeeded or failed.
 */
typedef enum
{
    LCM_SUCCESS = 0,         // Operation succeeded
    LCM_ERROR,               // Operation failed
    LCM_BUSY,                // Operation busy
    LCM_ERROR_NULL_POINTER,  // Operation failed due to null pointer
    LCM_ERROR_INVALID_PARAM, // Operation failed due to invalid parameter
    LCM_QUEUE_EMPTY,         // Queue is empty
    LCM_STOP_ITERATION       // Stop iteration
} lcm_status_t;

typedef float float32_t;
typedef _Bool bool_t;

#endif