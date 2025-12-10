#include "mgmt_packet_handler.hh"
#include "logger.hh"
#include "ui_mgmt_link.h"

void HandleMgmtLinkPackets(AudioChip& audio, Uart& serial, ConfigStorage& storage)
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
                char buff[config.len] = {0};
                for (int i = 0; i < config.len; ++i)
                {
                    buff[i] = config.buff[i];
                }

                UI_LOG_RAW("%s", config.buff);
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
            uint16_t preamp = audio.Preamp();

            UI_LOG_RAW("%u", preamp);
            break;
        }
        case Configuration::Set_Preamp:
        {
            // Get two bytes
            uint16_t preamp = 0;
            packet->Get(&preamp, 2, 0);

            UI_LOG_INFO("Try to set preamp to %u", preamp);
            audio.PreampSet(preamp);
            UI_LOG_INFO("Preamp = %u", audio.Preamp());
            break;
        }
        case Configuration::Preamp_Up:
        {
            serial.ReplyAck();
            audio.PreampUp();
            break;
        }
        case Configuration::Preamp_Down:
        {
            serial.ReplyAck();
            audio.PreampDown();
            break;
        }
        case Configuration::Get_Volume:
        {
            uint16_t volume = audio.Volume();

            UI_LOG_RAW("%u", volume);
            break;
        }
        case Configuration::Set_Volume:
        {
            // Get two bytes
            uint16_t volume = 0;
            packet->Get(&volume, 2, 0);

            UI_LOG_INFO("Try to set volume to %u", volume);
            audio.VolumeSet(volume);
            UI_LOG_INFO("Volume = %u", audio.Volume());
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