/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "cli.h"
#include "minio.h"

static struct {
    cli_callback_fn_t callback;
    char buf[CLI_BUFFER_SIZE];
    uint32_t buf_ix;
    const char *commits;
    const char *delims;
    const char *controls;
    const char *ignores;
    const char *arg[CLI_ARGS_MAX];
    char ctrl_arg[CLI_ARGS_MAX];
    uint32_t arg_ix;
    int was_control;
} state;

static const char *contains(char needle, const char *haystack) {
    char straw;
    if (!haystack) return haystack;
    while ((straw = *haystack++) != 0 && straw != needle);
    return straw == 0 ? (void *)0 : --haystack;
}

void cli_init(cli_callback_fn_t cb, const char *commit_chars, const char *delim_chars,
    const char *control_chars, const char *ignore_chars) {
    state.callback = cb;
    state.buf_ix = state.arg_ix = state.was_control = 0;
    state.commits = commit_chars;
    state.delims = delim_chars;
    state.controls = control_chars;
    state.ignores = ignore_chars;
}

extern void *__cli_func_entries_start, *__cli_func_entries_end;

int cli_entry_list_length(void) {
    return ((intptr_t)&__cli_func_entries_end - (intptr_t)&__cli_func_entries_start) / sizeof(void *);
}

cli_entry_t **cli_entry_list_start(void) {
    return (cli_entry_t **)&__cli_func_entries_start;
}

static void cli_commit(void) {
    int res = ERR_CLI_NO_ENTRY;
    int entries = cli_entry_list_length();
    cli_entry_t **list = cli_entry_list_start();
    for (int i = 0; i < entries; i++) {
        cli_entry_t *e = list[i];
        if (strcmp(e->name, state.buf) == 0 && e->func) {
            res = e->func(state.arg_ix, state.arg);
            break;
        }
    }
    if (state.callback) {
        state.callback(state.buf, res);
    }
}

void cli_parse(const char *str, uint32_t len) {
    for (uint32_t ix = 0; ix < len; ix++) {
        char c = str[ix];
        if (state.buf_ix >= CLI_BUFFER_SIZE || state.arg_ix >= CLI_ARGS_MAX) {
            // cli buffer full or out of arguments
            // ignore and discard input until a commit char is found
            if (contains(c, state.commits)) {
                state.buf_ix = state.arg_ix = state.was_control = 0;
                if (state.callback) {
                    state.callback((void *)0,
                        state.buf_ix >= CLI_BUFFER_SIZE ? ERR_CLI_INPUT_OVERFLOW : ERR_CLI_ARGUMENT_OVERFLOW);
                }
            }
            continue;
        }
        if (state.arg_ix == 0 && contains(c, state.ignores)) {
            continue;
        } else if (contains(c, state.delims)) {
            if (state.buf_ix == 0) {
                continue; // ignore delimiters if they arrive before command
            }
            state.buf[state.buf_ix++] = 0;
            state.arg[state.arg_ix] = state.buf + state.buf_ix;
            state.arg_ix++;
        } else if (contains(c, state.controls)) {
            if (state.buf_ix == 0) {
                continue; // ignore controls if they arrive before command
            }
            state.buf[state.buf_ix++] = 0;
            state.ctrl_arg[state.arg_ix] = c;
            state.arg[state.arg_ix] = &state.ctrl_arg[state.arg_ix];
            state.arg_ix++;
            state.was_control = 1;
        } else if (contains(c, state.commits)) {
            state.buf[state.buf_ix++] = 0;
            if (state.buf_ix > 1) {
                cli_commit();
            }
            state.buf_ix = state.arg_ix = state.was_control = 0;
        } else {
            if (state.was_control) {
                state.was_control = 0;
                state.arg[state.arg_ix] = state.buf + state.buf_ix;
                state.arg_ix++;
            }
            state.buf[state.buf_ix++] = c;
        }
    }
}
