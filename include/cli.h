/**
 * ==============================================================================
 * Project: ZeroHAL
 * File: cli.h
 * Description: Zero-Libc Bare-Metal Interactive Diagnostic Command Parser
 * ==============================================================================
 */

#ifndef ZERO_HAL_CLI_H
#define ZERO_HAL_CLI_H

#include <stdint.h>

#define CLI_BUFFER_SIZE         64U

/**
 * @brief Initialize the CLI internal state and buffer index.
 */
void cli_init(void);

/**
 * @brief Display the CLI terminal prompt ("ZeroHAL> ").
 */
void cli_prompt(void);

/**
 * @brief Process an incoming raw character from UART or simulated terminal.
 *        Handles carriage return/line feed, backspace, and character echo.
 * @param c Incoming ASCII character.
 */
void cli_handle_char(char c);

/**
 * @brief Parse and execute an ECU diagnostic command string.
 * @param cmd Null-terminated command line string.
 */
void cli_execute(const char *cmd);

#endif /* ZERO_HAL_CLI_H */
