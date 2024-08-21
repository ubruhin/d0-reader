# Sintio Plug Firmware

## Set up Toolchain

The
[ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/v5.2.2/esp32c6/get-started/index.html)
toolchain **v5.2.2** is required to build the firmware.

To work with VSCode, the toolchain does not need to be installed manually.
Just install the following VSCode extensions:

- [ESP-IDF](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension) by following [this guide](https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/tutorial/install.md)
- [`C/C++`](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
  for C/C++ support
- [Doxygen Documentation Generator](https://marketplace.visualstudio.com/items?itemName=cschlosser.doxdocgen)
  for generating Doxygen comments

## Build

    idf.py build

## Flash

    idf.py flash

## Flash & Run/Monitor Application

    idf.py flash monitor
