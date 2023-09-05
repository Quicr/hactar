<script setup lang="ts">

import { reactive, ref } from "vue";
import axios from "axios";

import LogItem from "@/components/LogItem.vue";

import HactarFlasher from "@/classes/hactar_flasher";
let logs: any = reactive([]);
let log_idx = 0;
let progress = "";
let progress_display = "";
let progress_dot_last_time = 0;
let Progress_Dot_Interval = 1000;

const log_list = ref(null);
const log_container = ref(null);
let auto_scroll = true;

let user_info = reactive({ text: "" });

let flasher: HactarFlasher = new HactarFlasher();


async function FlashHactar()
{

    try
    {
        const filters = [
            { usbVendorId: 6790, usbProductId: 29987 }
        ];

        log_idx = 0;

        if (await flasher.ConnectToHactar(filters))
        {
        }

        const log_progress_interval = setInterval(() =>
        {
            GetProgressUpdate();
            GetLogs();
        }, 5);

        // Get the ui bin from the server
        let res = await axios.get("http://localhost:7775/");
        let data = res.data.data;

        await flasher.FlashUI(data);

        clearInterval(log_progress_interval);

        user_info.text = "Update complete";
    }
    catch (exception)
    {
        console.error(exception);
        await flasher.ClosePortAndNull();
    }
}

function GetProgressUpdate()
{
    if (progress != flasher.progress)
    {
        progress = flasher.progress;
        progress_display = flasher.progress;
        progress_dot_last_time = Date.now();
    }
    else if (Date.now() - progress_dot_last_time > Progress_Dot_Interval)
    {
        progress_display += ".";
        progress_dot_last_time = Date.now();

        if (progress_display.slice(-4) == "....")
        {
            progress_display = progress;
        }
    }

    user_info.text = progress_display;
}

function GetLogs()
{
    if (flasher.logs.length == log_idx)
        return;

    let log = flasher.logs[log_idx++];
    if (log["replace_previous"])
        logs.splice(logs.length - 1, 1);
    logs.push(log);

    if (auto_scroll && log_list.value)
    {
        (log_list.value as any).scrollTop =
            (log_list.value as any).scrollHeight + 19;
    }
}

async function ToggleLogConsole()
{
    if (log_container.value != null)
        (log_container.value as any).classList.toggle("hide");
}

</script>

<template>
    <div class="user_info__container">
        <div class="user_info">
            <button @click="FlashHactar()">Update Hactar</button>
            <p>{{ user_info.text }}</p>
        </div>
    </div>
    <div class="adv_button__container">
        <div class="adv_info">
            <p>Advanced info</p>

            <button @click="ToggleLogConsole()">Toggle Log Console</button>
        </div>
    </div>
    <div ref="log_container" class="log__container hide">
        <ul ref="log_list" class="log__list">
            <LogItem v-for="log in logs" :text="log.text"></LogItem>
        </ul>
    </div>
</template>
