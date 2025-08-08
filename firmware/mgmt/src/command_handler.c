#include "command_handler.h"
#include "utils.h"
#include <string.h>

int8_t ParseWords(const uint8_t* buff, const size_t len, char words[][32], uint8_t* word_count)
{
    *word_count = 0;
    int word_idx = 0;
    int char_idx = 0;
    int offset = 0;

    // Skip any leading spaces
    while (buff[offset] == ' ' || buff[offset] == '\t')
    {
        offset++;

        if (offset >= len)
        {
            return 0;
        }
    }

    while (buff[offset] != '\0' && offset < len && *word_count < 3)
    {
        if (buff[offset] == ' ' || buff[offset] == '\t')
        {
            // End of current word
            if (char_idx > 0)
            {
                words[word_idx][char_idx] = '\0';
                ++(*word_count);
                ++word_idx;
                char_idx = 0;
            }
            else
            {
                ++offset;
            }
        }
        else
        {
            // Add characters to the current word
            if (char_idx < 31)
            {
                uint8_t c = buff[offset];
                // Convert character to uppercase
                if (c >= 'a' && c <= 'z')
                {
                    c -= 32;
                }
                words[word_idx][char_idx++] = c;
            }
        }
        ++offset;
    }

    return 1;
}

void ProcessCommand(UART_HandleTypeDef* huart, const uint8_t* buff, const size_t len)
{
    uint8_t words[WORD_COUNT_MAX][32];
    uint8_t word_count;

    ParseWords(buff, len, words, &word_count);

    if (word_count == 0)
    {
        // Empty command!
        return;
    }

    // Find matching command
    for (int i = 0; commands[i].pattern != NULL; ++i)
    {
        if (commands[i].word_count == word_count && strcmp(words[0], commands[i].pattern) == 0)
        {
            const char* w1 = (word_count > 0) ? words[0] : "";
            const char* w2 = (word_count > 1) ? words[1] : "";
            const char* w3 = (word_count > 2) ? words[2] : "";
            const char* w4 = (word_count > 3) ? words[3] : "";

            commands[i].callback(huart, w1, w2, w3, w4);
            return;
        }
    }

    // If we get here than a command was not found.
    UART_SendString(huart, "ERROR: Unknown command '");

    for (int i = 0; i < word_count; ++i)
    {
        UART_SendString(huart, words[i]);
        if (i < word_count - 1)
        {
            // Send a space per word if its not last
            UART_SendString(huart, " ");
        }
    }
    UART_SendString(huart, "'. Type 'HELP' for available commands\n");

    return;
}

void Cmd_Help(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4)
{
    UART_SendString(huart, "Available commands:\n");
    UART_SendString(huart, "Single word commands:\n");
    UART_SendString(huart, "  HELP - Display this help message\n");
    UART_SendString(huart, "  STATUS - Display system status\n");
    UART_SendString(huart, "Two word commands:\n");
    UART_SendString(huart, "  CLEAR [ui|net] - Clear the current configuration in chip\n");
    UART_SendString(huart, "  DEBUG [ui|net|all] - Reset and show logs\n");
    UART_SendString(huart, "  HIDE [ui|net|all] - Hide logs from specified chip\n");
    UART_SendString(huart, "  RESET [ui|net|all] - Reset and hide logs\n");
    UART_SendString(huart, "  SHOW [ui|net|all] - Show logs from specified chip\n");
    UART_SendString(huart, "  STOP [ui|net|all] - Hold a chip in reset\n");
    UART_SendString(huart, "  UPLOAD [ui|net] - Go into upload mode for a chip\n");
    UART_SendString(huart, "  VERSION [ui|net|mgmt] - Display version number\n");
    UART_SendString(huart, "Three word commands:\n");
    UART_SendString(huart, "  GET [ui|net] [configuration] - Get configuration from chip\n");
    UART_SendString(huart, "Four word commands:\n");
    UART_SendString(huart, "  SET [ui|net] [configuration] [value] - Set configuration on chip\n");
}

void Cmd_Version(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4)
{
    if (strcmp(w2, "ui") == 0)
    {
        // TODO get from ui
        UART_SendString(huart, "Version: 0.13.0\n");
    }
    else if (strcmp(w2, "net") == 0)
    {
        // TODO get from net
        UART_SendString(huart, "Version: 0.13.0\n");
    }
    else if (strcmp(w3, "mgmt") == 0)
    {
        // TODO, MACRO
        UART_SendString(huart, "Version: 0.13.0\n");
    }
    else
    {
        UART_SendString(huart, "ERROR: invalid value: ");
        UART_SendString(huart, w2);
        UART_SendString(huart, "\n");
    }
}

void Cmd_Clear(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4)
{
}

void Cmd_Debug(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4)
{
}

void Cmd_Hide_Logs(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4)
{
}

void Cmd_Show_Logs(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4)
{
}

void Cmd_Reset(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4)
{
}

void Cmd_Stop(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4)
{
}

void Cmd_Upload(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4)
{
}

void Cmd_Version(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4)
{
}

void Cmd_Get(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4)
{
}

void Cmd_Set(
    UART_HandleTypeDef* huart, const char* w1, const char* w2, const char* w3, const char* w4)
{
}
