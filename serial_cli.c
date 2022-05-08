#include <stdbool.h>
#include <stdio.h>

#include "serial_cli.h"

#ifndef NULL
#define NULL ((void *) 0)
#endif

/* Define bools if not done already in headers */
#ifndef FALSE
#define FALSE (0)
#define TRUE (!(FALSE))
#endif

/* Delimiter in long-format commands */
#define CLI_LONG_SPACE ' '

/* Log CLI parse errors */
#ifdef CLI_DEBUG_PARSER
#define parse_error_printf(format, ...) printf(format "\n", ##__VA_ARGS__)
#define parse_error_printf_str(format, begin, end, ...) parse_error_printf(format " (token: %*.*s)", ##__VA_ARGS__, (int) (end - begin), (int) (end - begin), begin)
#define parse_error_printf_token(format, token, ...) parse_error_printf(format " (opcode: 0x%04x)", ##__VA_ARGS__, token)
#else
#define parse_error_printf(...) do { } while (0)
#define parse_error_printf_str(...) do { } while (0)
#define parse_error_printf_token(...) do { } while (0)
#endif

/*
 * Find next space or non-space
 */
static const char *find_space(const char *begin, const char *end, bool space)
{
    if (!begin) {
        return NULL;
    }
    const char *it;
    for (it = begin; it != end && *it; ++it) {
        if ((*it == CLI_LONG_SPACE) == space) {
            return it;
        }
    }
    return space ? it : NULL;
}

/*
 * Test equality of string range and null-terminated string
 */
static bool string_equal(const char *begin, const char *end, const char *other)
{
    if (!begin) {
        return FALSE;
    }
    for (; begin != end && *other; ++begin, ++other) {
        if (*begin != *other) {
            return FALSE;
        }
    }
    return begin == end && !*other;
}

enum parse_int_result
{
    parse_int_success,
    parse_int_fail,
    parse_int_out_of_range,
};

/*
 * Parse an integer from string range, and return byte-code for it
 */
static enum parse_int_result parse_int(expression_token *result, const char *begin, const char *end)
{
    const char *it = begin;
    bool negative = *it == '-';
    if (negative) {
        ++it;
    }
    int value = 0;
    for (; it != end; ++it) {
        char ch = *it;
        if (ch < '0' || ch > '9') {
            parse_error_printf_str("Invalid digit: %c", begin, end, ch);
            return parse_int_fail;
        }
        value *= 10;
        value += ch - '0';
        if (value > CLI_NUMBER_ABSMAX) {
            parse_error_printf_str("Numeric value out of range: %d", begin, end, value);
            return parse_int_out_of_range;
        }
    }
    if (negative) {
        value = -value;
    }
    if (value < CLI_NUMBER_MIN || value > CLI_NUMBER_MAX) {
        parse_error_printf_str("Numeric value out of range: %d", begin, end, value);
        return parse_int_out_of_range;
    }
    *result = CLI_INT_TO_EXPR_NUMBER(value);
    return parse_int_success;
}

enum parse_keyword_result
{
    parse_keyword_success,
    parse_keyword_fail,
};

/*
 * Parse a keyword from a string range, and return byte-code for it
 */
static bool parse_keyword(expression_token *token, const char *begin, const char *end, const cli_keyword *keywords)
{
    for (const cli_keyword *keyword = keywords; *keyword; ++keyword) {
        if (string_equal(begin, end, *keyword)) {
            *token = (keyword - keywords) + CLI_EXPR_KEYWORD_BEGIN;
            return parse_keyword_success;
        }
    }
    return parse_keyword_fail;
}

//enum cli_command_syntax_validation
//{
//    cli_next_invalid = 0x00,
//    cli_next_terminal = 0x01,
//    cli_next_keyword = 0x02,
//    cli_next_number = 0x04,
//};
//
///*
// * Return set of valid token types that can follow the current prefix
// */
// enum cli_command_syntax_validation next_token(const cli_command_syntax *command_specs, const cli_expression *prefix, int length)
// {
//     if (length > CLI_MAX_TOKENS) {
//         return 0x00;
//     }
//     const expression_token * const prefix_end = *prefix + length;
//     enum cli_command_syntax_validation next = 0x00;
//     /* Iterate over command specificatoins */
//     for (const cli_command_syntax *specs_it = command_specs; *specs_it; ++specs_it) {
//         /* Iterate over tokens of command specification and tokens of prefix */
//         const syntax_token *spec_it = *specs_it;
//         const expression_token *prefix_it = *prefix;
//         while (prefix_it != prefix_end) {
//             syntax_token spec_token = *spec_it;
//             expression_token prefix_token = *prefix_it;
//             if (CLI_SPEC_IS_TERMINAL(spec_token)) {
//                 /* Reached end of command spec before end of prefix */
//                 break;
//             } else if (CLI_SPEC_IS_KEYWORD(spec_token) && spec_token != CLI_EXPR_KEYWORD_TO_SPEC_KEYWORD(prefix_token)) {
//                 /* Keyword is not the same as in the spec */
//                 break;
//             } else if (CLI_SPEC_IS_NUMBER(spec_token) && !CLI_EXPR_IS_NUMBER(prefix_token)) {
//                 /* Not anumber but a number is in the spec */
//                 break;
//             }
//             ++spec_it;
//             ++prefix_it;
//             if (prefix_it == prefix_end) {
//                 /* Reached end of prefix, study next token of command spec */
//                 spec_token = *spec_it;
//                 if (CLI_SPEC_IS_TERMINAL(spec_token)) {
//                     next |= cli_next_terminal;
//                 } else if (CLI_SPEC_IS_KEYWORD(spec_token)) {
//                     next |= cli_next_keyword;
//                 } else if (CLI_SPEC_IS_NUMBER(spec_token)) {
//                     next |= cli_next_number;
//                 }
//             }
//         }
//     }
//     return next;
// }

/*
 * Parse a long command to bytecode
 */
enum parse_long_command_result parse_long_command(const struct cli_language_definition *spec, const char *command, const char *command_end, cli_expression *parsed)
{
    const char *it = command;
    expression_token *out_it = *parsed;
    expression_token *out_end = *parsed + CLI_MAX_TOKENS;
    const char *word_begin;
    while ((word_begin = find_space(it, command_end, FALSE))) {
        if (out_it == out_end) {
            /* Max tokens already parsed */
            parse_error_printf("Too many tokens: %s", command);
            return parse_long_command_too_many_tokens;
        }
        const char *word_end = find_space(word_begin, command_end, TRUE);
        parse_error_printf_str("Token:", word_begin, word_end);
        it = word_end;
        expression_token token = 0;
        if (parse_keyword(&token, word_begin, word_end, spec->keywords) == parse_keyword_success) {
            /* Keyword matches have highest precedence */
        } else if (parse_int(&token, word_begin, word_end) == parse_int_success) {
            /* Integer value */
        } else {
            parse_error_printf_str("Unrecognised token:", word_begin, word_end);
            return parse_long_command_invalid_token;
        }
        *out_it++ = token;
    }
    if (out_it != out_end) {
        *out_it = CLI_EXPR_TERMINAL;
    }
    return parse_long_command_success;
}

/*
 * Match a bytecode command to the respective definition
 */
enum match_command_result match_command(const struct cli_language_definition *language, const cli_expression *value, const struct cli_command_definition **result)
{
    *result = NULL;
    /* Iterate over command specificatoins */
    for (const struct cli_command_definition *command_it = &language->commands[0]; command_it->handler; ++command_it) {
        /* Iterate over tokens of command specification and tokens of command */
        const syntax_token *syntax_it = &command_it->syntax[0];
        const syntax_token *syntax_end = syntax_it + CLI_MAX_TOKENS;
        const expression_token *value_it = *value;
        const expression_token *value_end = value_it + CLI_MAX_TOKENS;
        while (1) {
            bool syntax_terminal = syntax_it == syntax_end || CLI_SPEC_IS_TERMINAL(*syntax_it);
            bool value_terminal = value_it == value_end || CLI_EXPR_IS_TERMINAL(*value_it);
            if (syntax_terminal || value_terminal) {
                /* Lengths differ */
                if (syntax_terminal != value_terminal) {
                    break;
                }
                *result = command_it;
                return match_command_success;
            } else if (CLI_SPEC_IS_KEYWORD(*syntax_it)) {
                /* Keyword */
                if (*syntax_it != CLI_EXPR_KEYWORD_TO_SPEC_KEYWORD(*value_it)) {
                    /* Keyword is not the same as in the spec */
                    break;
                }
            } else if (CLI_SPEC_IS_NUMBER(*syntax_it)) {
                /* Number */
                if (!CLI_EXPR_IS_NUMBER(*value_it)) {
                    /* Not a number but a number is in the spec */
                    break;
                }
            } else {
                /* Invalid token */
                parse_error_printf_token("Unrecognised token: ", *value_it);
                return match_command_invalid_token;
            }
            ++syntax_it;
            ++value_it;
        }
    }
    return match_command_fail;
}

/*
 * Debug print bytecode command
 */
void _print_bytecode(const struct cli_language_definition *language, const cli_expression *bytecode)
{
    const expression_token * const begin = *bytecode;
    const expression_token * const end = begin + CLI_MAX_TOKENS;
    printf("Bytecode dump: \"");
    for (const expression_token *it = begin; it != end && !CLI_EXPR_IS_TERMINAL(*it); ++it) {
        if (it != *bytecode) {
            printf("%c", CLI_LONG_SPACE);
        }
        if (CLI_EXPR_IS_KEYWORD(*it)) {
            int index = CLI_EXPR_KEYWORD_TO_KEYWORD_INDEX(*it);
            printf("%s", language->keywords[index]);
        } else if (CLI_EXPR_IS_NUMBER(*it)) {
            int value = CLI_EXPR_NUMBER_TO_INT(*it);
            printf("%d", value);
        } else {
            printf("<error:0x%04hx>", (expression_token) *it);
        }
    }
    printf("\" (");
    for (const expression_token *it = begin; it != end && !CLI_EXPR_IS_TERMINAL(*it); ++it) {
        if (it != *bytecode) {
            printf(" ");
        }
        printf("%04hx", (expression_token) *it);
    }
    printf(")\n");
}

void list_all_commands(const struct cli_language_definition *language)
{
	printf(PRINTF_LINEBREAK "Commands:" PRINTF_LINEBREAK);
	for (const struct cli_command_definition *cmd = language->commands; cmd->handler; ++cmd) {
		printf(">>>");
		for (
			const syntax_token *token = &cmd->syntax[0], *end = token + CLI_MAX_TOKENS;
			token != end && !CLI_SPEC_IS_TERMINAL(*token);
			++token
		) {
			if (CLI_SPEC_IS_KEYWORD(*token)) {
				printf(" %s", language->keywords[CLI_SPEC_KEYWORD_TO_KEYWORD_INDEX(*token)]);
			} else if (CLI_SPEC_IS_NUMBER(*token)) {
				printf(" #");
			} else {
				printf(" !");
			}
		}
		printf(PRINTF_LINEBREAK);
	}
	printf(PRINTF_LINEBREAK);
}
