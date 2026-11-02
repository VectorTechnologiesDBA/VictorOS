# VictorOS

**VictorOS is custom firmware for the Anki Vector robot, built on [WireOS](https://github.com/os-vector/wire-os) and pulled back toward the original Anki experience.**

A Vector running VictorOS should feel like the robot you took out of the box: the original Anki boot, processing and backpack lights (including the low-battery light), a clean setup screen, and stock behavior, while keeping the openness of custom firmware.

This repo is the `/anki` folder of the OS (the robot's personality, setup flow and services).

## Download

Get the latest OTA from the [Releases page](https://github.com/VectorTechnologiesDBA/VictorOS/releases/latest), or flash it straight from VTCS.

### Flashing

1. Put your Vector in recovery mode (place him on the charger and hold the backpack button for about 15 seconds).
2. Open the setup page and connect to your Vector.
3. Enter this OTA URL:

```
http://server.vectortech.dev:8088/vic/full/latest.ota
```

Updating keeps your Vector's data: his account link, name, Wi-Fi and settings carry over.

## VTCS (Vector Tech Community Server)

VictorOS is made to work with **VTCS**, the Vector Tech Community Server at `server.vectortech.dev`. It replaces Anki's cloud, so there's nothing to host yourself.

- **Accounts:** sign up in the [VTCS Vector app](https://github.com/VectorTechnologiesDBA/vtcs-app/releases/latest) or at [vectortech.dev/account.html](https://vectortech.dev/account.html). Your Vectors are linked to your account, and you can remove one there after a factory reset.
- **Voice commands, weather and knowledge graph:** set per robot from the account page (OpenWeatherMap or WeatherAPI.com; OpenAI, Together AI, any OpenAI-compatible endpoint, or Houndify).
- **Weather location:** set per robot on the account page.
- **Extras anywhere:** Drive, camera, Talk, eye color and quick actions work even when your Vector is on a different network from you. Vectors outside the server's network keep a secure relay connection to VTCS (`vtcs-relay`).
- **Automatic updates:** Vectors check VTCS for new VictorOS versions and install them on their own. You can also check from the app under **Settings → Updates**.

Prefer to run your own server? VictorOS also works with [WirePod](https://github.com/kercre123/wire-pod). The [setup guide](https://vectortech.dev/v.html) walks through both.

## What's changed from WireOS

- Setup screen with the robot's name, a teal V spinning on a tablet, and `vectortech.dev/v`
- Original Anki boot sweep, processing and backpack lights
- Points at VTCS for its cloud, OTA updates and the relay
- Update checks from the app's Updates screen, and automatic updates on dev builds
- Mic data tap off unless enabled (`/data/mic_tap_dest`)

## Building

VictorOS builds the same way as WireOS. Follow the build instructions in [wire-os](https://github.com/os-vector/wire-os) and [wire-os-victor](https://github.com/os-vector/wire-os-victor), using this repo in place of `wire-os-victor`.

## Credits

- **Wire** ([os-vector](https://github.com/os-vector)) for WireOS, which VictorOS is built on
- **kercre123** for [WirePod](https://github.com/kercre123/wire-pod)
- **Anki** and **Digital Dream Labs** for Vector
- Built by **Jaceroo404** and **DevHayes** at [Vector Tech](https://vectortech.dev)
