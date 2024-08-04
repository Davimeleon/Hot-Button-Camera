# Hot-Button-Camera

## Overview
This repository provides two programs that enable video recording using the SeekThermal Starter Kit with a Windows Laptop:
1. **seekcamera-simple**: Trigger recordings using your computer's keyboard.
2. **seekcamera-daq**: Trigger recordings using a data acquisition device (DAQ) based on analog signals, such as those from a pulse stimulator.

## Instructions

### Operating Camera from Computer
1. **Download Hot-Button-Camera Software:**
   - Open Command Prompt.
   - Navigate to the directory where you want to download the software.
   - Clone the GitHub repository: `git clone https://github.com/Davimeleon/Hot-Button-Camera.git`
2. Plug in the SeekThermal camera.
3. Navigate to `x64-windows/bin` and click on `seekcamera-simple`.

### Operating Camera with Pulse Stimulator and DAQ
1. **Download Hot-Button-Camera Software:**
   - Open Command Prompt.
   - Navigate to the directory where you want to download the software.
   - Clone the GitHub repository: `git clone https://github.com/Davimeleon/Hot-Button-Camera.git`
2. Download Instacal.
3. Plug in the SeekThermal camera and the Measurement Computing DAQ device.
4. Navigate to `x64-windows/bin` and click on `seekcamera-daq`.

### Modifying Programs
1. Download [Visual Studio](https://visualstudio.microsoft.com/vs/community/).
2. Download [CMake](https://cmake.org/download/).
3. **Download Hot-Button-Camera Software:**
   - Open Command Prompt.
   - Navigate to the directory where you want to download the software.
   - Clone the GitHub repository: `git clone https://github.com/Davimeleon/Hot-Button-Camera.git`
4. **Create Build Folder:**
   - Go to the `x64-windows/programs` directory in Command Prompt.
   - Run the following commands:
     ```sh
     mkdir build
     cd build
     cmake ..
     ```
5. Open the `build` folder in File Explorer and click on `seekcamera_programs.sln` to find the projects and code for `seekcamera-simple` and `seekcamera-daq`.

## Help
For a step-by-step tutorial on running the code and downloading the required software, watch this [video tutorial](https://vimeo.com/585062558).
