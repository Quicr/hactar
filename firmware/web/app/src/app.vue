<script setup lang="ts">
import { RouterLink, RouterView } from "vue-router";
// import Greeting from "./components/Greeting.vue";
</script>

<template>
  <header>
    <h1 class="blue">Hactar Flasher</h1>
  </header>

  <RouterView />
</template>

<style scoped>
header {
  line-height: 1.5;
  max-height: 100vh;
}

nav a:first-of-type {
  border: 0;
}

@media (min-width: 1024px) {
  header {
    text-align: center;
  }

  header .wrapper {
  }

  nav {
  }
}
</style>
