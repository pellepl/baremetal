#include "minio.h"
#include "settings.h"
#include "nvmtnv.h"
#include "assert.h"
#include "cli.h"

static const user_setting_t settings[] = {
    (user_setting_t){.id = SETTING_KILN_MAX_TEMP,
                     .ui_name = "Max temp",
                     .unit = UNIT_DEGREE_C,
                     .min_value = (void *)20,
                     .max_value = (void *)1300,
                     .def_value = (void *)250},
    (user_setting_t){.id = SETTING_KILN_MAX_TIME,
                     .ui_name = "Max time",
                     .unit = UNIT_HOURS,
                     .min_value = (void *)1,
                     .max_value = (void *)48,
                     .def_value = (void *)2},
    (user_setting_t){.id = SETTING_KILN_MAX_TIME_FULL_PWR,
                     .ui_name = "Full power max time",
                     .unit = UNIT_MINUTES,
                     .min_value = (void *)0,
                     .max_value = (void *)2880,
                     .def_value = (void *)120},
    (user_setting_t){.id = SETTING_CTRL_MAX_TEMP,
                     .ui_name = "Controller max temp",
                     .unit = UNIT_DEGREE_C,
                     .min_value = (void *)20,
                     .max_value = (void *)80,
                     .def_value = (void *)50},
    (user_setting_t){.id = SETTING_KILN_MIN_SWITCH_TIME,
                     .ui_name = "Min switch time",
                     .unit = UNIT_MILLISECONDS,
                     .min_value = (void *)100,
                     .max_value = (void *)30000,
                     .def_value = (void *)1000},

    (user_setting_t){.id = SETTING_PID_K_P,
                     .ui_name = "PID Kp",
                     .unit = UNIT_PER_TEN_THOUSAND,
                     .min_value = (void *)0,
                     .max_value = (void *)1000000,
                     .def_value = (void *)1000},
    (user_setting_t){.id = SETTING_PID_K_I,
                     .ui_name = "PID Ki",
                     .unit = UNIT_PER_TEN_THOUSAND,
                     .min_value = (void *)0,
                     .max_value = (void *)1000000,
                     .def_value = (void *)0},
    (user_setting_t){.id = SETTING_PID_K_D,
                     .ui_name = "PID Kd",
                     .unit = UNIT_PER_TEN_THOUSAND,
                     .min_value = (void *)0,
                     .max_value = (void *)1000000,
                     .def_value = (void *)0},
    (user_setting_t){.id = SETTING_PID_EPSILON,
                     .ui_name = "PID epsilon",
                     .unit = UNIT_PER_TEN_THOUSAND,
                     .min_value = (void *)0,
                     .max_value = (void *)1000000,
                     .def_value = (void *)100000},

    (user_setting_t){.id = SETTING_SCREEN_TIMEOUT,
                     .ui_name = "Screen timeout",
                     .unit = UNIT_SECONDS,
                     .min_value = (void *)5,
                     .def_value = (void *)60,
                     .max_value = (void *)7200},
    (user_setting_t){.id = SETTING_TEMP_GRAPH_UPDATE,
                     .ui_name = "Temp graph update",
                     .unit = UNIT_SECONDS,
                     .min_value = (void *)1,
                     .max_value = (void *)60,
                     .def_value = (void *)1},
    (user_setting_t){.id = SETTING_TEMP_GRAPH_LENGTH,
                     .ui_name = "Temp graph length",
                     .unit = UNIT_INTEGER,
                     .min_value = (void *)100,
                     .max_value = (void *)SETTINGS_TEMP_GRAPH_LEN_MAX,
                     .def_value = (void *)SETTINGS_TEMP_GRAPH_LEN_MAX},
    (user_setting_t){.id = SETTING_FACTORY,
                     .ui_name = "Factory settings",
                     .unit = UNIT_RESETTABLE},
    (user_setting_t){.id = SETTING_UI_RESET,
                     .ui_name = "Processor",
                     .unit = UNIT_RESETTABLE}};
static setting_t ix_id[_SETTING_INDEXABLE_COUNT];
static void *values[sizeof(settings) / sizeof(settings[0])];
static void (*setting_change_cb)(setting_t id, void *val);
static const user_setting_t *(*setting_get_cb)(uint32_t ix);
static void *(*settings_val_cb)(uint32_t ix);

uint32_t settings_count(void)
{
    return sizeof(settings) / sizeof(settings[0]);
}

const user_setting_t *settings_get(uint32_t ix)
{
    if (ix >= settings_count() && setting_get_cb)
    {
        return setting_get_cb(ix);
    }
    else
    {
        return &settings[ix];
    }
}

void *settings_get_val(uint32_t ix)
{
    if (ix > _SETTINGS_VOLATILE)
    {
        return settings_val_cb(ix);
    }
    else
    {
        return values[ix];
    }
}

void *settings_get_val_for_id(setting_t id)
{
    return settings_get_val(settings_get_index_for_id(id));
}

uint32_t settings_get_index_for_id(setting_t id)
{
    if (id > _SETTINGS_VOLATILE)
        return (uint32_t)id;
    return ix_id[id];
}

int settings_set_val(uint32_t ix, void *val)
{
    const user_setting_t *s = settings_get(ix);
    int res = 0;
    if (ix < _SETTINGS_VOLATILE)
    {
        printf("saving setting ix %d, id %d:%s\n", ix, s->id, s->ui_name);
        if (s->unit == UNIT_STRING)
        {
            res = nvmtnv_write(s->id, (uint8_t *)val, strlen((char *)val));
        }
        else
        {
            res = nvmtnv_write(s->id, (uint8_t *)&val, sizeof(values[ix]));
        }
    }
    if (res >= 0)
    {
        printf("setting changed, callback: %s=%d\n", s->ui_name, (uint32_t)val);
        if (ix < _SETTINGS_VOLATILE)
            values[ix] = val;
        if (setting_change_cb)
            setting_change_cb(s->id, val);
    }

    return res;
}

const char *settings_unit_str(uint32_t ix)
{
    const user_setting_t *s = settings_get(ix);
    switch (s->unit)
    {
    case UNIT_DEGREE_C:
        return "\x60\x43";
    case UNIT_MILLISECONDS:
        return "ms";
    case UNIT_SECONDS:
        return "sec";
    case UNIT_MINUTES:
        return "min";
    case UNIT_HOURS:
        return "h";
    case UNIT_PER_TEN_THOUSAND:
        return "/10000";
    default:
        return "";
    }
}

void settings_set_cb_fns(
    void (*setting_change_cb_fn)(setting_t id, void *val),
    const user_setting_t *(*setting_get_cb_fn)(uint32_t ix),
    void *(*settings_val_cb_fn)(uint32_t ix))
{
    setting_change_cb = setting_change_cb_fn;
    setting_get_cb = setting_get_cb_fn;
    settings_val_cb = settings_val_cb_fn;
}

void settings_get_val_as_string(uint32_t ix, char *dst, uint32_t n, int unit)
{
    memset(dst, 0, n);
    void *v = settings_get_val(ix);
    const user_setting_t *s = settings_get(ix);
    switch (s->unit)
    {
    case UNIT_BOOLEAN:
        snprintf(dst, n, "%s", (int)v ? "ON" : "OFF");
        break;
    case UNIT_DEGREE_C:
        snprintf(dst, n, "%d%s", (int32_t)v, unit ? settings_unit_str(ix) : "");
        break;
    case UNIT_PER_TEN_THOUSAND:
        snprintf(dst, n, "%d%s", (int32_t)v, unit ? settings_unit_str(ix) : "");
        break;
    case UNIT_MILLISECONDS:
        snprintf(dst, n, "%d%s", (uint32_t)v, unit ? settings_unit_str(ix) : "");
        break;
    case UNIT_SECONDS:
        snprintf(dst, n, "%d%s", (uint32_t)v, unit ? settings_unit_str(ix) : "");
        break;
    case UNIT_MINUTES:
        snprintf(dst, n, "%d%s", (uint32_t)v, unit ? settings_unit_str(ix) : "");
        break;
    case UNIT_HOURS:
        snprintf(dst, n, "%d%s", (uint32_t)v, unit ? settings_unit_str(ix) : "");
        break;
    case UNIT_STRING:
        snprintf(dst, n, "%s", (const char *)v);
        break;
    case UNIT_RESETTABLE:
        snprintf(dst, n, "RESET");
        break;
    default:
        snprintf(dst, n, "%d", (int32_t)v);
        break;
    }
}

void settings_init(void)
{
    static char string_pool[4][24];
    uint8_t string_pool_ix = {0};
    int res;
    void *fact_reset;
    if (nvmtnv_read(SETTING_FACTORY, (uint8_t *)&fact_reset, sizeof(fact_reset)) == ERR_NVMTNV_NOENT)
    {
        printf("reset to factory settings\n");
        for (uint32_t ix = 0; ix < settings_count(); ix++)
        {
            const user_setting_t *s = settings_get(ix);
            (void)nvmtnv_delete(s->id);
        }
        uint32_t v = 0;
        (void)nvmtnv_write(SETTING_FACTORY, (uint8_t *)&v, sizeof(v));
    }

    for (uint32_t ix = 0; ix < settings_count(); ix++)
    {
        const user_setting_t *s = settings_get(ix);
        ix_id[s->id] = ix;
        if (s->unit == UNIT_STRING)
        {
            ASSERT(string_pool_ix < sizeof(string_pool) / sizeof(string_pool)[0]);
            values[ix] = &string_pool[string_pool_ix];
            string_pool_ix++;
            res = nvmtnv_read(s->id, values[ix], sizeof(string_pool)[0]);
        }
        else
        {
            res = nvmtnv_read(s->id, (uint8_t *)&values[ix], sizeof(values[ix]));
        }
        if (res < 0)
        {
            printf("no setting found for ix %d, id %d:%s, take default %d\n", ix, s->id, s->ui_name, (uint32_t)s->def_value);
            values[ix] = s->def_value;
        }
    }
}

static int cli_settings_list(int argc, const char **argv)
{
    for (uint32_t ix = 0; ix < settings_count(); ix++)
    {
        const user_setting_t *s = settings_get(ix);
        char str[32];
        settings_get_val_as_string(ix, str, sizeof(str), 1);
        printf("%d\t%s = %s\n", s->id, s->ui_name, str);
    }
    return 0;
}
CLI_FUNCTION(cli_settings_list, "cfg_ls", ": lists settings");

static int cli_settings_set(int argc, const char **argv)
{
    if (argc < 2)
    {
        printf("id val\n");
        return ERR_CLI_EINVAL;
    }
    int id = atoi(argv[0]);
    uint32_t val = atoi(argv[1]);
    uint32_t ix = settings_get_index_for_id(id);
    const user_setting_t *setting = settings_get(ix);
    if (setting == 0)
        return ERR_CLI_EINVAL;
    values[ix] = (void *)val;
    if (setting_change_cb)
        setting_change_cb(setting->id, (void *)val);

    return 0;
}
CLI_FUNCTION(cli_settings_set, "cfg_set", "<id> <val>: sets setting");
