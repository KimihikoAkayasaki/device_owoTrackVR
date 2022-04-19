<h1 dir=auto>
<b>owoTrackVR</b>
<text style="color:#9966cc;">Amethyst</text>
<text>device (plugin)</text>
</h1>

## <ins>__[Discord server](https://discord.gg/YBQCRDG)__</ins> and I'm **公彦赤屋先#5023**

## **License**
This project is licensed under the GNU GPL v3 License 

## **Downloads**
You're going to find built plugins in [repo Actions](https://github.com/KimihikoAkayasaki/device_owoTrackVR/actions), in the run artifact's section.

## **Build & Deploy**
Both build and deployment instructions [are available here](https://github.com/KimihikoAkayasaki/device_owoTrackVR/blob/main/BUILD_AND_DEPLOY.md).

## **Overview**
This repo is an implementation of [owoTrack](https://www.reddit.com/r/VRchat/comments/kw57ib/owotrack_a_free_android_phone_based_virtual_hip/) as a wrapped [Amethyst](https://github.com/KinectToVR/Amethyst-Releases) plugin.<br>
Please thank the original devs for making this. Used owo repos: [ [server](https://github.com/abb128/owo-track-driver), [client](https://github.com/abb128/owo-track-overlay) ]

## **Wanna make one too? (K2API Devices Docs)**
[This repository](https://github.com/KinectToVR/K2TrackingDevice-Samples) contains sample projects of devices / plugins in terms of `Amethyst` project.<br>
Each project in the solution is an example of a different possible implementation<br>
of a tracking device (Later referred to as `K2TrackingDevice`), and an additional one with settings.<br>

<ins>[You can find a detailed description about every single plugin and device type here.](https://github.com/KinectToVR/K2TrackingDevice-Samples/blob/main/DEVICES.md)</ins>

Briefly, currently supported device types are:
- Tracking providers:
  - `JointsBasis` - Provide a vector of named, tracked joints
  - `KinectBasis` - Provide a fixed array of enumerated, tracked joints
    + `Full` Character - Provide Joint_Total joints (as many as Kinect V2 does)
    + `Simple` Character - Provide [ Head, Elbows, Waist, Knees, Ankles, Foot Tips ]
    + `Basic` Character - Provide [ Head, Waist, Ankles ]
- `Spectator` - Only poll data from the app

Please note that `KinectBasis` types can use automatic calibration, whereas `JointsBasis` cannot.<br>
Additionally, plugins (devices) can provide their own settings, as [described here](https://github.com/KinectToVR/K2TrackingDevice-Samples/blob/main/DEVICES.md#device-settings) and [shown here](https://github.com/KinectToVR/K2TrackingDevice-Samples/blob/main/device_KinectBasis_Full_Settings/DeviceHandler.h#L28).