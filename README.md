# GPIO "Programmer" Keyboard Driver for Raspberry Pi

This is a simple Linux kernel module that turns two GPIO buttons on a Raspberry Pi into a virtual keyboard. Pressing one button types `0`, while pressing the other types `1`.

## Features
- Uses GPIO buttons as keyboard input
- Implements the Linux input subsystem
- Sends key events for `0` and `1` when buttons are pressed
- Works on Raspberry Pi 2B (tested with GPIO23 and GPIO24)

## Requirements
- Raspberry Pi 2B (or similar device with GPIO support)
- Linux kernel with module support
- Build tools (`make`, `gcc`, `linux-headers` package for your kernel)

## Installation

### 1. Clone the repository:
```bash
git clone https://github.com/yourusername/gpio-keyboard-driver.git
cd gpio-keyboard-driver
```

### 2. Build the module:
```bash
make
```

### 3. Load the module:
```bash
sudo insmod gpio_keyboard.ko
```

### 4. Check if the device is registered:
```bash
dmesg | grep -i "Programmer Keyboard"
```

## Unloading the Module
To remove the module, run:
```bash
sudo rmmod gpio_keyboard
```

## License
This project is licensed under the GPLv2 license.

## Author
Andrei Galitianu

