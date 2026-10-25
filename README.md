# ezBasic

A Plutonium-based Nintendo Switch homebrew app that exposes the console as a USB HID keyboard and types an embedded SmileBASIC bootstrap into a connected USB host. The bootstrap is bundled in RomFS; users do not select or edit a file on the console.

## Features

- Bundled `romfs/bootstrap.bas`, based on the `kurumuz/ns2jb` fork.
- Strips leading BASIC comment and blank lines before typing.
- Preflights the payload against the supported keyboard mapping before sending any keystrokes.
- Waits for USB host configuration and HID report-descriptor acknowledgement before enabling typing.
- Shows typing progress and completion; **B** cancels and **+** exits.

## Install and use

1. Download `ezBasic.nro` from the [v1.0.0 release](https://github.com/StonedModder/ezBasic/releases/tag/v1.0.0).
2. Copy it to `/switch/ezBasic/ezBasic.nro` on the Switch SD card.
3. Connect the Switch to the target device with a USB data cable. The Switch acts as the USB device; the connected target must be the USB host.
4. Launch ezBasic from the Homebrew Menu. Close other apps that use `usb:ds` (for example, USB/MTP file-transfer apps) if the keyboard does not become ready.
5. Focus the appropriate text-entry field on the target. Wait for **USB ready**, then press **A** to type.

Test a short input and confirm the target's keyboard layout before using a full bootstrap. ezBasic does not verify that the target accepted the text.

## Build

Build with devkitPro, devkitA64, libnx, Plutonium's Switch dependencies, and `make` installed. From the project root in the devkitPro shell:

```sh
export DEVKITPRO=/opt/devkitpro
make -C Plutonium/Plutonium
make -j4
```

The output is `ezBasic.nro`. The NACP metadata uses title `ezBasic`, author `StonedModder`, version `1.0.0`.

### Host tests

```sh
g++ -std=c++17 -Wall -Wextra -Werror -Iinclude \
  tests/asset_smoke.cpp source/RomfsBootstrap.cpp source/Utils.cpp source/HidEncode.cpp \
  -o build/asset_smoke
./build/asset_smoke romfs/bootstrap.bas

g++ -std=c++17 -Wall -Wextra -Werror -Itests/mock -Iinclude \
  tests/typing_engine_usb.cpp source/TypingEngine.cpp source/HidEncode.cpp \
  -o build/typing_engine_usb
./build/typing_engine_usb
```

The USB simulator covers descriptor/status acknowledgement, reset recovery, DMA alignment, speed-specific descriptors, and HID report completion. It mocks `usb:ds`; it is not a substitute for testing on a Switch and a connected USB host.

### Updating the embedded payload

Replace `romfs/bootstrap.bas` with an authorized updated payload, then run `make`. The Makefile tracks this file as an NRO dependency, so no C++ changes are needed to refresh the asset.

## Credits and licensing

See [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) for upstream credits, included dependency notices, and the licensing status of the embedded bootstrap. The original ezBasic source has no separate license grant in this repository; third-party licenses do not apply to the original ezBasic code.
