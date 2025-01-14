# Simple DS Slot-1 Launcher
An extremely simple homebrew to launch the Nintendo DS or Nintendo DSi game currently inside of slot 1.

Works on Nintendo DS, Nintendo DSi and Nintendo 3DS consoles.

## Usage
The homebrew will try to auto-launch what is present in slot 1. If nothing is found, it will display a small menu to manually launch the game.

Holding DEBUG or SELECT at startup will delay the first check. Holding B at startup will prevent auto-launching.

There are certain options which can be set to alter how the cartridge is launched, if not auto-launching.
These are different when the software is launched on a regular Nintendo DS and on a Nintendo DSi or Nintendo 3DS.
On Nintendo DSi and Nintendo 3DS, these options can also be saved to the SD card, so they're automatically loaded for future executions of the homebrew.

## Credits
Based off TWiLightMenu's slot1launch, which was based off NTR Launcher - Apache Thunder - Original code from NitroHax but with cheat engine/menu stripped out.

## Building
To build, you need to have BlocksDS installed.

Alternatively, Docker allows easily building without having to install the dependencies.
- To build for the NDS, while in the root of the project, run: `docker run --rm -it -v ${PWD}:/home/builder/slot1launch lorenzooone/slot1launch:nds_builder`
