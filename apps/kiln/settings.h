#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include "bmtypes.h"

typedef enum
{
    SETTING_ID = 0,
    SETTING_TOUCH_CALIB,
    SETTING_FACTORY,
    SETTING_KILN_MAX_TEMP,
    SETTING_KILN_MAX_TIME,
    SETTING_KILN_MAX_TIME_FULL_PWR,
    SETTING_KILN_MIN_SWITCH_TIME,
    SETTING_CTRL_MAX_TEMP,

    SETTING_PID_K_P,
    SETTING_PID_K_I,
    SETTING_PID_K_D,
    SETTING_PID_EPSILON,

    SETTING_AUDIO_KEY_CLICK,
    SETTING_SCREEN_TIMEOUT,
    SETTING_TEMP_GRAPH_UPDATE,
    SETTING_TEMP_GRAPH_LENGTH,
    SETTING_KILN_SIMULATION,
    _SETTING_INDEXABLE_COUNT,

    _SETTINGS_VOLATILE = 100, // ix == id from now on
    SETTING_UI_KILN_TEMP,
    SETTING_UI_KILN_POWER,
    SETTING_UI_RESET,
} setting_t;

typedef enum
{
    UNIT_DECIMAL,
    UNIT_INTEGER,
    UNIT_BOOLEAN,
    UNIT_RESETTABLE,
    UNIT_DEGREE_C,
    UNIT_PER_TEN_THOUSAND,
    UNIT_STRING,
    UNIT_MILLISECONDS,
    UNIT_SECONDS,
    UNIT_MINUTES,
    UNIT_HOURS,
} setting_unit_t;

typedef struct
{
    setting_t id;
    const char *ui_name;
    setting_unit_t unit;
    void *min_value;
    void *max_value;
    void *def_value;
} user_setting_t;

#define SETTINGS_TEMP_GRAPH_LEN_MAX (3600)

uint32_t settings_count(void);
const user_setting_t *settings_get(uint32_t ix);
void *settings_get_val(uint32_t ix);
void *settings_get_val_for_id(setting_t id);
int settings_set_val(uint32_t ix, void *val);
const char *settings_unit_str(uint32_t ix);
void settings_get_val_as_string(uint32_t ix, char *dst, uint32_t n, int unit);
uint32_t settings_get_index_for_id(setting_t id);
void settings_set_cb_fns(
    void (*setting_change_cb_fn)(setting_t id, void *val),
    const user_setting_t *(*setting_get_cb_fn)(uint32_t ix),
    void *(*settings_val_cb_fn)(uint32_t ix));
void settings_init(void);

#endif