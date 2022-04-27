#pragma once

/* Enable a load of annoying noisy debug messages */
// #define CLI_DEBUG_PARSER

/* A specification for a token */
typedef unsigned char syntax_token;

/* Bytecode value of a parsed token */
typedef unsigned short expression_token;

/* Max tokens in one CLI command */
#define CLI_MAX_TOKENS (8)

/* Maximum number of commands that can be defined */
#define CLI_MAX_COMMANDS (64)

/* Codes for defining spceification of commands */
#define CLI_SPEC_TERMINAL ((syntax_token) 0x00)
#define CLI_SPEC_KEYWORD_BEGIN ((syntax_token) 0x80)
#define CLI_SPEC_KEYWORD_END ((syntax_token) 0xc0)
#define CLI_SPEC_NUMBER ((syntax_token) 0xfe)

/* Permitted range of parsed numbers */
#define CLI_NUMBER_MIN (-1000)
#define CLI_NUMBER_MAX (1000)
#define CLI_NUMBER_ABSMAX ((CLI_NUMBER_MAX) > -(CLI_NUMBER_MIN) ? (CLI_NUMBER_MAX) : -(CLI_NUMBER_MIN))

/* Predicates for parsing command definitions */
#define CLI_SPEC_IS_TERMINAL(token) ((token) == (CLI_SPEC_TERMINAL))
#define CLI_SPEC_IS_KEYWORD(token) ((token) >= (CLI_SPEC_KEYWORD_BEGIN) && (token) < (CLI_SPEC_KEYWORD_END))
#define CLI_SPEC_IS_NUMBER(token) ((token) == (CLI_SPEC_NUMBER))

/* Ranges */
#define CLI_RANGE_KEYWORD ((CLI_SPEC_KEYWORD_END) - (CLI_SPEC_KEYWORD_BEGIN))
#define CLI_RANGE_NUMBER ((CLI_NUMBER_MAX) - (CLI_NUMBER_MIN))

/* Bytecode values/boundaries */
#define CLI_EXPR_TERMINAL ((expression_token) 0x0000)
#define CLI_EXPR_KEYWORD_BEGIN ((expression_token) 0xf000)
#define CLI_EXPR_KEYWORD_END ((expression_token) ((CLI_EXPR_KEYWORD_BEGIN) + (CLI_RANGE_KEYWORD)))
#define CLI_EXPR_NUMBER_BEGIN ((expression_token) (0xe000))
#define CLI_EXPR_NUMBER_END ((expression_token) ((CLI_EXPR_NUMBER_BEGIN) + (CLI_RANGE_NUMBER)))

/* Predicates for parsing bytecode */
#define CLI_EXPR_IS_TERMINAL(token) ((token) == (CLI_EXPR_TERMINAL))
#define CLI_EXPR_IS_KEYWORD(token) ((token) >= (CLI_EXPR_KEYWORD_BEGIN) && (token) < (CLI_EXPR_KEYWORD_END))
#define CLI_EXPR_IS_NUMBER(token) ((token) >= (CLI_EXPR_NUMBER_BEGIN) && (token) < (CLI_EXPR_NUMBER_END))

/* Convert keyword bytecode value to index in keywords list / or back */
#define CLI_EXPR_KEYWORD_TO_KEYWORD_INDEX(token) ((token) - (CLI_EXPR_KEYWORD_BEGIN))
#define CLI_KEYWORD_INDEX_TO_EXPR_KEYWORD(index) ((index) + (CLI_EXPR_KEYWORD_BEGIN))

/* Convert keyword index (in keywords list) to command-definition value */
#define CLI_KEYWORD_INDEX_TO_SPEC_KEYWORD(index) ((index) + (CLI_SPEC_KEYWORD_BEGIN))

/* Convert keyword bytecode value to command-definition value */
#define CLI_EXPR_KEYWORD_TO_SPEC_KEYWORD(token) CLI_KEYWORD_INDEX_TO_SPEC_KEYWORD(CLI_EXPR_KEYWORD_TO_KEYWORD_INDEX(token))

/* Convert bytecode number to number value, or the reverse */
#define CLI_EXPR_NUMBER_TO_INT(token) ((int) ((expression_token) (token) - (CLI_EXPR_NUMBER_BEGIN)) + (CLI_NUMBER_MIN))
#define CLI_INT_TO_EXPR_NUMBER(value) ((expression_token) ((int) (value) - (CLI_NUMBER_MIN)) + (CLI_EXPR_NUMBER_BEGIN))

/* Keyword is defined by reference to a null-terminated string */
typedef const char *cli_keyword;

/* Command specification is defined by array of tokens (trailing slots should be TERMINAL) */
typedef const syntax_token cli_command_syntax[CLI_MAX_TOKENS];

/* Bytecode is an array of bytecode values (trailing slots should be TERMINAL) */
typedef expression_token cli_expression[CLI_MAX_TOKENS];

/* Result of executing a command */
enum cli_command_result
{
    cli_command_success,
    cli_command_fail,
    cli_command_invalid_argument,
};

struct cli_command_definition;

/* Signature of command handler, called to execute command */
typedef enum cli_command_result cli_command_handler(const cli_expression *bytecode, const struct cli_command_definition *def);

/* Definition of a command: Specification of syntax, and handler callback */
struct cli_command_definition
{
    cli_command_syntax syntax;
    cli_command_handler *handler;
};

/* Language specification: Keyword LUT and command syntax definitions */
struct cli_language_definition
{
    /* Terminated by entry with NULL handler */
    const cli_keyword *keywords;
    /* Terminated by entry with NULL members */
    const struct cli_command_definition *commands;
};

/*
 * Parse a text command to bytecode
 */
enum parse_long_command_result
{
    parse_long_command_success,
    parse_long_command_too_many_tokens,
    parse_long_command_invalid_token,
};

enum parse_long_command_result parse_long_command(const struct cli_language_definition *spec, const char *command, cli_expression *parsed);

/*
 * Match a bytecode command to the respective handler callback
 */
enum match_command_result
{
    match_command_success,
    match_command_fail,
    match_command_invalid_token,
};

enum match_command_result match_command(const struct cli_language_definition *language, const cli_expression *value, const struct cli_command_definition **result);

/*
 * Debug print bytecode command
 */
#ifdef CLI_DEBUG_PARSER
#define print_bytecode _print_bytecode
void _print_bytecode(const struct cli_language_definition *language, const cli_expression *bytecode);
#else
#define print_bytecode(...) do { } while (0)
#endif
