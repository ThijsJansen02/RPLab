# RPLab
## introduction
RPLab is a graphical user interface to interact with, and run custom DSP fpga configurations on Red Pitaya development boards. currently RPLab is only available on windows PC.

## Build
**currently RPLab is only buildable on windows!**

RPLab is build on the [LabCore / Physica](https://github.com/ThijsJansen02/Physica/tree/dev-RPLab) framework and adds this as a submodule. Currently this software requires a [VulkanSDK](https://vulkan.lunarg.com/sdk/home) installation on your computer. When this SDK is installed the the premake buildconfig automatically figures out where it is located.

### Clone the repository
To clone the repository, it is important that the repository is cloned recursively. Clone with:
```
git clone --recursive https://github.com/ThijsJansen02/RPLab <target directory>
```
Now there should be a complete copy of the project in the target directory.

### Build the repository
This repository uses [premake](https://premake.github.io/) to generate the necessary buildfiles. Premake does not require any additional installs. To generate the buildfiles, run:
```
buildconfig\\premake5 <target>
```

Now there should be a buildable solution in your repository. Premake can generate many different buildfiles, see the premake website for all available options. However, this project is tested and developed using the MSVC compiler in [Visual Studio 2026](https://visualstudio.microsoft.com/).

### Building for visual studio 2026
To build for visual studio 2026, supply "vs2026" as <target> to the premake build instruction.
```
buildconfig\\premake5 vs2026
```
Now there should be a file name `RPLab.slnx` in your root directory. Open this file with Visual Studio to open the solution. To build a distributable version of the software, select "dist" as a configuration and build the solution with F5.
