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
#include <stddef.h>

#include "animations.h"
#include "status_leds.h"
#include "timer.h"
#include "event_queue.h"
#include "function_generator.h"
#include "tiny_math.h"

/**
 * @brief Animation delay
 */
#define ANIMATION_DELAY 25U

/**
 * @brief Reusable structure to represent a color animation
 */
typedef struct
{
    color_mode_t mode;              /*< Animation color mode*/
    const status_leds_color_t *rgb; /*< RGB color */
    function_generator_t fg;        /*< Function generator for h */
} color_animation_t;

/**
 * @brief Reusable structure to represent a brightness animation
 */
typedef struct
{
    brightness_mode_t mode;  /*< Animation brightness mode*/
    uint8_t static_value;    /*< Static brightness value, 0-255 = 0.0-1.0 */
    function_generator_t fg; /*< Function generator for b */
} brightness_animation_t;

/**
 * @brief Structure to represent a scan animation
 *
 * The scan animation moves a gaussian distribution across the LED strip.
 * It looks similar to a laser scan, or "knight rider" if the color is
 * red. Always uses SIGMA_DEFAULT - see calculate_brightness().
 */
typedef struct
{
    status_leds_color_t *buffer; /*< LED buffer */
    color_animation_t color;     /*< Color animation */
    scan_direction_t direction;  /*< Scan direction */
    function_generator_t fg;     /*< Function generator for mu */
} scan_animation_t;

/**
 * @brief Structure to represent a fill animation
 *
 * This animation fills a range of LEDs with a color and brightness.
 * The color and brightness can be animated. Example use cases include:
 * - Rotating a color wheel
 * - Fading in and out
 * - Flashing a color for warnings, errors, etc
 */
typedef struct
{
    status_leds_color_t *buffer;       /*< LED buffer */
    uint8_t first_led;                 /*< First LED index */
    uint8_t last_led;                  /*< Last LED index */
    color_animation_t color;           /*< Color animation */
    brightness_animation_t brightness; /*< Brightness animation */
    fill_mode_t mode;                  /*< Fill mode */
} fill_animation_t;

/**
 * @brief Structure to represent a fade animation
 */
typedef struct
{
    status_leds_color_t *buffer; /*< LED buffer */
    uint16_t period_ms;
    uint16_t elapsed_ms;
    animation_callback_t callback;
} fade_animation_t;

#ifdef ENABLE_FIRE_ANIMATION
typedef struct
{
    status_leds_color_t *buffer;
    uint8_t heat[STATUS_LEDS_COUNT];
    uint8_t prng_state;
} fire_animation_t;
#endif

/**
 * @brief Union to represent the animation configuration
 *
 * Since only one animation can be active at a time, we use a union to
 * store the configuration and save memory.
 */
typedef union {
    scan_animation_t scan;
    fill_animation_t fill;
    fade_animation_t fade;
#ifdef ENABLE_FIRE_ANIMATION
    fire_animation_t fire;
#endif
} animation_config_t;

typedef void (*animation_tick_t)(uint32_t tick);

static timer_id_t animation_timer = INVALID_TIMER_ID;
static animation_config_t animation_config = {0};
static animation_tick_t timer_callback = NULL;
static uint16_t animation_id = 1U;         // Next ID to assign; 0 is reserved as "no animation"
static uint16_t current_animation_id = 0U; // ID of the animation currently running

// Each animation is implemented as a timer callback
TIMER_CALLBACK(animation, tick);

/**
 * @brief Gaussian brightness falloff, indexed by |i - mu| in 1/16-LED steps
 * (16 steps/LED keeps scan motion visually smooth). Computed offline for
 * SIGMA_DEFAULT (0.7) - the only sigma value scan_animation_setup() is ever
 * called with (previously a sigma parameter threaded this value through at
 * runtime; since it never varied, this LUT replaces the runtime
 * exp()/tiny_expf() call entirely). Distances beyond the table (>= 2.5 LEDs)
 * are ~0 and not stored.
 */
static const uint8_t brightness_falloff_lut[40] = {
    255, 254, 251, 246, 239, 231, 221, 210,
    198, 185, 171, 157, 144, 130, 117, 104,
    92,  81,  70,  60,  52,  44,  37,  31,
    26,  21,  17,  14,  11,  9,   7,   6,
    4,   3,   3,   2,   1,   1,   1,   1,
};

/**
 * @brief Calculate the brightness of a status LED.
 *
 * @param mu The mean value or center of the distribution (Q16.16 LED index).
 * @param i The index of the LED for which brightness is being calculated.
 * @return The calculated brightness, 0-255 = 0.0-1.0.
 */
static uint8_t calculate_brightness(fixed16_t mu, uint8_t i)
{
    fixed16_t distance = ((fixed16_t)i << 16) - mu;
    uint32_t abs_distance = (uint32_t)((distance < 0) ? -distance : distance);
    uint32_t idx = abs_distance >> 12; // 1/16-LED steps (65536 / 16 = 4096)

    if (idx >= 40U)
    {
        return 0U;
    }
    return brightness_falloff_lut[idx];
}

/**
 * @brief Starts the animation timer with the specified callback.
 */
void animation_start(animation_tick_t callback)
{
    if (callback != NULL)
    {
        timer_callback = callback;
        animation_timer = set_timer(ANIMATION_DELAY, TIMER_CALLBACK_NAME(animation, tick), true);
    }
    else
    {
        fault(EMERGENCY_FAULT_NULL_POINTER);
    }
}

/**
 * @brief mu falloff distance: sigma * sqrt(-2*sigma^2*ln(threshold)).
 *
 * Was a runtime sqrt()/log() call; scan_animation_setup() only ever calls it
 * with SIGMA_DEFAULT (0.7) and a threshold of 0.01, so this is the
 * precomputed constant for that one case instead (folded at compile time,
 * verified against the original formula: sigma=0.7, threshold=0.01 ->
 * 1.4870785867974432).
 */
#define MU_FALLOFF FIXED16(1.4870785867974432)

/**
 * @brief Converts HSL color values to RGB color values.
 *
 * This function takes in hue, saturation, and lightness (HSL) values and
 * converts them to red, green, and blue (RGB) values. The hue value is
 * expected to be in the range [0, 360) degrees, while the saturation and
 * lightness values should be in the range [0, 1]. The resulting RGB values
 * are stored in a status_leds_color_t struct with each component scaled
 * to the range [0, 255].
 *
 * @param h The hue component of the color in degrees.
 * @param s The saturation component of the color.
 * @param l The lightness component of the color.
 * @param color Pointer to a status_leds_color_t struct to store the resulting
 * RGB values.
 */
/**
 * @brief Absolute value of a Q16.16 fixed-point value.
 */
static fixed16_t fixed_abs16(fixed16_t x)
{
    return (x < 0) ? -x : x;
}

void hsl_to_rgb(fixed16_t h, uint8_t s, uint8_t l, status_leds_color_t *color)
{
    if (color != NULL)
    {
        fixed16_t s16 = frac8_to_fixed16(s);
        fixed16_t l16 = frac8_to_fixed16(l);
        fixed16_t c, x, m, h_prime;
        fixed16_t r = 0, g = 0, b = 0;
        int32_t sector;

        while (h >= FIXED16(360.0))
        {
            h -= FIXED16(360.0);
        }
        while (h < 0)
        {
            h += FIXED16(360.0);
        }

        // Chroma = (1 - |2l - 1|) * s
        c = fixed_mul16(FIXED16(1.0) - fixed_abs16((l16 << 1) - FIXED16(1.0)), s16);

        // h/60 ("hours") mod 2, then second largest component = c * (1 - |h_prime - 1|).
        // Division by 60 is a compile-time constant - folds to a multiply, no idiv call.
        h_prime = h / 60;
        while (h_prime >= FIXED16(2.0))
        {
            h_prime -= FIXED16(2.0);
        }
        x = fixed_mul16(c, FIXED16(1.0) - fixed_abs16(h_prime - FIXED16(1.0)));
        m = l16 - (c >> 1);

        sector = (h >> 16) / 60; // 0-5, division by a constant is free
        switch (sector)
        {
        case 0:
            r = c, g = x, b = 0;
            break;
        case 1:
            r = x, g = c, b = 0;
            break;
        case 2:
            r = 0, g = c, b = x;
            break;
        case 3:
            r = 0, g = x, b = c;
            break;
        case 4:
            r = x, g = 0, b = c;
            break;
        default: // sector 5
            r = c, g = 0, b = x;
            break;
        }

        // fixed16_to_frac8() clamps, which also makes this safe for a
        // negative r/g/b+m (which the original float version's unclamped
        // (uint8_t) cast did not handle).
        color->r = fixed16_to_frac8(r + m);
        color->g = fixed16_to_frac8(g + m);
        color->b = fixed16_to_frac8(b + m);
    }
}

/**
 * @brief Updates the color based on the current animation mode.
 *
 * This function calculates the next color for the status LEDs based on the
 * specified color animation mode. It supports three modes: HSV increase, HSV
 * sine wave, and RGB. For HSV modes, it converts the hue to RGB values and
 * updates the `color`. For RGB mode, it directly sets the `color` from the
 * provided RGB values in the animation. If any input pointers are NULL, it
 * triggers an emergency fault.
 *
 * @param color Pointer to a status_leds_color_t struct to store the updated
 * RGB values.
 * @param color_animation Pointer to a color_animation_t struct containing the
 * current animation state and parameters.
 */
void next_color(status_leds_color_t *color, color_animation_t *color_animation)
{
    fixed16_t h = 0;

    if (color != NULL && color_animation != NULL)
    {
        switch (color_animation->mode)
        {
        case COLOR_MODE_HSV_SQUARE:
            // Fall-through intentional
        case COLOR_MODE_HSV_SINE:
            // Fall-through intentional
        case COLOR_MODE_HSV_INCREASE:
            // Fall-through intentional
        case COLOR_MODE_HSV_DECREASE:
            if (LCM_SUCCESS != function_generator_next_sample(&(color_animation->fg), &h))
            {
                // This should never return false unless we forgot
                // to enable repeating
                fault(EMERGENCY_FAULT_INVALID_ARGUMENT);
            }
            hsl_to_rgb(h, SATURATION_DEFAULT, LIGHTNESS_DEFAULT, color);
            break;
        case COLOR_MODE_RGB:
            if (color_animation->rgb != NULL)
            {
                color->r = color_animation->rgb->r;
                color->g = color_animation->rgb->g;
                color->b = color_animation->rgb->b;
            }
            else
            {
                fault(EMERGENCY_FAULT_NULL_POINTER);
            }
            break;
        default:
            fault(EMERGENCY_FAULT_INVALID_ARGUMENT);
            break;
        }
    }
    else
    {
        fault(EMERGENCY_FAULT_NULL_POINTER);
    }
}

/**
 * @brief Initializes the brightness animation with the specified parameters.
 */
void brightness_init(brightness_animation_t *brightness_animation,
                     brightness_mode_t brightness_mode, fixed16_t brightness_min,
                     fixed16_t brightness_max, uint32_t brightness_speed,
                     uint16_t brightness_sequence)
{
    if (brightness_animation != NULL)
    {
        brightness_animation->mode = brightness_mode;
        switch (brightness_mode)
        {
        case BRIGHTNESS_MODE_FLASH:
            function_generator_init(&(brightness_animation->fg), FUNCTION_GENERATOR_SQUARE,
                                    brightness_speed, ANIMATION_DELAY, brightness_min,
                                    brightness_max, FG_FLAG_REPEAT | FG_FLAG_INVERT, 0);
            break;
        case BRIGHTNESS_MODE_SINE:
            function_generator_init(&(brightness_animation->fg), FUNCTION_GENERATOR_SINE,
                                    brightness_speed, ANIMATION_DELAY, brightness_min,
                                    brightness_max, FG_FLAG_REPEAT, 0);
            break;
        case BRIGHTNESS_MODE_STATIC:
            brightness_animation->static_value = fixed16_to_frac8(brightness_max);
            break;
        case BRIGHTNESS_MODE_FADE:
            function_generator_init(&(brightness_animation->fg), FUNCTION_GENERATOR_SAWTOOTH,
                                    brightness_speed, ANIMATION_DELAY, brightness_min,
                                    brightness_max, FG_FLAG_INVERT | FG_FLAG_REPEAT, 0);
            break;
        case BRIGHTNESS_MODE_SEQUENCE:
            function_generator_init(&(brightness_animation->fg), FUNCTION_GENERATOR_SEQUENCE,
                                    brightness_speed, ANIMATION_DELAY, brightness_min,
                                    brightness_max, FG_FLAG_REPEAT, brightness_sequence);
            break;
        default:
            fault(EMERGENCY_FAULT_INVALID_ARGUMENT);
            break;
        }
    }
    else
    {
        fault(EMERGENCY_FAULT_NULL_POINTER);
    }
}

/**
 * @brief Calculates the next brightness value for the animation.
 */
uint8_t next_brightness(brightness_animation_t *brightness_animation)
{
    uint8_t b = 0U;

    if (brightness_animation != NULL)
    {
        if (brightness_animation->mode == BRIGHTNESS_MODE_STATIC)
        {
            b = brightness_animation->static_value;
        }
        else
        {
            fixed16_t sample = 0;
            if (LCM_SUCCESS != function_generator_next_sample(&(brightness_animation->fg), &sample))
            {
                fault(EMERGENCY_FAULT_INVALID_ARGUMENT);
            }
            b = fixed16_to_frac8(sample);
        }
    }
    else
    {
        fault(EMERGENCY_FAULT_NULL_POINTER);
    }

    return b;
}

/**
 * @brief Initializes the color animation with the specified parameters.
 *
 * This function sets up the color animation parameters in the
 * color_animation_t struct, and initializes the sine wave if necessary.
 *
 * @param color_animation Pointer to a color_animation_t struct to store the
 * color animation parameters.
 * @param color_mode The mode of the color animation (HSV increase, HSV sine
 * wave, or RGB).
 * @param hue_min The minimum hue value in degrees.
 * @param hue_max The maximum hue value in degrees.
 * @param color_speed The speed of the color animation in degrees per
 * ANIMATION_DELAY.
 * @param rgb Pointer to a status_leds_color_t struct containing the RGB values
 * for the color animation (used for RGB mode only).
 */
void color_init(color_animation_t *color_animation, color_mode_t color_mode, fixed16_t hue_min,
                fixed16_t hue_max, uint32_t color_speed, const status_leds_color_t *rgb)
{
    if (color_animation != NULL)
    {
        color_animation->mode = color_mode;

        switch (color_mode)
        {
        case COLOR_MODE_HSV_INCREASE:
            function_generator_init(&(color_animation->fg), FUNCTION_GENERATOR_SAWTOOTH,
                                    color_speed, ANIMATION_DELAY, hue_min, hue_max, FG_FLAG_REPEAT,
                                    0);
            break;
        case COLOR_MODE_HSV_DECREASE:
            function_generator_init(&(color_animation->fg), FUNCTION_GENERATOR_SAWTOOTH,
                                    color_speed, ANIMATION_DELAY, hue_min, hue_max,
                                    FG_FLAG_INVERT | FG_FLAG_REPEAT, 0);
            break;
        case COLOR_MODE_HSV_SINE:
            function_generator_init(&(color_animation->fg), FUNCTION_GENERATOR_SINE, color_speed,
                                    ANIMATION_DELAY, hue_min, hue_max, FG_FLAG_REPEAT, 0);
            break;
        case COLOR_MODE_HSV_SQUARE:
            function_generator_init(&(color_animation->fg), FUNCTION_GENERATOR_SQUARE, color_speed,
                                    ANIMATION_DELAY, hue_min, hue_max, FG_FLAG_REPEAT, 0);
            break;
        case COLOR_MODE_RGB:
            if (rgb != NULL)
            {
                color_animation->rgb = rgb;
            }
            else
            {
                fault(EMERGENCY_FAULT_NULL_POINTER);
            }
            break;
        default:
            fault(EMERGENCY_FAULT_INVALID_ARGUMENT);
            break;
        }
    }
    else
    {
        fault(EMERGENCY_FAULT_NULL_POINTER);
    }
}

/**
 * @brief Scales the brightness of the given color
 *
 * This function scales the RGB values of the given color by the given
 * brightness. The brightness is a float between 0.0f and 1.0f, where 0.0f
 * represents black and 1.0f represents the original color.
 *
 * @param color Pointer to a status_leds_color_t struct to scale the brightness
 * of.
 * @param brightness The brightness to scale the color to, 0-255 = 0.0-1.0.
 */
void scale_brightness(status_leds_color_t *color, uint8_t brightness)
{
    if (color != NULL)
    {
        color->r = scale8(color->r, brightness);
        color->g = scale8(color->g, brightness);
        color->b = scale8(color->b, brightness);
    }
}

/**
 * @brief Fills the specified range of LEDs with a gradient color.
 */
void gradient_fill(status_leds_color_t *buffer, color_animation_t *color_animation,
                   uint8_t first_led, uint8_t last_led, uint8_t brightness)
{
    if (color_animation == NULL || color_animation->mode == COLOR_MODE_RGB)
    {
        fault(EMERGENCY_FAULT_NULL_POINTER);
        return;
    }

    status_leds_color_t color = {0};
    fixed16_t h = 0;

    int8_t step = (first_led <= last_led) ? 1 : -1;
    uint8_t count = (first_led <= last_led) ? (last_led - first_led + 1) : (first_led - last_led + 1);

    next_color(&color, color_animation);

    for (uint8_t idx = 0; idx < count; idx++)
    {
        uint8_t i = first_led + step * idx;
        if (LCM_SUCCESS != function_generator_peek_sample(&(color_animation->fg), &h, idx))
        {
            fault(EMERGENCY_FAULT_INVALID_ARGUMENT);
        }

        hsl_to_rgb(h, SATURATION_DEFAULT, LIGHTNESS_DEFAULT, &color);
        scale_brightness(&color, brightness);

        buffer[i].r = color.r;
        buffer[i].g = color.g;
        buffer[i].b = color.b;
    }
}

/**
 * @brief Ticks the fill animation
 */
void fill_animation_tick(uint32_t tick)
{
    status_leds_color_t color = {0};
    uint8_t midpoint = 0;
    uint8_t b = 0U;

    // Clear the LEDs
    status_leds_set_color(&color, 0, STATUS_LEDS_COUNT - 1);

    // Get the next brightness
    b = next_brightness(&animation_config.fill.brightness);

    // Update the LEDs
    switch (animation_config.fill.mode)
    {
    case FILL_MODE_SOLID:
        // Solid fill is simply setting all the LEDs to the same color
        next_color(&color, &animation_config.fill.color);
        scale_brightness(&color, b);
        status_leds_set_color(&color, animation_config.fill.first_led,
                              animation_config.fill.last_led);
        break;
    case FILL_MODE_HSV_GRADIENT:
        gradient_fill(animation_config.fill.buffer, &animation_config.fill.color,
                      animation_config.fill.first_led, animation_config.fill.last_led, b);
        break;
    case FILL_MODE_HSV_GRADIENT_MIRROR:
        // Mirror mode is similar to gradient mode, but we mirror the left
        // and right sides of the area
        midpoint = (animation_config.fill.first_led + animation_config.fill.last_led) / 2;

        // Fill up to the midpoint
        gradient_fill(animation_config.fill.buffer, &animation_config.fill.color,
                      animation_config.fill.first_led, midpoint, b);

        // Reflect the other side. Works for any first_led/last_led and
        // either parity: buffer[i] mirrors the LED equidistant from the
        // other end of the range.
        for (uint8_t i = midpoint + 1; i <= animation_config.fill.last_led; i++)
        {
            uint8_t mirror_i =
                animation_config.fill.first_led + animation_config.fill.last_led - i;
            animation_config.fill.buffer[i].r = animation_config.fill.buffer[mirror_i].r;
            animation_config.fill.buffer[i].g = animation_config.fill.buffer[mirror_i].g;
            animation_config.fill.buffer[i].b = animation_config.fill.buffer[mirror_i].b;
        }
        break;
    default:
        break;
    }
    status_leds_refresh();
}

/**
 * @brief Ticks the scan animation
 *
 * Updates the scan animation by
 * 1. Updating the position of the gaussian distribution (mu)
 * 2. Updating the hue
 * 3. Updating the LEDs
 *
 */
void scan_animation_tick(uint32_t tick)
{
    status_leds_color_t color = {0};
    fixed16_t mu = 0;
    bool mirror = (animation_config.scan.direction == SCAN_DIRECTION_LEFT_TO_RIGHT_MIRROR) ||
                  (animation_config.scan.direction == SCAN_DIRECTION_RIGHT_TO_LEFT_MIRROR);

    // Update animation parameters
    if (LCM_SUCCESS != function_generator_next_sample(&animation_config.scan.fg, &mu))
    {
        // No more samples, disable animation. The buffer already holds the
        // final rendered frame - don't render another one with mu reset.
        stop_animation();
        return;
    }
    next_color(&color, &animation_config.scan.color);

    // Step 3: Update the LEDs
    for (uint8_t i = 0; i < STATUS_LEDS_COUNT; i++)
    {
        uint8_t brightness = 0U;
        fixed16_t i_fixed = (fixed16_t)i << 16;

        // Calculate the brightness based on the distance from the center
        if (animation_config.scan.direction == SCAN_DIRECTION_LEFT_TO_RIGHT_FILL && i_fixed < mu)
        {
            brightness = 255U;
        }
        else if (animation_config.scan.direction == SCAN_DIRECTION_RIGHT_TO_LEFT_FILL &&
                 i_fixed > mu)
        {
            brightness = 255U;
        }
        else
        {
            brightness = calculate_brightness(mu, i);
        }

        if (animation_config.scan.buffer != NULL)
        {
            animation_config.scan.buffer[i].r = scale8(color.r, brightness);
            animation_config.scan.buffer[i].g = scale8(color.g, brightness);
            animation_config.scan.buffer[i].b = scale8(color.b, brightness);
        }
        else
        {
            // Error
            fault(EMERGENCY_FAULT_NULL_POINTER);
        }
    }

    // Step 4: mirror the left and right sides of the area
    if (mirror)
    {
        for (uint8_t i = 0; i < STATUS_LEDS_COUNT / 2; i++)
        {
            animation_config.scan.buffer[STATUS_LEDS_COUNT - 1 - i].r =
                animation_config.scan.buffer[i].r;
            animation_config.scan.buffer[STATUS_LEDS_COUNT - 1 - i].g =
                animation_config.scan.buffer[i].g;
            animation_config.scan.buffer[STATUS_LEDS_COUNT - 1 - i].b =
                animation_config.scan.buffer[i].b;
        }
    }
    status_leds_refresh();
}

/**
 * @brief Ticks the fade animation
 */
void fade_animation_tick(uint32_t tick)
{
    animation_config.fade.elapsed_ms += ANIMATION_DELAY;

    if (animation_config.fade.elapsed_ms >= animation_config.fade.period_ms)
    {
        // Fade animation is complete
        for (uint8_t i = 0; i < STATUS_LEDS_COUNT; i++)
        {
            animation_config.fade.buffer[i].r = 0;
            animation_config.fade.buffer[i].g = 0;
            animation_config.fade.buffer[i].b = 0;
        }
        status_leds_refresh();

        // Stop the timer
        cancel_timer(animation_timer);

        // Call the callback
        if (animation_config.fade.callback != NULL)
        {
            animation_config.fade.callback();
        }
    }
    else
    {
        // Update the LEDs. period_ms is a runtime (not compile-time
        // constant) divisor, so this is a real unsigned divide - already
        // linked in elsewhere in the firmware, so free.
        uint8_t fade_factor = (uint8_t)(255U - (((uint32_t)animation_config.fade.elapsed_ms * 255U) /
                                                animation_config.fade.period_ms));

        for (uint8_t i = 0; i < STATUS_LEDS_COUNT; i++)
        {
            animation_config.fade.buffer[i].r = scale8(animation_config.fade.buffer[i].r, fade_factor);
            animation_config.fade.buffer[i].g = scale8(animation_config.fade.buffer[i].g, fade_factor);
            animation_config.fade.buffer[i].b = scale8(animation_config.fade.buffer[i].b, fade_factor);
        }

        status_leds_refresh();
    }
}

#ifdef ENABLE_FIRE_ANIMATION
/**
 * @brief Ticks the fire animation
 */
void fire_animation_tick(uint32_t tick)
{
    animation_config.fire.prng_state = (animation_config.fire.prng_state ^ tick) % 256;

    // Update the fire animation
    for (uint8_t i = 0; i < STATUS_LEDS_COUNT; i++)
    {
        // Cool down every LED a little
        animation_config.fire.heat[i] = qsub8(animation_config.fire.heat[i],
                                              prng(&animation_config.fire.prng_state, 0, 22) + 2);
    }

    // Heat from each cell drifts 'up' and diffuses a little
    for (uint8_t i = STATUS_LEDS_COUNT - 1; i >= 2; i--)
    {
        animation_config.fire.heat[i] =
            (animation_config.fire.heat[i - 1] + animation_config.fire.heat[i - 2] +
             animation_config.fire.heat[i - 2]) /
            3;
    }

    // Add sparking
    if (prng(&animation_config.fire.prng_state, 0, 255) < 45)
    {
        uint8_t y = prng(&animation_config.fire.prng_state, 0, 1);
        animation_config.fire.heat[y] =
            qadd8(animation_config.fire.heat[y], prng(&animation_config.fire.prng_state, 160, 255));
    }

    for (uint8_t i = 0; i < STATUS_LEDS_COUNT; i++)
    {
        // Map from heat to LED colors
        uint8_t t192 = scale8(animation_config.fire.heat[i], 192);
        uint8_t heatramp = t192 & 0x3F; // 0..63
        heatramp <<= 2;                 // scale up to 0..252

        if (t192 & 0x80)
        {
            animation_config.fire.buffer[i].r = 200;
            animation_config.fire.buffer[i].g = 200;
            animation_config.fire.buffer[i].b = heatramp / 3;
        }
        else if (t192 & 0x40)
        {
            animation_config.fire.buffer[i].r = 255;
            animation_config.fire.buffer[i].g = heatramp;
            animation_config.fire.buffer[i].b = 0;
        }
        else
        {
            // just the heatramp
            animation_config.fire.buffer[i].r = heatramp;
            animation_config.fire.buffer[i].g = 0;
            animation_config.fire.buffer[i].b = 0;
        }
    }

    // Refresh the LEDs
    status_leds_refresh();
}
#endif

/**
 * @brief Initializes the scan animation with the specified parameters.
 */
uint16_t scan_animation_setup(status_leds_color_t *buffer, scan_direction_t direction,
                              color_mode_t color_mode, uint32_t movement_speed,
                              fixed16_t hue_min, fixed16_t hue_max, uint32_t color_speed,
                              scan_start_t scan_start, scan_end_t scan_end, fixed16_t init_mu,
                              const status_leds_color_t *rgb)
{
    fixed16_t mu_start = 0;
    fixed16_t mu_end = FIXED16(STATUS_LEDS_COUNT - 1) + MU_FALLOFF;

    if (scan_start == SCAN_START_DEFAULT)
    {
        mu_start = -MU_FALLOFF;
    }
    else
    {
        mu_start = init_mu;
    }

    // Copy the animation configuration
    animation_config.scan.buffer = buffer;
    animation_config.scan.direction = direction;

    switch (direction)
    {
    case SCAN_DIRECTION_LEFT_TO_RIGHT_MIRROR:
        mu_end = FIXED16((STATUS_LEDS_COUNT / 2) - 1) + MU_FALLOFF;
        // fallthrough intentional
    case SCAN_DIRECTION_LEFT_TO_RIGHT_FILL:
        // fallthrough intentional
    case SCAN_DIRECTION_LEFT_TO_RIGHT:
        function_generator_init(&(animation_config.scan.fg), FUNCTION_GENERATOR_SAWTOOTH,
                                movement_speed, ANIMATION_DELAY, mu_start, mu_end,
                                scan_end == SCAN_END_NEVER ? FG_FLAG_REPEAT : FG_FLAG_NONE, 0);
        break;
    case SCAN_DIRECTION_RIGHT_TO_LEFT_MIRROR:
        mu_end = FIXED16((STATUS_LEDS_COUNT / 2) - 1) + MU_FALLOFF;
        // fallthrough intentional
    case SCAN_DIRECTION_RIGHT_TO_LEFT_FILL:
        // fallthrough intentional
    case SCAN_DIRECTION_RIGHT_TO_LEFT:
        function_generator_init(
            &(animation_config.scan.fg), FUNCTION_GENERATOR_SAWTOOTH, movement_speed,
            ANIMATION_DELAY, mu_start, mu_end,
            scan_end == SCAN_END_NEVER ? FG_FLAG_REPEAT | FG_FLAG_INVERT : FG_FLAG_INVERT, 0);
        break;
    case SCAN_DIRECTION_SINE:
        function_generator_init(&(animation_config.scan.fg), FUNCTION_GENERATOR_SINE,
                                movement_speed, ANIMATION_DELAY, FIXED16(0.0),
                                FIXED16(STATUS_LEDS_COUNT - 1),
                                scan_end == SCAN_END_NEVER ? FG_FLAG_REPEAT : FG_FLAG_NONE, 0);
        break;
    default:
        fault(EMERGENCY_FAULT_INVALID_ARGUMENT);
        break;
    }

    // Initialize the color animation
    color_init(&animation_config.scan.color, color_mode, hue_min, hue_max, color_speed, rgb);

    // If single tick, just run tick handler without starting timer
    if (scan_end == SCAN_END_SINGLE_TICK)
    {
        scan_animation_tick(0);
    }
    else
    {
        // Start animation timer
        animation_start(scan_animation_tick);
    }

    // Return the animation ID
    current_animation_id = animation_id++;
    return current_animation_id;
}

/**
 * @brief Set up a fill animation.
 */
uint16_t fill_animation_setup(status_leds_color_t *buffer, color_mode_t color_mode,
                              brightness_mode_t brightness_mode, fill_mode_t fill_mode,
                              uint8_t first_led, uint8_t last_led, fixed16_t hue_min,
                              fixed16_t hue_max, uint32_t color_speed, fixed16_t brightness_min,
                              fixed16_t brightness_max, uint32_t brightness_speed,
                              uint16_t brightness_sequence, const status_leds_color_t *rgb)
{
    // Copy the animation configuration
    animation_config.fill.buffer = buffer;
    animation_config.fill.first_led = first_led;
    animation_config.fill.last_led = last_led;
    animation_config.fill.mode = fill_mode;

    // Initialize the color and brightness animations
    color_init(&animation_config.fill.color, color_mode, hue_min, hue_max, color_speed, rgb);
    brightness_init(&animation_config.fill.brightness, brightness_mode, brightness_min,
                    brightness_max, brightness_speed, brightness_sequence);

    animation_start(fill_animation_tick);

    // Return the animation ID
    current_animation_id = animation_id++;
    return current_animation_id;
}

/**
 * @brief Set up a fade animation.
 */
uint16_t fade_animation_setup(status_leds_color_t *buffer, uint16_t period,
                              animation_callback_t callback)
{
    animation_config.fade.buffer = buffer;
    animation_config.fade.period_ms = period;
    animation_config.fade.elapsed_ms = 0;
    animation_config.fade.callback = callback;

    animation_start(fade_animation_tick);

    // Return the animation ID
    current_animation_id = animation_id++;
    return current_animation_id;
}

#ifdef ENABLE_FIRE_ANIMATION
uint16_t fire_animation_setup(status_leds_color_t *buffer)
{
    animation_config.fire.buffer = buffer;
    animation_config.fire.prng_state = 123;
    for (uint8_t i = 0; i < STATUS_LEDS_COUNT; i++)
    {
        animation_config.fire.heat[i] = 0;
    }

    animation_start(fire_animation_tick);

    // Return the animation ID
    current_animation_id = animation_id++;
    return current_animation_id;
}
#endif

/**
 * @brief Handle the timer tick for the animations.
 */
TIMER_CALLBACK(animation, tick)
{
    if (timer_callback != NULL)
    {
        timer_callback(system_tick);
    }
    else
    {
        // This should never happen unless the timer callback is not set
        cancel_timer(animation_timer);
        animation_timer = INVALID_TIMER_ID;
    }
}

/**
 * @brief Stops the current animation.
 *
 * This function cancels the active animation timer if it is valid and active,
 * and sets the current animation state to ANIMATION_NONE, effectively stopping
 * any ongoing animation.
 */
void stop_animation(void)
{
    if (animation_timer != INVALID_TIMER_ID && is_timer_active(animation_timer))
    {
        cancel_timer(animation_timer);
        animation_timer = INVALID_TIMER_ID;
    }
    timer_callback = NULL;
}

uint16_t get_animation_id(void)
{
    return current_animation_id;
}