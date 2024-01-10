<script lang="ts" setup>

import { defineComponent, getCurrentInstance, onUpdated, reactive, ref } from "vue";

import LogItem from "@/components/LogItem.vue";

import HactarFlasher from "@/classes/flasher";

import logger from "@/classes/logger";
import { LogLevel } from "@/classes/logger";

let enabled_log_levels = LogLevel.Info | LogLevel.Warning | LogLevel.Error | LogLevel.Debug;
logger.SetLogLevel(enabled_log_levels);

const log_list = ref(null);
const adv_container = ref(null);
const log_list_key = ref(0);
let auto_scroll: number = 1;

let flasher: HactarFlasher = new HactarFlasher();

// TODO Serial monitor
// TODO serial commands to ui and net
// TODO set configurations for each chip

async function FlashHactar()
{

    try
    {
        const filters = [
            { usbVendorId: 6790, usbProductId: 29987 }
        ];


        if (!await flasher.ConnectToHactar(filters))
        {
            return;
        }

        await flasher.Flash("mgmt");
    }
    catch (exception)
    {
        logger.Error(exception);
        await flasher.serial.ClosePortAndNull();
    }
}

async function ShowAdvancedOptions()
{
    if (adv_container.value == null)
    {
        return;
    }

    (adv_container.value as any).classList.toggle("hide");
    (log_list.value as any).scrollTop = (log_list.value as any).scrollHeight;
}

async function TestLog()
{
    logger.Debug("test");
    logger.Info("test")
    logger.Warning("test");
    logger.Error("test");
}

function UpdateLogLevel(log_level: any)
{
    enabled_log_levels = enabled_log_levels ^ log_level.target.value;
    log_list_key.value += 1;
}

function EnableAutoScroll(event: any)
{
    auto_scroll = auto_scroll ^ event.target.value;
}

function AutoScroll()
{
    if (!auto_scroll) return;
    (log_list.value as any).scrollTop = (log_list.value as any).scrollHeight;
}

onUpdated(() =>
{
    AutoScroll();
});


</script>

<template>
    <div class="user_info__container">
        <div class="user_info">
            <button @click="FlashHactar()">Update Hactar</button>
            <!-- <p>{{ user_info.text }}</p> -->
        </div>
    </div>
    <div class="adv_button__container">
        <p>Advanced info</p>

        <button @click="ShowAdvancedOptions()">Toggle Log Console</button>
        <button @click="TestLog()">test log</button>
    </div>
    <div ref="adv_container" class="adv__container">
        <div class="adv__options">
            <div class="adv__wrapper__checkbox">
                <h4>Logging options:</h4>
                <label>
                    <input type="checkbox" class="adv__checkbox" name="log_info" @input="UpdateLogLevel"
                        :value="LogLevel.Info" checked />
                    Info
                </label>
                <label>
                    <input type="checkbox" class="adv__checkbox" name="log_warning" @input="UpdateLogLevel"
                        :value="LogLevel.Warning" checked />
                    Warning
                </label>
                <label>
                    <input type="checkbox" class="adv__checkbox" name="log_error" @input="UpdateLogLevel"
                        :value="LogLevel.Error" checked />
                    Error
                </label>
                <label>
                    <input type="checkbox" class="adv__checkbox" name="log_debug" @input="UpdateLogLevel"
                        :value="LogLevel.Debug" checked />
                    Debug
                </label>
                <label>
                    <input type="checkbox" class="adv__checkbox" name="log_autoscroll" @input="EnableAutoScroll" :value="1"
                        checked />
                    Auto scroll
                </label>
            </div>
        </div>
        <div ref="log_container" class="log__container">
            <ul ref="log_list" class="log__list" :key="log_list_key">
                <template v-for="log, idx in logger.logs" :key="'log' + idx">
                    <LogItem v-if="!log['gui_overwrite'] && (log['log_level'] & enabled_log_levels) > 0" :log="log">
                    </LogItem>
                </template>
            </ul>
        </div>
        <div class="adv__command_wrapper">
            <p>
                Command
            </P>
            <input type="input">
            <button>Send</button>
        </div>
    </div>
</template>
