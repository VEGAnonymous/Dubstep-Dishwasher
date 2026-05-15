# Dubstep Dishwasher

The **Dubstep Dishwasher** is a modular multi-effects guitar effects pedal built around a modern, software-centric architecture that greatly expands the creative possibilities of traditional stompboxes. It is controlled entirely from an Android app over Bluetooth, supporting dynamically routed serial and parallel effect chains, twenty distinct effects (including many not commonly found in commercial pedals), a built-in parameter modulation system akin to Serum, and preset storage/recall, all configurable live.

## Features

- **20 real-time audio [effects](docs/api-reference/Effects.md)** implemented from scratch in C++, including several (e.g., vocoder, granulator, spectral gate, formant shifter, buffer looper with spectral resynthesis) not commonly found in commercial hardware pedals
- **Configurable effect chain** — add, remove, reorder, and bypass effects live without interruption; supports parallel subchains and persistent preset storage/recall
- **Parameter modulation engine** — many-to-many parameter routing, user-defined piecewise LFO curves, Perlin noise / sample-and-hold generators, and expression pedal mapping
- **Android companion app** — interface with the pedal over Bluetooth for complete effect chain control, real-time modulation visualization, LFO curve editing, and preset management

![App GUI](https://u.pone.rs/erkaqzay.png "App GUI")

## Documentation

See available documentation [here](docs/index.md).

## Build

KiCad project files can be found under [Releases](https://www.github.com/VEGAnonymous/Dubstep-Dishwasher/releases), which includes:

- Bill of Materials .csv
- Circuit schematic
- PCB design
- Pregenerated gerbers and drill files

![Schematic](https://u.pone.rs/rhbmjwok.png "Circuit Schematic")

<p align="center">
  <img src="https://u.pone.rs/ewnjdmxg.PNG" />
</p>

The PCB measures 85 x 86.3 mm and is designed to fit within a standard Hammond 1590BB enclosure. The PCB design may be and should be adjusted to suit component availability, sizing, and other demands (or for potential improvements). For example, the ESP32 devboard can be swapped out for a module if preferred.

Other necessary components not mentioned in the BOM include:
- [Teensy Audio Shield](https://www.pjrc.com/store/teensy3_audio.html) — soldered directly to the Teensy 4.1. The LINE nets on the PCB should be connected directly to its LINE IN L and LINE OUT L pins accordingly.
- [PSRAM](https://www.pjrc.com/store/psram.html) — 8 MB chip(s) soldered directly to the Teensy 4.1. These are *required* to run effects which use heavy buffers without crashing.
- **Rotary potentiometers** — 100k logarithmic; right-angle PCB mount with long pins recommended.
- **Antenna** — for facilitating ESP32 BLE connection. IPEX/u.FL/IPX to RP-SMA adapter cable recommended. The bulkhead should be mounted to the enclosure.

Standard pedal components (jacks, switches, etc., indicated by JST-XH connector footprints) sourced at user discretion.

It is recommended to build and upload the firmware for the Teensy 4.1 and ESP32 using [PlatformIO](https://platformio.org/). Open the repository root as a PlatformIO project; the provided configuration file [platformio.ini](platformio.ini) should suffice.

The app .apk is also included with each release. If building manually, it is recommended to use [Android Studio](https://developer.android.com/studio). Open `src/Android/` as a Gradle project. Requires Android 10+ (API 29) for BLE compatibility.

An example work-in-progress build is shown below:

![WIP Build](https://github.com/user-attachments/assets/aa53ea21-6d31-4bf7-961f-b282787d4c66 "WIP Build")
