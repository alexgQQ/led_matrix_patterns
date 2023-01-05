## Matrix Pattern Library

This is a repo for creating motion based animations with multiple led strips or a led matrix.

## Install

This library makes a lot of use of a [specific fork of the Neopixel lib](https://github.com/moose4lord/Adafruit_NeoPixel) and is required for usage. Additionally the `knob_control` sketch uses the [Encoder](https://www.arduino.cc/reference/en/libraries/encoder/) library. Either download and import the zips or install from their repos/managers. 

You can download the zip file for this [repo](https://github.com/alexgQQ/led_matrix_patterns) and add it through the traditional Arduino IDE. Additionally you can install direclty from the repo from the repo too.
```bash
arduino-cli lib install Encoder  # Only needed for the knob control example sketch
arduino-cli lib install --git-url https://github.com/moose4lord/Adafruit_NeoPixel
arduino-cli lib install --git-url https://github.com/alexgQQ/led_matrix_patterns
```

## Development Setup

This specifically uses the [arduino-cli](https://arduino.github.io/arduino-cli/0.29/installation/#download). Additionally the Pro Mini is connected through an FTDI chip and their [drivers](https://ftdichip.com/drivers/) are required.

This is only required once after installing the cli. Install the base avr uploader, allow installing libraries from git repos and install third party code libraries.  
```powershell
arduino-cli core install arduino:avr
arduino-cli config init
arduino-cli config set library.enable_unsafe_install true
# Default location on windows but needs to be specified
arduino-cli config set directories.user "$HOME\Documents\Arduino"

arduino-cli lib install Encoder
arduino-cli lib install --git-url https://github.com/moose4lord/Adafruit_NeoPixel
```

Make a hard link from the local repo to the arduino library dir. For example, a default install on Windows may do this from powershell with:
```powershell
mkdir $HOME\Documents\Arduino\libraries\led_bars
cmd /c mklink /J $HOME\Documents\Arduino\libraries\led_bars $PWD
```

Now you can edit the library code and compile it against the examples. Check that the code compiles.
```powershell
arduino-cli compile --clean --fqbn arduino:avr:pro $PWD\examples\simple_cycle
```

Compile the code and upload. Check for the correct USB port before.
```powershell 
arduino-cli compile --clean --upload -p COM3 --fqbn arduino:avr:pro $PWD\examples\simple_cycle --build-property "build.extra_flags=-DLED_SEGMENTS=4"
```
