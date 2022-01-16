# RenderBender
A third party program to change Minecraft RTX's settings externally, directly in-memory.
Get the latest release [here](https://github.com/SpeedyCodes/RenderBender/releases).

## About
RenderBender is a third party program that aims to improve Minecraft RTX's look.
This is achieved by editing the settings in-memory, meaning that there is little to no risk of breaking the game.
If you do manage to break something, everything goes back to normal as soon as you restart the game.

Currently, its features are simple: when you start up the program, 
it **displays the current value of the settings, and lets you edit them**.
Settings are specified in a JSON file, which you can write yourself, or get from the Minecraft RTX discord server.

There are many more features to come, such as
- Presets
- Keybinds to toggle presets
- Dimension-specific presets
- Preset layer system
- User-made scripts for custom behaviour (i.e. wet ground during rain)

For more information about upcoming features, and in what order they will be worked on, see the Roadmap section.

## Usage

The latest version of RenderBender can be found in the `release` section.
Make sure you download the JSON file that matches your Minecraft version. (Currently, only release 1.18.2 is supported).
If you require assistance, the [Minecraft RTX Discord server](https://www.discord.gg/minecraftrtx) can help you (under the #mc-support channel).
Unzip the file, and run RenderBender.exe.
When prompted for a JSON file, click the button to locate the previously downloaded JSON.
All settings included in the JSON file should appear. You can now change them by entering a new value, toggling the checkbox or using the arrow buttons/keys.
Have fun!

## Roadmap

These are the planned features for the next few updates. While they are all considered necessary and valuable additions, their order may still change in the future.

Current version: **`0.1.0`**

##### 0.1.1 update
- Loading the static memory offset from the settings JSON file (allowing other versions of Minecraft)
- Sorting setting by category (maybe in tabs)
##### 0.2 release
- Replace spinboxes with sliders (maybe in 0.1)
- People can create and share their own presets.
- Installation program made available besides the .zip file
##### 0.2.1 update
- Sorting settings by category in different tabs
##### 0.2.2 update
- Descriptions + example images

More features are to come in the more distant future.
## Licenses

- This project is built on the Qt graphics library, licensed under the GPL v3 license, available in `QT_LICENSE`.
- This project uses code from Colin Duquesnoy's QDarkStyleSheet (https://github.com/ColinDuquesnoy/QDarkStyleSheet), under the MIT license.
It also uses the dark theme icons under the CC-BY license (no changes were made to the icons as they were on the 4th of november 2021). Many thanks to the maintainers: Colin Duquesnoy, Daniel Pizetta, Gonzalo Peña-Castellanos, Carlos Cordoba, and to the contributors: mowoolli, Xingyun Wu, KcHNST, goanpeca, tsilia, isabela-pf, juanis2112, ccordoba12.
- This project itself is licensed under the GPL v3 license, available in the `LICENSE` file.
**Important: RenderBender is in no way associated with, or approved by, the QDarkStyleSheet authors.**
Both licenses can be found in `qdarkstyle/LICENSE.rst`.

## Credits

- **Sleepi** - Originally started the project by discovering the hidden RTX settings within the game's memory. Later wrote the presets script for Cheat Engine and is currently helping with pointer logic and feature suggestions.
- **Speedy (Me)** -  Helped Sleepi with research and later, turned it into the working desktop application we have today.
- **MADLAD** - Helped with discovering the settings, finds memory-pointers for every new version (making everything actually work).
- **Ngārara Ariki** - Also helped with Cheat Engine here and there, Discovered existance of settings along with Sleepi.

## Miscellaneous
- During early development, [this](https://www.youtube.com/watch?v=wiX5LmdD5yk) tutorial by GuidedHacking on YouTube was of great help.
- If you encounter any bugs or have a suggestion, please contact us in the Minecraft RTX discord server (https://www.discord.gg/minecraftrtx) 
- **THIS IS NOT AN OFFICIAL MINECRAFT PRODUCT. IT IS NOT APPROVED BY OR ASSOCIATED WITH MOJANG.**
