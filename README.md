# RenderBender
A third party program to change Minecraft RTX's settings externally, directly in-memory.
Get the latest release [here](https://github.com/SpeedyCodes/RenderBender/releases).

## About
RenderBender is a third party program that aims to improve Minecraft RTX's look.
This is achieved by editing the settings in-memory, meaning that there is little to no risk of breaking the game.
If you do manage to break something, everything goes back to normal as soon as you restart the game.

Currently, its features are simple: when you start up the program, 
it **displays the current value of the settings, and lets you edit them**.
Settings are specified in a JSON file which comes with the executable. It can be customised to your liking. 
Presets can be created to quickly apply a bunch of values at once.

There are many more features to come, such as
- Keybinds to toggle presets
- Dimension-specific presets
- A preset layer system
- User-made scripts for custom behaviour (i.e. wet ground during rain)

For all upcoming features, and in what order they will be worked on, see the Roadmap section.

## Usage

The most important features will be explained here. If you require assistance, the [Minecraft RTX Discord server](https://discord.gg/R56qgBBA9D) can help you (under the #mc-support channel).

The latest version can be found in the `release` section.
Download and unzip the file, and run RenderBender.exe. If Minecraft wasn't already running, RenderBender will start it for you (this behaviour can be disabled in Preferences).

A notification will pop up telling you the version number of the latest Minecraft version available at the time of writing. 

If that's the number of the version you're using: great! You can start using RenderBender immediately. All settings should appear with their correct values. 
You can now change them by entering a new value, toggling the checkbox, using the arrow buttons/keys or moving the sliders. Settings can be reset to their default value by using the Reset button next to the settings.

If you are using a different version however, you'll probably need to edit the static memory offset.
##### Editing the static memory offset
Go to `File/Preferences` and set the static memory offset for your specific Minecraft version. You can find values for some common versions here.
- Release 1.19.0: 0x0411AB28
- Release 1.19.2: 0x04177558
- Release 1.19.10: 0x0430A460
- Release 1.19.11: 0x0430A4C0
- Release 1.19.20: 0x044B0080

If your version isn't yet on the list, you'll have to ask the people in the previously mentioned Discord server for help. If you have a Cheat Engine Cheat Table, you can get it from there too, if you know how. 
If this all sounds rather complicated: we are currently working on a way for RenderBender to detect the static memory offset automatically.
##### Changing the settings JSON
Attention: most users will not need to do this, only do it if you know what you're doing.

Click the button on the main screen to locate the previously downloaded JSON. You can always switch to a new file/location by editing it in `File/Preferences` and then restarting.
All settings included in the JSON file should appear. 
##### Saving presets
When you've set the settings to values you like, you can save them by clicking Edit->Save Preset. Here, you can select the settings you want to store a preset for and you can enter a title. To load the values from a preset, click Edit->Load Preset->the title of the preset.

##### Importing Cheat Engine presets
If you want to use a preset from the #rtx-presets channel of the Minecraft RTX Discord server (originally intended to be used with Cheat Engine), you can click `File/Import old CE preset`. The string containing setting values can be pasted directly into the text box. It will be parsed and automatically converted to a RenderBender preset.

## Roadmap

These are the planned features for the next few updates. While they are all considered necessary and valuable additions, their order may still change in the future.

##### 0.3.1 update
- Descriptions + example images
- Installation program made available besides the .zip file
##### 0.4 update
- Minimize to system tray
- Keybinds to toggle presets

More features are to come in the more distant future, such as
- Automatic reading of the static memory offset
- A layer system for presets
- User-made scripts for custom behaviour (i.e. wet ground during rain)
    - Disable sky brightness, sun brightness gradually when below certain y-value
    - Smoothertron during and possibly after rain.
- Snapping sliders to default and round numbers (toggleable)
- Setting descriptions and example images
- Dimension-specific presets
- Editing radian values as if they were degrees
## Licenses

- This project is built on the Qt graphics library, licensed under the GPL v3 license, available in `QT_LICENSE`.
- This project uses code from Colin Duquesnoy's QDarkStyleSheet (https://github.com/ColinDuquesnoy/QDarkStyleSheet), under the MIT license.
It also uses the dark theme icons under the CC-BY license (no changes were made to the icons as they were on the 4th of november 2021). Many thanks to the maintainers: Colin Duquesnoy, Daniel Pizetta, Gonzalo Peña-Castellanos, Carlos Cordoba, and to the contributors: mowoolli, Xingyun Wu, KcHNST, goanpeca, tsilia, isabela-pf, juanis2112, ccordoba12.
**Important: RenderBender is in no way associated with, or approved by, the QDarkStyleSheet authors.**
Both licenses can be found in `qdarkstyle/LICENSE.rst`.
- This project itself is licensed under the GPL v3 license, available in the `LICENSE` file.

## Credits

- [Sleepi](https://github.com/bliksemremi) - Originally started the project by discovering the hidden RTX settings within the game's memory. Later wrote the presets script for Cheat Engine and is currently helping with pointer logic and feature suggestions.
- [Speedy](https://github.com/SpeedyCodes)(me) -  Helped Sleepi with research and later, turned it into the working desktop application we have today.
- [MADLAD](https://github.com/MADLAD3718) - Helped with discovering the settings, finds memory-pointers for every new version (making everything actually work).
- [Ngārara Ariki](https://github.com/Tui-Vao) - Also helped with Cheat Engine here and there, discovered existance of settings along with Sleepi.

## Miscellaneous
- During early development, [this](https://www.youtube.com/watch?v=wiX5LmdD5yk) tutorial by GuidedHacking on YouTube was of great help.
- If you encounter any bugs or have a suggestion, please contact us in the [Minecraft RTX discord server](https://discord.gg/R56qgBBA9D) 
- **THIS IS NOT AN OFFICIAL MINECRAFT PRODUCT. IT IS NOT APPROVED BY OR ASSOCIATED WITH MOJANG.**
