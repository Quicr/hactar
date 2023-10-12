<script lang="ts">
import { defineComponent } from 'vue';
import logger, { LogLevel, LogLevelToString, LogTime } from "@/classes/logger";
import type { LogEntry } from '@/classes/logger';

export default defineComponent({
    name: "LogItem",
    props:
    [
        "log",
    ],
    data()
    {
        return {
            log_level: LogLevel
        };
    },
    mounted()
    {
        this.ReplaceSibling();
    },
    methods:
    {
        LogLevelToString,
        LogTime,
        GetColourClass(log_level: any)
        {
            switch (log_level)
            {
                case LogLevel.Info:
                    return "log_entry--info";
                case LogLevel.Warning:
                    return "log_entry--warning";
                case LogLevel.Error:
                    return "log_entry--error";
                case LogLevel.Debug:
                    return "log_entry--debug";
                default:
                    return "";
            }
        },
        ReplaceSibling()
        {
            if (!this.log['gui_overwrite']) return;

            // We will just hide the element above this one
            const parent = (this.$refs['log_entry'] as any).parentNode;
            console.log(parent.children[parent.children.length -1]);
            logger.Info("hello");
            console.log(parent.children[parent.children.length -1]);

        }
    }
});

</script>

<template>
    <li ref="log_entry" class="log__entry">
        <span class="log_entry" :class="GetColourClass(log.log_level)">
            [{{ LogLevelToString(log['log_level']) }}]
            {{ LogTime(log) }} {{ log['text'] }}
        </span>
    </li>
</template>

<style>
.log_entry {
    font-family: 'Courier New', Courier, monospace;
}

.log_entry--info {
    color: white;
}

.log_entry--warning {
    color: yellow;
}

.log_entry--error {
    color: red;
}

.log_entry--debug {
    color: magenta;
}
</style>