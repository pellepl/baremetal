#ifndef _CLI_H_
#define _CLI_H_

/* Simple command line interface.
 *
 * Define cli functions anywhere in your linked code in following manner:
 * 
 *    static int cli_hello_world(int argc, const char **argv) {
 *       printf("hello world\n");
 *       return 0;
 *    }
 *    CLI_FUNCTION(cli_hello_world, "helloworld");
 * 
 * Expects a pointer aligned section in linker called .cli_func_entries,
 * with __cli_func_entries_start and __cli_func_entries_end symbols wrapping
 * the section.
 * Like so:
 * 
 *    . = ALIGN(4);
 *    __cli_func_entries_start = .;
 *    KEEP(*(.cli_func_entries*))
 *    __cli_func_entries_end = .;
 */

#include "types.h"

#ifndef CLI_BUFFER_SIZE
#define CLI_BUFFER_SIZE             (256)
#endif //CLI_BUFFER_SIZE

#ifndef CLI_ARGS_MAX
#define CLI_ARGS_MAX                (16)
#endif //CLI_ARGS_MAX

#ifndef ERR_CLI_BASE
#define ERR_CLI_BASE                (0x00001000)
#endif //ERR_CLI_BASE

#define ERR_CLI_INPUT_OVERFLOW      -(ERR_CLI_BASE+0)
#define ERR_CLI_ARGUMENT_OVERFLOW   -(ERR_CLI_BASE+1)
#define ERR_CLI_NO_ENTRY            -(ERR_CLI_BASE+2)
#define ERR_CLI_EINVAL              -(ERR_CLI_BASE+3)

typedef struct cli_entry {
    const char *name;
    int (* func)(int argc, const char **argv);
} cli_entry_t;

#define CLI_FUNCTION(_func, _name) \
static const cli_entry_t  __cli_descr_ ## _func = {.name=_name, .func=_func }; \
__attribute__(( used, section(".cli_func_entries") )) static const cli_entry_t  *__cli_entry_ ## _func = &__cli_descr_ ## _func;

/** CLI callback */
typedef void (* cli_callback_fn_t)(const char *func, int result);

/**
 * Initializes cli.
 * @param cb            Function being called on invocation of a cli command
 * @param commit_chars  Any of these characters will commit entered text so far
 * @param delim_chars   Any of these characters will serve as argument delimiters
 * @param control_chars Any of these characters functions as a single argument by itself
 * @param ignore_chars  Any of these characters are ignored at start of input
 */
void cli_init(cli_callback_fn_t cb,
            const char *commit_chars, const char *delim_chars,
            const char *control_chars, const char *ignore_chars);

/**
 * Parses some text, may eventually call a cli function,
 * resulting in a callback.
 * @param str   Text to parse
 * @param len   Length of text to parse
 */
void cli_parse(const char *str, uint32_t len);

/**
 * Returns number of cli commands
 */
int cli_entry_list_length(void);

/**
 * Returns start of cli command list
 */
cli_entry_t **cli_entry_list_start(void);

#endif // _CLI_H_
