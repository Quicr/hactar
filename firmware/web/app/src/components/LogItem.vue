<script lang="ts">
import { defineComponent } from 'vue';


export default defineComponent({
    name: "LogItem",
    props:
        [
            "text"
        ],
    methods:
    {

    }
});

</script>

<template>
    <li class="log__item">
        {{ text }}
    </li>
</template>