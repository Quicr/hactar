#include "mgmt_packet_handler.hh"
#include "logger.hh"
#include "ui_mgmt_link.h"

static void reverse(char* str, int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end)
    {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

static char* int_to_arr(int value, char* str, int base)
{
    if (base < 2 || base > 36)
    { // Base check: only supports bases 2-36
        *str = '\0';
        return str;
    }

    bool isNegative = false;
    if (value < 0 && base == 10)
    { // Handle negative numbers in base 10
        isNegative = true;
        value = -value;
    }

    int i = 0;
    do
    {
        int digit = value % base;
        str[i++] = (digit > 9) ? (digit - 10 + 'a') : (digit + '0');
        value /= base;
    } while (value != 0);

    if (isNegative)
    {
        str[i++] = '-';
    }

    str[i] = '\0'; // Null-terminate the string

    reverse(str, i); // Reverse the string to correct order

    return str;
}

static char* int_to_arr(int value, char* str, int& len, int base)
{
    if (base < 2 || base > 36)
    { // Base check: only supports bases 2-36
        *str = '\0';
        len = 0;
        return str;
    }

    bool isNegative = false;
    if (value < 0 && base == 10)
    { // Handle negative numbers in base 10
        isNegative = true;
        value = -value;
    }

    int i = 0;
    do
    {
        int digit = value % base;
        str[i++] = (digit > 9) ? (digit - 10 + 'a') : (digit + '0');
        value /= base;
    } while (value != 0);

    if (isNegative)
    {
        str[i++] = '-';
    }

    len = i;
    str[i] = '\0'; // Null-terminate the string

    reverse(str, i); // Reverse the string to correct order

    return str;
}

void HandleMgmtLinkPackets(AudioChip& audio, Serial& serial, ConfigStorage& storage)
{
    while (true)
    {
        link_packet_t* packet = serial.Read();
        if (!packet)
        {
            break;
        }

        switch (packet->type)
        {
        case Configuration::Version:
        {
            UI_LOG_INFO("VERSION TODO");
            break;
        }
        case Configuration::Clear:
        {
            // NOTE, the storage clear WILL brick your hactar for some amount of time until
            // the eeprom fixes itself
            UI_LOG_INFO("OK! Clearing configurations");
            storage.Clear();
            UI_LOG_INFO("OK! Cleared all configurations");
            break;
        }
        case Configuration::Set_Sframe:
        {
            if (packet->length != 16)
            {
                serial.ReplyNack();
                UI_LOG_ERROR("ERR. Sframe key is too short!");
                break;
            }

            if (storage.Save(ConfigStorage::Config_Id::Sframe_Key, packet->payload, packet->length))
            {
                serial.ReplyAck();
                UI_LOG_INFO("OK! Saved SFrame Key configuration");
            }
            else
            {
                serial.ReplyNack();
                UI_LOG_ERROR("ERR. Failed to save SFrame Key configuration");
            }
            break;
        }
        case Configuration::Get_Sframe:
        {
            ConfigStorage::Config config = storage.Load(ConfigStorage::Config_Id::Sframe_Key);
            if (config.loaded && config.len == 16)
            {

                // Copy to a buff and print it.
                char buff[config.len + 1] = {0};
                for (int i = 0; i < config.len; ++i)
                {
                    buff[i] = config.buff[i];
                }
                buff[config.len + 1] = '\n';

                serial.Write(config.buff, config.len);
            }
            else
            {
                UI_LOG_INFO("ERR. No sframe key in storage");
            }
            break;
        }
        case Configuration::Toggle_Logs:
        {
            serial.ReplyAck();
            Logger::Toggle();
            break;
        }
        case Configuration::Disable_Logs:
        {
            serial.ReplyAck();
            Logger::Disable();
            break;
        }
        case Configuration::Enable_Logs:
        {
            serial.ReplyAck();
            Logger::Enable();
            break;
        }
        case Configuration::Get_Preamp:
        {
            // UI_LOG_INFO("Got preamp command");
            char str[3] = {0};
            int len = 0;
            uint16_t preamp = audio.MicVolume();
            int_to_arr(preamp, str, len, 10);
            str[len++] = '\n';

            serial.Write((uint8_t*)str, len);
            break;
        }
        case Configuration::Preamp_Up:
        {
            serial.ReplyAck();
            audio.MicVolumeUp();
            break;
        }
        case Configuration::Preamp_Down:
        {
            serial.ReplyAck();
            audio.MicVolumeDown();
            break;
        }
        case Configuration::Get_Volume:
        {
            uint16_t volume = audio.Volume();
            serial.Write((uint8_t*)&volume, sizeof(volume));
            break;
        }
        case Configuration::Volume_Up:
        {
            serial.ReplyAck();
            audio.VolumeUp();
            break;
        }
        case Configuration::Volume_Down:
        {
            serial.ReplyAck();
            audio.VolumeDown();
            break;
        }
        default:
        {
            UI_LOG_ERROR("ERR. No handler for received packet type");
            break;
        }
        }
    }
}