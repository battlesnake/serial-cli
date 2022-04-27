#include <stdbool.h>
#include <stdio.h>
#include "cli.h"

#ifndef NULL
#define NULL ((void *) 0)
#endif

static enum cli_command_result test_handler(const cli_expression *command, const struct cli_command_definition *def);

static const struct cli_language_definition lang1;

#define KWIDX(name) CLI_KEYWORD_INDEX_TO_SPEC_KEYWORD(kw_##name)

enum keywords
{
    kw_true,
    kw_false,
    kw_get,
    kw_set,
    kw_bake,
    kw_potato,
    kw_lemon,
    kw_count,
    kw_mass,
    kw_to
};

enum commands
{
    cmd_true,
    cmd_false,
    cmd_get_potato_count,
    cmd_get_potato_mass,
    cmd_get_lemon_count,
    cmd_get_lemon_mass,
    cmd_set_potato_count,
    cmd_set_lemon_count,
    cmd_bake_potato,
};

static const struct cli_language_definition lang1 =
{
    .keywords = (const cli_keyword[]) {
        "true",
        "false",
        "get",
        "set",
        "bake",
        "potato",
        "lemon",
        "count",
        "mass",
        "to",
        NULL
    },
    .commands = (const struct cli_command_definition[]) {
        {
            .syntax = { KWIDX(true) },
            .handler = test_handler,
        },
        {
            .syntax = { KWIDX(false) },
            .handler = test_handler,
        },
        {
            .syntax = { KWIDX(get), KWIDX(potato), KWIDX(count) },
            .handler = test_handler,
        },
        {
            .syntax = { KWIDX(get), KWIDX(potato), KWIDX(mass) },
            .handler = test_handler,
        },
        {
            .syntax = { KWIDX(get), KWIDX(lemon), KWIDX(count) },
            .handler = test_handler,
        },
        {
            .syntax = { KWIDX(get), KWIDX(lemon), KWIDX(mass) },
            .handler = test_handler,
        },
        {
            .syntax = { KWIDX(set), KWIDX(potato), KWIDX(count), KWIDX(to), CLI_SPEC_NUMBER },
            .handler = test_handler,
        },
        {
            .syntax = { KWIDX(set), KWIDX(lemon), KWIDX(count), KWIDX(to), CLI_SPEC_NUMBER },
            .handler = test_handler,
        },
        {
            .syntax = { KWIDX(bake), KWIDX(potato) },
            .handler = test_handler,
        },
        {
            .handler = NULL,
        },
    },
};

static int test_case = 0;
#define ASSERT(expect, actual) \
    do { \
        int _case = ++test_case; \
        bool _result = ((expect) == (actual)); \
        if (_result) { \
            printf("#%d: Assertion passed: %s == %s\n", _case, #expect, #actual); \
        } else { \
            printf("#%d: Assertion failed: %s == %s\n", _case, #expect, #actual); \
            return -1; \
        } \
    } while (0)

int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;
    const struct cli_command_definition *def;
    cli_expression bytecode;

    ASSERT(parse_long_command_success, parse_long_command(&lang1, "", &bytecode));
    ASSERT(match_command_fail, match_command(&lang1, &bytecode, &def));

    ASSERT(parse_long_command_invalid_token, parse_long_command(&lang1, "invalid", &bytecode));

    ASSERT(parse_long_command_success, parse_long_command(&lang1, "true", &bytecode));
    ASSERT(match_command_success, match_command(&lang1, &bytecode, &def));
    ASSERT(cli_command_success, def->handler(&bytecode, def));

    ASSERT(parse_long_command_success, parse_long_command(&lang1, "false", &bytecode));
    ASSERT(match_command_success, match_command(&lang1, &bytecode, &def));
    ASSERT(cli_command_fail, def->handler(&bytecode, def));

    ASSERT(parse_long_command_success, parse_long_command(&lang1, "set potato count to 42", &bytecode));
    ASSERT(match_command_success, match_command(&lang1, &bytecode, &def));
    ASSERT(cli_command_success, def->handler(&bytecode, def));

    ASSERT(parse_long_command_success, parse_long_command(&lang1, "set lemon count to -42", &bytecode));
    ASSERT(match_command_success, match_command(&lang1, &bytecode, &def));
    ASSERT(cli_command_success, def->handler(&bytecode, def));

    ASSERT(parse_long_command_invalid_token, parse_long_command(&lang1, "set potato count to -1001", &bytecode));

    ASSERT(parse_long_command_invalid_token, parse_long_command(&lang1, "set potato count to 1001", &bytecode));

    ASSERT(parse_long_command_too_many_tokens, parse_long_command(&lang1, "bake set true potato count to false lemon count", &bytecode));

    return 0;
}

static enum cli_command_result test_handler(const cli_expression *bytecode, const struct cli_command_definition *def)
{
    (void) bytecode;
    print_bytecode(&lang1, bytecode);
    enum commands idx = def - &lang1.commands[0];
    if (idx == cmd_true) {
        return cli_command_success;
    } else if (idx == cmd_false) {
        return cli_command_fail;
    } else if (idx == cmd_set_potato_count) {
        ASSERT(42, CLI_EXPR_NUMBER_TO_INT((*bytecode)[4]));
        return cli_command_success;
    } else if (idx == cmd_set_lemon_count) {
        ASSERT(-42, CLI_EXPR_NUMBER_TO_INT((*bytecode)[4]));
        return cli_command_success;
    } else {
        return cli_command_invalid_argument;
    }
}
