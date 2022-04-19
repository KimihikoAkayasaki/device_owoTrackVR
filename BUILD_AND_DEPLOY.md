## **Build instructions**
You'll need:
 - The `device_owoTrackVR` repo cloned somewhere and `cd`'d into
 - (For testing purpose) Working installation of Amethyst and SteamVR

Follow these steps:

- [Install tools for the Windows App development](https://docs.microsoft.com/en-us/windows/apps/windows-app-sdk/set-up-your-development-environment?tabs=vs-2022-17-1-a%2Cvs-2022-17-1-b).<br>
  You'll have to install Visual Studio 2022 or its Build Tools.

- Clone `cereal` & `Eigen3` to `external/` and set up `GLog` & `GFlags`<br>
  (For this step you must have cmake installed and visible in PATH)<br>
  ```powershell
  # Clone cereal
  > git clone https://github.com/USCiLab/cereal ./external/cereal

  # Clone and setup Eigen3
  > git clone https://gitlab.com/libeigen/eigen ./external/eigen
  # Reset Eigen to the latest OK state
  > cd ./external/eigen
  > git reset --hard 1fd5ce1002a6f30e1169b529b291216a18be2f7e
  # Go back
  > cd ../..
  
  # Clone and setup GLog
  > git clone https://github.com/google/glog.git ./external/glog
  # Reset GLog to the latest OK state and build it
  > cd ./external/glog
  > git reset --hard f8c8e99fdfb998c2ba96cfb470decccf418f0b30
  > mkdir vcbuild; cd ./vcbuild
  > cmake .. -DBUILD_SHARED_LIBS=ON
  > &"$msbuild" glog.vcxproj "/p:Configuration=Release;Platform=x64"
  # Go back
  > cd ../../..

  # Clone and setup GFlags
  > git clone https://github.com/gflags/gflags.git ./external/gflags
  # Reset GFlags to the latest OK state and build it
  > cd ./external/gflags
  > git reset --hard 827c769e5fc98e0f2a34c47cef953cc6328abced
  > mkdir vcbuild; cd ./vcbuild
  > cmake .. -DBUILD_SHARED_LIBS=ON
  > &"$msbuild" gflags.vcxproj "/p:Configuration=Release;Platform=x64"
  # Go back
  > cd ../../..
  ```

- Build Samples:<br>
  ```powershell
  # Download the vswhere tool to find msbuild without additional interactions
  > Invoke-WebRequest -Uri 'https://github.com/microsoft/vswhere/releases/latest/download/vswhere.exe' -OutFile './vswhere.exe'
  # Use the downloaded vswhere tool to find msbuild. Skip this step if you use the Dev Powershell
  > $msbuild = "$("$(.\vswhere.exe -legacy -prerelease -products * -format json | Select-String -Pattern "2022" | `
        Select-String -Pattern "Studio" | Select-Object -First 1 | Select-String -Pattern "installationPath")" `
        -replace('"installationPath": "','') -replace('",',''))".Trim() + "\\MSBuild\\Current\\Bin\\MSBuild.exe"

  # Restore NuGet packages and build everything
  > &"$msbuild" device_owoTrackVR.sln "/p:Configuration=Release;Platform=x64"
  ```

## **Deployment**
The whole output can be found in `x64/Release` directory<br>
(or `x64/Debug` if you're building for `Debug`, naturally) and:
 - Devices (plugins) are inside `devices/` folder

Note: Debug builds will only work with a debug host,<br>
the same schema applies to OpenVR Driver, the API and Amethyst.