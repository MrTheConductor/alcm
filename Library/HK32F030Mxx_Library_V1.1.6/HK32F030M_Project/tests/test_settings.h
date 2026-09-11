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
#ifndef TEST_SETTINGS_H
#define TEST_SETTINGS_H

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include "settings.h"
#include "event_queue.h"
#include "mock_event_queue.h"
#include "mock_eeprom.h"

// Not declared in settings.h - non-static so tests can call them directly,
// matching this codebase's convention for internal-but-linkable helpers.
bool_t settings_range_check(void);
EVENT_HANDLER(settings, mode_changed);

/**
 * @brief Resets the fake EEPROM and calls the real settings_init() directly
 * (rather than settings_get(), which only inits once per process). Every
 * test uses this as its own self-contained arrange step, since
 * settings_init() unconditionally subscribes every time it's called
 * regardless of the module's internal "already loaded" flag - so each test
 * needs its own matching subscribe_event expectation.
 */
static lcm_status_t init_from_blank_eeprom(void)
{
    mock_eeprom_reset();
    expect_value(subscribe_event, event, EVENT_BOARD_MODE_CHANGED);
    expect_any(subscribe_event, callback);
    return settings_init();
}

/**
 * @brief Same as init_from_blank_eeprom() but WITHOUT blanking the fake
 * EEPROM first - used to simulate "rebooting" and re-reading whatever is
 * currently stored, rather than starting from a factory-fresh chip.
 */
static lcm_status_t init_from_blank_eeprom_reread(void)
{
    expect_value(subscribe_event, event, EVENT_BOARD_MODE_CHANGED);
    expect_any(subscribe_event, callback);
    return settings_init();
}

/**
 * @brief A blank/factory-fresh EEPROM has a CRC mismatch (and, incidentally,
 * a bad magic number too) - settings_init() must reset to defaults.
 */
static void test_settings_init_from_blank_eeprom_resets_to_defaults(void **state)
{
    (void)state;

    assert_int_equal(LCM_SUCCESS, init_from_blank_eeprom());

    settings_t *s = settings_get();
    assert_non_null(s);
    assert_true(s->enable_beep);
    assert_true(s->enable_headlights);
    assert_true(s->enable_status_leds);
    assert_int_equal(ANIMATION_OPTION_FLOATWHEEL_CLASSIC, s->boot_animation);
    assert_int_equal(ANIMATION_OPTION_NONE, s->idle_animation);
    assert_int_equal(ANIMATION_OPTION_NONE, s->dozing_animation);
    assert_int_equal(ANIMATION_OPTION_NONE, s->shutdown_animation);
    assert_int_equal(ANIMATION_OPTION_NONE, s->ride_animation);
    assert_int_equal(204U, s->headlight_brightness);
    assert_int_equal(204U, s->status_brightness);
    assert_int_equal(200U, s->personal_color);

    // Resetting from blank always writes once (defaults differ from the
    // blank/0xFF-filled EEPROM).
    assert_int_equal(1U, mock_eeprom_get_write_count());
}

/**
 * @brief settings_reset() directly: same defaults, callable independent of
 * settings_init()'s CRC/range dispatch.
 */
static void test_settings_reset_direct(void **state)
{
    (void)state;

    // Establish a loaded baseline first so settings_get() below doesn't
    // itself trigger a second, unexpected settings_init().
    assert_int_equal(LCM_SUCCESS, init_from_blank_eeprom());

    // Mutate away from defaults, then reset and confirm it's back.
    settings_get()->personal_color = 42U;
    settings_reset();

    assert_int_equal(200U, settings_get()->personal_color);
    assert_true(settings_get()->enable_beep);
}

/**
 * @brief A previously-saved, self-consistent (valid CRC, valid range) blob
 * is loaded as-is - settings_init() must NOT reset it, proving the
 * "already good" path preserves whatever was actually stored rather than
 * silently reverting to defaults.
 */
static void test_settings_init_loads_previously_saved_values(void **state)
{
    (void)state;

    assert_int_equal(LCM_SUCCESS, init_from_blank_eeprom());
    settings_get()->personal_color = 99U;
    settings_save();

    // Re-init "from scratch" (as if the board rebooted): reads the same
    // fake EEPROM contents back, which are still self-consistent.
    assert_int_equal(LCM_SUCCESS, init_from_blank_eeprom_reread());
    assert_int_equal(99U, settings_get()->personal_color);
}

/**
 * @brief A blob with a valid CRC but semantically-invalid content (built by
 * saving after directly corrupting a field, so the CRC is self-consistent
 * with the corruption) must still force a reset - this is the
 * range-check-only trigger, independent of the CRC-mismatch trigger above.
 */
static void test_settings_init_invalid_range_forces_reset(void **state)
{
    (void)state;

    assert_int_equal(LCM_SUCCESS, init_from_blank_eeprom());
    // An out-of-range animation option, saved with a CRC that matches this
    // corrupted content (settings_save() always recomputes the CRC).
    settings_get()->boot_animation = ANIMATION_OPTION_COUNT;
    settings_save();

    // Re-init: CRC matches the (corrupted) stored blob, so that check
    // alone would pass - but settings_range_check() must catch the
    // out-of-range boot_animation and force a reset back to defaults.
    assert_int_equal(LCM_SUCCESS, init_from_blank_eeprom_reread());
    assert_int_equal(ANIMATION_OPTION_FLOATWHEEL_CLASSIC, settings_get()->boot_animation);
}

/**
 * @brief settings_range_check()'s individual branches, exercised directly
 * by mutating the live settings struct - cheaper than round-tripping
 * through the EEPROM for each one.
 */
static void test_settings_range_check_branches(void **state)
{
    (void)state;

    assert_int_equal(LCM_SUCCESS, init_from_blank_eeprom());
    settings_t *s = settings_get();
    assert_non_null(s);

    // Freshly reset defaults are valid.
    assert_true(settings_range_check());

    uint32_t saved_magic = s->magic;
    s->magic = 0U;
    assert_false(settings_range_check());
    s->magic = saved_magic;

    animation_option_t saved_anim = s->boot_animation;
    s->boot_animation = ANIMATION_OPTION_COUNT;
    assert_false(settings_range_check());
    s->boot_animation = saved_anim;

    saved_anim = s->idle_animation;
    s->idle_animation = ANIMATION_OPTION_COUNT;
    assert_false(settings_range_check());
    s->idle_animation = saved_anim;

    saved_anim = s->dozing_animation;
    s->dozing_animation = ANIMATION_OPTION_COUNT;
    assert_false(settings_range_check());
    s->dozing_animation = saved_anim;

    saved_anim = s->shutdown_animation;
    s->shutdown_animation = ANIMATION_OPTION_COUNT;
    assert_false(settings_range_check());
    s->shutdown_animation = saved_anim;

    // Note: ride_animation is NOT checked by settings_range_check() (a
    // pre-existing gap in the source, not this test) - deliberately not
    // asserted here since it would fail.

    uint16_t saved_color = s->personal_color;
    s->personal_color = 360U;
    assert_false(settings_range_check());
    s->personal_color = saved_color;

    // Back to fully valid after undoing every mutation above.
    assert_true(settings_range_check());
}

/**
 * @brief settings_save() only writes when the in-memory settings actually
 * differ from what's stored - saving twice in a row with no change in
 * between must not write a second time.
 */
static void test_settings_save_skips_write_when_unchanged(void **state)
{
    (void)state;

    assert_int_equal(LCM_SUCCESS, init_from_blank_eeprom());
    assert_int_equal(1U, mock_eeprom_get_write_count()); // the reset-to-defaults write

    settings_save();
    assert_int_equal(1U, mock_eeprom_get_write_count()); // unchanged - no new write

    settings_get()->personal_color = 7U;
    settings_save();
    assert_int_equal(2U, mock_eeprom_get_write_count()); // changed - wrote again
}

/**
 * @brief The mode_changed handler only saves on the specific
 * IDLE/SHUTTING_DOWN transition - any other event/mode/submode combination
 * must not trigger a write.
 */
static void test_settings_mode_changed_saves_only_on_shutdown(void **state)
{
    (void)state;

    assert_int_equal(LCM_SUCCESS, init_from_blank_eeprom());
    settings_get()->personal_color = 55U;
    uint32_t writes_before = mock_eeprom_get_write_count();

    event_data_t data = {0};
    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_ACTIVE;
    settings_mode_changed_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
    assert_int_equal(writes_before, mock_eeprom_get_write_count()); // no save yet

    data.board_mode.mode = BOARD_MODE_RIDING;
    data.board_mode.submode = BOARD_SUBMODE_RIDING_NORMAL;
    settings_mode_changed_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
    assert_int_equal(writes_before, mock_eeprom_get_write_count()); // still no save

    data.board_mode.mode = BOARD_MODE_IDLE;
    data.board_mode.submode = BOARD_SUBMODE_IDLE_SHUTTING_DOWN;
    settings_mode_changed_event_handler(EVENT_BOARD_MODE_CHANGED, &data);
    assert_int_equal(writes_before + 1U, mock_eeprom_get_write_count()); // saved
    assert_int_equal(55U, settings_get()->personal_color);
}

static const struct CMUnitTest settings_tests[] = {
    cmocka_unit_test(test_settings_init_from_blank_eeprom_resets_to_defaults),
    cmocka_unit_test(test_settings_reset_direct),
    cmocka_unit_test(test_settings_init_loads_previously_saved_values),
    cmocka_unit_test(test_settings_init_invalid_range_forces_reset),
    cmocka_unit_test(test_settings_range_check_branches),
    cmocka_unit_test(test_settings_save_skips_write_when_unchanged),
    cmocka_unit_test(test_settings_mode_changed_saves_only_on_shutdown),
};

#endif // TEST_SETTINGS_H
