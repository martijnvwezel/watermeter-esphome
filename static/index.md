# Muino Water-Meter Reader - sub-100 millilitre precision

<p class="lead">The Muino Smart Water Meter is a <strong>single-board</strong> device that measures water consumption with <strong>sub-100 millilitre</strong> accuracy. The other big benefit is the <strong>ease of installation</strong>, for friends/family that wanted a similar solution this is easier to use.</p>

<div class="hero">
  <img src="/img/muino_with_case.png" alt="Muino water meter reader"/>
  <div class="install-box">
    <h2>Update your watermeter with a clean binary</h2>
    <p>You can use the button below to install the pre-built firmware directly to your device via USB from the browser.</p>
    <esp-web-install-button manifest="firmware/project-template.manifest.json"></esp-web-install-button>
    <p class="small">First time? Follow the <a href="/installation/">installation steps</a>.</p>
  </div>
</div>
<script type="module" src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module"></script>

## How it works

Water meters are devices that measure how much water you use. They have a spinning disk inside them, and each time it spins all the way around, it means you've used one liter of water. Most water meter readers use a simple method: they check if a metal disk is there or not. But the Muino water meter is different. It uses three light sensors to keep track of where the disk is. It uses some smart techniques to do this, and we use calculate with some fine adjustments to get things just right. This helps the Muino water meter measure very accurately, down to almost a millimeter. But remember, the spinning disk doesn't move perfectly like a smooth wave. So, in some parts of its rotation, the measurements might jump a bit more than in other parts.
