# FPVue_xr
WFB-ng android client, running rtl8812au driver in userspace and a low latency videodecoder in XR mode.

At this stage, this is only intended for developers.

## Compilation
```
git clone https://github.com/gehee/FPVue_xr.git
git submodule init
git submodule update
```
Create `local.properties` at the root of the project and set your SDK directory:

Example on windows:
```
sdk.dir=C\:\\Users\\...\\AppData\\Local\\Android\\Sdk
```

Open project in android studio and wait for gradle to init the project.

## Run the project

1. Connect the Quest 2 to your computer.
2. Open terminal and run `adb tcpip 5555` then connect adb to your ip ` adb connect 192.168.x.y`.
3. Unplug the usb cable from the Quest 2.
4. Launch the app for android studio then close it.
5. Plug an rtl8812 wifi adapter and wait for the popup asking to open FPVue XR when rtl8812 is plugged.
   Accept the prompt and check always open Open FPVue XR when rtl8812 is attached.

6. Start the app and you should get data on channel 149 (edit app.cpp to change channel). If it does not work, restart the app from android studio.