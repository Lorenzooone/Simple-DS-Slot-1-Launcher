# Simple DS Slot-1 Launcher
An extremely simple homebrew to launch the game currently inside of slot 1.

For Nintendo DS consoles only, to keep it simple. It will not work on DSi and 3DS.

## Usage
The homebrew will try to auto-launch what is present in slot 1. If nothing is found, it will display a small menu to manually launch the game.

Holding DEBUG or SELECT at startup will delay the first check. Holding B at startup will prevent auto-launching.

## Credits
Based off TWiLightMenu's slot1launch, which was based off NTR Launcher - Apache Thunder - Original code from NitroHax but with cheat engine/menu stripped out.

## Building
Docker allows easily building without having to install the dependencies.
- To build for the NDS, while in the root of the project, run: `docker run --rm -it -v ${PWD}:/home/builder/slot1launch lorenzooone/slot1launch:nds_builder`
