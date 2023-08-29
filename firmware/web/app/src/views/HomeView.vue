<script setup lang="ts">
import { defineComponent } from 'vue';

var port:any = null;

function mounted()
{
  console.log("hello");
}

async function FlashFirmware()
{

  if ('serial' in navigator)
  {
    const filters = [
      { usbVendorId: 6790, usbProductId: 29987 }
    ];
    port = await (navigator as any).serial.requestPort({ filters });
    await port.open({ baudRate: 115200, bufferSize:1024});
    const writer = port.writable.getWriter();
    const data = new Uint8Array([117, 105, 95, 117, 112, 108, 111, 97, 100])

    await writer.write(data);

    writer.releaseLock();

  }
  else
  {
    alert("Web serial is not enabled in your browser");
  }
  // Send off request to server for the latest file

  // Save it locally on the web browser

  //

  // Delete the hactar code
  console.log("hello")
}

async function ClosePort()
{
  if (port != null)
  {
    await port.close()
  }
}

</script>

<template>
  <main>
    <button @click="FlashFirmware()">Flash hactar app</button>
    <button @click="ClosePort()">Close port</button>
  </main>
</template>
