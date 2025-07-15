/*
 * Copyright (c) 2024-2025, Mitchell White <mitchell.n.white@gmail.com>
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
#include <string.h>

#include "vesc_serial.h"
#include "vesc_serial_hw.h"
#include "event_queue.h"
#include "timer.h"
#include "crc16_ccitt.h"
#include "lcm_types.h"
#include "interrupts.h"
#include "tiny_math.h"

// Values from VESC datatypes.h
#define COMM_GET_VALUES_SETUP_SELECTIVE 51
#define COMM_GET_VALUES_SETUP_SELECTIVE_RESPONSE_LENGTH 16
#define COMM_GET_VALUES_SETUP_SELECTIVE_MASK 0x101b0

#ifdef ENABLE_IMU_EVENTS
#define COMM_GET_IMU_DATA 65
#define COMM_GET_IMU_DATA_RESPONSE_LENGTH 12 
#define COMM_GET_IMU_DATA_MASK 0x03 
#endif

#define SERIAL_BAUDRATE 115200U

/* The VESC serial messages are typically < 80 bytes, but 128 gives
 * a good margin. */
#define VESC_SERIAL_RX_BUFFER_SIZE 128U
#define POLLING_INTERVAL_MS 250U

#define START_BYTE 0x02
#define END_BYTE 0x03
#define MAX_PACKET_LENGTH 32
#define MAX_OUTSTANDING_PACKETS 5
#define SIGNIFICANT_CHANGE(x, y) (fabsf((x) - (y)) > 0.02f)
#define RADIANS_TO_DEGREES(radians) ((radians) * (180.0f / M_PI))

typedef struct
{
    float32_t duty_cycle;
    int32_t rpm;
#if defined(ENABLE_VOLTAGE_MONITORING)
    float32_t input_voltage;
#endif
    float32_t battery_level;
    uint8_t fault;
} comm_get_values_setup_selective_t;

#ifdef ENABLE_IMU_EVENTS
typedef struct
{
    float32_t pitch;
    float32_t roll;
} comm_get_imu_data_t;
#endif

static volatile ring_buffer_t vesc_serial_rx_buffer = {0};
static volatile uint8_t vesc_serial_rx_buffer_data[VESC_SERIAL_RX_BUFFER_SIZE] = {0};
static timer_id_t vesc_serial_tx_timerid = INVALID_TIMER_ID;
static comm_get_values_setup_selective_t comm_get_values_setup_selective = {0};
static bool_t vesc_alive = false;
static uint8_t vesc_serial_outstaning_packet_count = 0;
static vesc_serial_callback_t vesc_serial_callback = NULL;
#ifdef ENABLE_IMU_EVENTS
static comm_get_imu_data_t comm_get_imu_data = {0};
#endif

// Forward declarations
EVENT_HANDLER(vesc_serial, rx);
EVENT_HANDLER(vesc_serial, board_mode_change);
TIMER_CALLBACK(vesc_serial, tx);

/**
 * @brief Initializes the VESC serial module
 *
 * Initializes the VESC serial module by initializing the ring buffer,
 * subscribing to the VESC serial data event, and initializing the
 * comm_get_values_setup_selective struct.
 */
lcm_status_t vesc_serial_init(void)
{
    lcm_status_t status = LCM_SUCCESS;

    // Initialize the ring buffer
    vesc_serial_rx_buffer.buffer = vesc_serial_rx_buffer_data;
    vesc_serial_rx_buffer.size = VESC_SERIAL_RX_BUFFER_SIZE;
    vesc_serial_rx_buffer.read_idx = 0U;
    vesc_serial_rx_buffer.write_idx = 0U;

    // Initialize local data structures 
    memset(&comm_get_values_setup_selective, 0, sizeof(comm_get_values_setup_selective));
#ifdef ENABLE_IMU_EVENTS
    memset(&comm_get_imu_data, 0, sizeof(comm_get_imu_data));
#endif

    // Assume VESC is not alive
    vesc_alive = false;

    vesc_serial_hw_init(SERIAL_BAUDRATE);

    // Subscribe to the VESC serial data event
    SUBSCRIBE_EVENT(vesc_serial, EVENT_SERIAL_DATA_RX, rx);
    SUBSCRIBE_EVENT(vesc_serial, EVENT_BOARD_MODE_CHANGED, board_mode_change);

    return status;
}

/**
 * @brief Clears the outstanding packets count and calls the callback if set
 *
 * This function is called when the VESC serial module is no longer busy.
 * It clears the outstanding packet count and calls the callback if it is set.
 */
void clear_outstanding_packets(void)
{
    // If someone is waiting for a callback, call it now
    if (vesc_serial_callback != NULL)
    {
        vesc_serial_callback();
        vesc_serial_callback = NULL;
    }
    vesc_serial_outstaning_packet_count = 0;
}

/**
 * @brief Checks if the VESC serial is busy and sets a callback if it is
 * busy.
 */
lcm_status_t vesc_serial_check_busy_and_set_callback(vesc_serial_callback_t callback)
{
    lcm_status_t status = LCM_SUCCESS;

    // If the VESC is alive and we are busy, set the callback
    if ((vesc_alive == true) && (vesc_serial_outstaning_packet_count > 0U))
    {
        status = LCM_BUSY;
        vesc_serial_callback = callback;
    }

    return status;
}

/**
 * @brief Gets the VESC serial RX buffer
 *
 * Returns a pointer to the RX buffer ring buffer.
 *
 * @return A pointer to the RX buffer ring buffer.
 */
ring_buffer_t *vesc_serial_get_rx_buffer(void)
{
    return (ring_buffer_t *)&vesc_serial_rx_buffer;
}

/**
 * @brief Extracts a 16-bit signed integer from a buffer
 *
 * Extracts a 16-bit signed integer from the buffer, where the first byte is the
 * most significant and the second byte is the least significant.
 *
 * @param buffer The buffer to read from
 * @return The extracted integer
 */
int16_t buffer_get_int16(const uint8_t *buffer)
{
    return (int16_t)((buffer[0] << 8) | buffer[1]);
}

/**
 * @brief Extracts an unsigned 16-bit integer from a buffer
 *
 * Extracts an unsigned 16-bit integer from the buffer, where the first byte is the
 * most significant and the second byte is the least significant.
 *
 * @param buffer The buffer to read from
 * @return The extracted integer
 */
uint16_t buffer_get_uint16(const uint8_t *buffer)
{
    return (uint16_t)((buffer[0] << 8) | buffer[1]);
}

/**
 * @brief Extracts a 32-bit signed integer from a buffer
 *
 * Extracts a 32-bit signed integer from the buffer, where the first byte is the
 * most significant and the last byte is the least significant.
 *
 * @param buffer The buffer to read from
 * @return The extracted integer
 */
int32_t buffer_get_int32(const uint8_t *buffer)
{
    return (int32_t)((uint32_t)buffer[0] << 24 |
                     (uint32_t)buffer[1] << 16 |
                     (uint32_t)buffer[2] << 8  |
                     (uint32_t)buffer[3]);
}

/**
 * @brief Extracts a 32-bit unsigned integer from a buffer
 *
 * Extracts a 32-bit unsigned integer from the buffer, where the first byte is the
 * most significant and the last byte is the least significant.
 *
 * @param buffer The buffer to read from
 * @return The extracted integer
 */
uint32_t buffer_get_uint32(const uint8_t *buffer)
{
    return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}

/**
 * @brief Extracts a 16-bit float from a buffer
 *
 * Extracts a 16-bit signed integer from the buffer and scales it to a float32_t.
 *
 * @param buffer The buffer to read from
 * @param scale The scale of the float (i.e. 100.0 for a 1/100th scaling)
 * @return The scaled float
 */
float16_t buffer_get_float16(const uint8_t *buffer, float32_t scale)
{
    return (float16_t)buffer_get_int16(buffer) / scale;
}

/**
 * @brief Extracts a 32-bit float from a buffer
 *
 * Extracts a 32-bit signed integer from the buffer and scales it to a float32_t.
 *
 * @param buffer The buffer to read from
 * @param scale The scale of the float (i.e. 100.0 for a 1/100th scaling)
 * @return The scaled float
 */
float16_t buffer_get_float32(const uint8_t *buffer, float32_t scale)
{
    return (float32_t)buffer_get_int32(buffer) / scale;
}

float32_t buffer_get_float32_auto(const uint8_t *buffer)
{
    union { uint32_t i; float f; } u;
    u.i = buffer_get_uint32(buffer);
    return (float32_t)u.f;
}

#ifdef ENABLE_IMU_EVENTS
/**
 * @brief Processes a COMM_GET_IMU_DATA packet
 *
 * This function is an event handler for the EVENT_SERIAL_DATA_RX event.
 * It copies the payload into a temporary struct and then checks if each field
 * has changed. If a field has changed, it pushes an event to the event queue
 * and updates the comm_get_imu_data struct.
 * @param payload The payload of the packet
 * @param packet_length The length of the packet
 */
void process_comm_get_imu_data(const uint8_t *payload, uint8_t packet_length)
{
    comm_get_imu_data_t imu_data = {0};
    uint16_t mask = 0;

    // Expect a specific packet length for the fields selected.
    // If the packet length is incorrect, the only thing we can do is abort,
    // since we can't make any assumptions about the contents of the packet.
    if (packet_length != COMM_GET_IMU_DATA_RESPONSE_LENGTH)
    {
        fault(EMERGENCY_FAULT_INVALID_LENGTH);
        return;
    }

    // Response contains a 16-bit mask of the fields we requested
    mask = buffer_get_uint16(&payload[1]);
    if (mask != COMM_GET_IMU_DATA_MASK)
    {
        // Invalid mask
        fault(EMERGENCY_FAULT_OUT_OF_BOUNDS);
        return;
    }

    // Copy the payload into the temporary comm_get_imu_data struct
    imu_data.roll = buffer_get_float32_auto(&payload[3]);
    imu_data.pitch = buffer_get_float32_auto(&payload[7]);

    // For each field, check if the value has changed
    if SIGNIFICANT_CHANGE(imu_data.pitch, comm_get_imu_data.pitch)
    {
        event_data_t data = {0};
        data.imu_pitch = RADIANS_TO_DEGREES(imu_data.pitch);
        event_queue_push(EVENT_IMU_PITCH_CHANGED, &data);

        comm_get_imu_data.pitch = imu_data.pitch;
    }

    if SIGNIFICANT_CHANGE(imu_data.roll, comm_get_imu_data.roll)
    {
        event_data_t data = {0};
        data.imu_roll = RADIANS_TO_DEGREES(imu_data.roll);
        event_queue_push(EVENT_IMU_ROLL_CHANGED, &data);

        comm_get_imu_data.roll = imu_data.roll;
    }
}
#endif

/**
 * @brief Processes a COMM_GET_VALUES_SETUP_SELECTIVE packet
 *
 * This function is an event handler for the EVENT_SERIAL_DATA_RX event.
 * It copies the payload into a temporary struct and then checks if each field
 * has changed. If a field has changed, it pushes an event to the event queue
 * and updates the comm_get_values_setup_selective struct.
 * @param payload The payload of the packet
 * @param packet_length The length of the packet
 */
void process_comm_get_values_setup_selective(const uint8_t *payload, uint8_t packet_length)
{
    comm_get_values_setup_selective_t values = {0};
    uint32_t values_mask = 0;

    // Expect a specific packet length for the fields selected.
    // If the packet length is incorrect, the only thing we can do is abort,
    // since we can't make any assumptions about the contents of the packet.
    if (packet_length != COMM_GET_VALUES_SETUP_SELECTIVE_RESPONSE_LENGTH)
    {
        fault(EMERGENCY_FAULT_INVALID_LENGTH);
        return;
    }

    // Response contains a 32-bit mask of the fields we requested
    values_mask = buffer_get_uint32(&payload[1]);
    if (values_mask != COMM_GET_VALUES_SETUP_SELECTIVE_MASK)
    {
        // Invalid mask
        fault(EMERGENCY_FAULT_OUT_OF_BOUNDS);
        return;
    }

    // Copy the payload into the temporary comm_get_values_setup_selective
    // struct
    values.duty_cycle = buffer_get_float16(&payload[5], 10.0f);

    // Sanity check the duty cycle
    if (values.duty_cycle < -100.0f || values.duty_cycle > 100.0f)
    {
        fault(EMERGENCY_FAULT_OUT_OF_BOUNDS);
        return;
    }

    values.rpm = buffer_get_float32(&payload[7], 1.0f);

    // Sanity check the RPM (approximately 50 MPH for a 30 pole motor)
    if (values.rpm < -25000 || values.rpm > 25000)
    {
        fault(EMERGENCY_FAULT_OUT_OF_BOUNDS);
        return;
    }
#if defined(ENABLE_VOLTAGE_MONITORING)
    values.input_voltage = buffer_get_float16(&payload[11], 10.0f);
#endif
    values.battery_level = buffer_get_float16(&payload[13], 10.0f);

    // Sanity check the battery level
    if (values.battery_level < 0.0f || values.battery_level > 100.0f)
    {
        fault(EMERGENCY_FAULT_OUT_OF_BOUNDS);
        return;
    }

    values.fault = payload[15];

    // For each field, check if the value has changed
    if (values.duty_cycle != comm_get_values_setup_selective.duty_cycle)
    {
        event_data_t data = {0};
        data.duty_cycle = values.duty_cycle;
        event_queue_push(EVENT_DUTY_CYCLE_CHANGED, &data);

        comm_get_values_setup_selective.duty_cycle = values.duty_cycle;
    }

    if (values.rpm != comm_get_values_setup_selective.rpm)
    {
        event_data_t data = {0};
        data.rpm = values.rpm;
        event_queue_push(EVENT_RPM_CHANGED, &data);

        comm_get_values_setup_selective.rpm = values.rpm;
    }

#if defined(ENABLE_VOLTAGE_MONITORING)
    if (values.input_voltage != comm_get_values_setup_selective.input_voltage)
    {
        event_data_t data = {0};
        data.voltage = values.input_voltage;
        event_queue_push(EVENT_VOLTAGE_CHANGED, &data);

        comm_get_values_setup_selective.input_voltage = values.input_voltage;
    }
#endif

    if (values.battery_level != comm_get_values_setup_selective.battery_level)
    {
        event_data_t data = {0};
        data.battery_level = values.battery_level;
        event_queue_push(EVENT_BATTERY_LEVEL_CHANGED, &data);

        comm_get_values_setup_selective.battery_level = values.battery_level;
    }

    if (values.fault != comm_get_values_setup_selective.fault)
    {
        // Raise VESC faults as emergencies
        fault(EMERGENCY_FAULT_VESC);
        comm_get_values_setup_selective.fault = values.fault;
    }
}

/**
 * @brief Processes a VESC packet
 *
 * @param payload The payload of the packet. The first byte of the payload is
 *                the command ID.
 * @param packet_length The length of the packet, including the command ID.
 */
void process_packet(uint8_t *payload, uint8_t packet_length)
{
    // The first time we recieve a valid packet from the VESC, we know it is
    // alive
    if (vesc_alive == false)
    {
        event_queue_push(EVENT_VESC_ALIVE, NULL);
        vesc_alive = true;
    }

    // First byte of payload is the command ID
    switch (payload[0])
    {
    case COMM_GET_VALUES_SETUP_SELECTIVE:
        process_comm_get_values_setup_selective(payload, packet_length);
        break;
#ifdef ENABLE_IMU_EVENTS
    case COMM_GET_IMU_DATA:
        process_comm_get_imu_data(payload, packet_length);
        break;
#endif
    default:
        // Unknown command
        break;
    }
}

/**
 * @brief Handles the reception of VESC serial data
 *
 * This event handler processes incoming serial data from the VESC. It
 * searches for the start byte in the data stream, extracts the packet
 * length, payload, CRC, and end byte. If a valid packet is found, with
 * the correct CRC and end byte, the packet is processed by calling
 * process_packet().
 */
EVENT_HANDLER(vesc_serial, rx)
{
    uint8_t byte = 0;
    uint16_t crc = 0;
    uint8_t packet_length = 0;

    // Initialization should not be necessary, since we will immediately
    // overwrite the buffer, but MISRA requires it
    uint8_t payload[MAX_PACKET_LENGTH] = {0};

    // Reset the outstanding packet count
    clear_outstanding_packets();

    // Search for start byte (or end of data)
    while (byte != START_BYTE && ring_buffer_pop((ring_buffer_t *)&vesc_serial_rx_buffer, &byte))
    {
        // If we found the start byte, read the rest of the packet
        if (byte == START_BYTE)
        {
            if (!ring_buffer_pop((ring_buffer_t *)&vesc_serial_rx_buffer, &packet_length) ||
                packet_length > MAX_PACKET_LENGTH)
            {
                return;
            }

            for (uint8_t i = 0; i < packet_length; i++)
            {
                if (!ring_buffer_pop((ring_buffer_t *)&vesc_serial_rx_buffer, &payload[i]))
                {
                    return;
                }
            }

            // Next two bytes should be the CRC
            if (!ring_buffer_pop((ring_buffer_t *)&vesc_serial_rx_buffer, &byte))
            {
                return;
            }
            crc = byte << 8;
            if (!ring_buffer_pop((ring_buffer_t *)&vesc_serial_rx_buffer, &byte))
            {
                return;
            }
            crc |= byte;

            // Last byte should be the end
            if (!ring_buffer_pop((ring_buffer_t *)&vesc_serial_rx_buffer, &byte))
            {
                return;
            }

            if (byte == END_BYTE)
            {
                // Check CRC
                if (crc16_ccitt(payload, packet_length) == crc)
                {
                    // Packet is valid
                    process_packet(payload, packet_length);
                }
            }
        }
    }
}

/**
 * @brief Handles board mode changes for VESC serial communication
 *
 * This function manages the VESC serial communication behavior
 * based on the current board mode. It sets a polling timer when
 * the board is in modes that require VESC data polling and cancels
 * the timer when the board is in modes that do not require polling.
 *
 * @param data The event data containing the current board mode
 */
EVENT_HANDLER(vesc_serial, board_mode_change)
{
    switch (data->board_mode.mode)
    {
    // modes where we want to poll the VESC
    case BOARD_MODE_BOOTING:
    case BOARD_MODE_IDLE:
    case BOARD_MODE_RIDING:
        if (vesc_serial_tx_timerid == INVALID_TIMER_ID || !is_timer_active(vesc_serial_tx_timerid))
        {
            vesc_serial_tx_timerid =
                set_timer(POLLING_INTERVAL_MS, TIMER_CALLBACK_NAME(vesc_serial, tx), true);
        }
        break;

    // modes where we don't want to poll the VESC
    default:
        vesc_alive = false;
        if (vesc_serial_tx_timerid != INVALID_TIMER_ID && is_timer_active(vesc_serial_tx_timerid))
        {
            cancel_timer(vesc_serial_tx_timerid);
            vesc_serial_tx_timerid = INVALID_TIMER_ID;
        }
        break;
    }
}

/**
 * @brief Timer callback for VESC serial communication
 *
 * This function is called by the timer subsystem when the VESC serial
 * communication timer expires. It sends a packet to the VESC to poll
 * for data. The packet is hardcoded so that the timer callback is
 * as lightweight as possible.
 */
TIMER_CALLBACK(vesc_serial, tx)
{
    /*
     * This packet is used to poll the VESC for data.  Since it is
     * called repeatedly and the data never changes, we can just
     * hardcode the message to save time and reduce code complexity.
     * 
     * COMM_GET_VALUES_SETUP_SELECTIVE:
     * byte 0: start byte (0x02)
     * byte 1: packet length (0x05)
     * byte 2: command (0x33)
     * bytes 3-6: mask: 0x101b0  (u32)
     *   float 16: duty cycle now (1<<4)
     *   int 32: RPM (1<<5)
     *   float 16: input voltage (1<<7)
     *   float 16: battery level (1<<8)
     *   int 8: fault (1<<16)
     * bytes 7-8 precomputed crc-16-ccitt (0x41e6)
     * byte 9: end byte (0x03)
     * 
     * COMM_GET_IMU_DATA (optional): 
     * byte 10: start byte (0x02)
     * byte 11: packet length (0x03)
     * byte 12: command (0x41)
     * bytes 13-14: mask (0x0003) (u32)
     *  float 32: roll (1<<0)
     *  float 32: pitch (1<<1) 
     * bytes 15-16 precomputed crc-16-ccitt (0x1afe)
     * byte 17: end byte (0x03)
     */
#ifdef ENABLE_IMU_EVENTS
#define BYTE_LENGTH 20U
    uint8_t buffer[BYTE_LENGTH] = {
        0x02, 0x05, 0x33, 0x00, 0x01, 0x01, 0xb0, 0x41, 0xe6, 0x03,
        0x02, 0x03, 0x41, 0x00, 0x03, 0x1a, 0xfe, 0x03
    };
#else
#define BYTE_LENGTH 10U
    uint8_t buffer[BYTE_LENGTH] = {
        0x02, 0x05, 0x33, 0x00, 0x01, 0x01, 0xb0, 0x41, 0xe6, 0x03
    };
#endif

    if (vesc_alive == true)
    {
        // If the VESC is alive, we keep track of the number of outstanding
        // (unanswered) packets.  If we exceed the maximum number of packets,
        // we trigger a fault and assume the VESC is dead.
        //
        // Note: the outstanding packet count is also used to determine if the
        // WS2812 LED updates should be disabled.
        if (vesc_serial_outstaning_packet_count++ >= MAX_OUTSTANDING_PACKETS)
        {
            // Too many outstanding packets, trigger a fault
            fault(EMERGENCY_FAULT_VESC_COMM_TIMEOUT);
            vesc_alive = false;
            clear_outstanding_packets();
        }
    }
    vesc_serial_hw_send(buffer, BYTE_LENGTH);
}

/**
 * @brief Returns the current duty cycle of the VESC
 *
 * @return The current duty cycle of the VESC
 */
float32_t vesc_serial_get_duty_cycle(void)
{
    return comm_get_values_setup_selective.duty_cycle;
}

/**
 * @brief Returns the current RPM of the VESC
 *
 * @return The current RPM of the VESC
 */
int32_t vesc_serial_get_rpm(void)
{
    return comm_get_values_setup_selective.rpm;
}

#if defined(ENABLE_VOLTAGE_MONITORING)
/**
 * @brief Returns the current input voltage of the VESC
 *
 * @return The current input voltage of the VESC
 */
float32_t vesc_serial_get_input_voltage(void)
{
    return comm_get_values_setup_selective.input_voltage;
}
#endif

/**
 * @brief Returns the current battery level of the VESC
 *
 * @return The current battery level of the VESC
 */
float32_t vesc_serial_get_battery_level(void)
{
    return comm_get_values_setup_selective.battery_level;
}

/**
 * @brief Returns the current fault of the VESC
 *
 * @return The current fault of the VESC
 */
uint8_t vesc_serial_get_fault(void)
{
    return comm_get_values_setup_selective.fault;
}

#ifdef ENABLE_IMU_EVENTS
/**
 * @brief Returns the current pitch of the VESC IMU in degrees
 *
 * @return The current pitch of the VESC IMU
 */
float32_t vesc_serial_get_imu_pitch(void)
{
    return RADIANS_TO_DEGREES(comm_get_imu_data.pitch);
}

/**
 * @brief Returns the current roll of the VESC IMU in degrees
 *
 * @return The current roll of the VESC IMU
 */
float32_t vesc_serial_get_imu_roll(void)
{
    return RADIANS_TO_DEGREES(comm_get_imu_data.roll);
}
#endif