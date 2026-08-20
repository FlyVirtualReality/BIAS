# Setting up a build environment on windows 7&8

> [!WARNING]
> This current state of the documentation is rescued from an archive file, matched BIAS-v0.58 
> released on Jun 28, 2020 and was partially outdated at that time. For some notes on how the 
> installation worked on a Windows 11 machine in 2024, have a look at the main [README](../README.md).

In this section we describe how to set up a build environment for bias on window (7 & 8). this process consists of the following steps:

We will describe the steps in more detail below. Note, for a trouble free build, it is good idea to use the the versions of gcc, qt and opencv which are suggested.

The instructions assume that git is installed. <https://git-scm.com/>

## Download and install gcc

Bias is currently built with the [tdm-gcc](https://web.archive.org/web/20211205201254/http://tdm-gcc.tdragon.net/) compiler suite. Version 4.7.1 is know to work. Note, this is a slightly older version of the compiler suite than what is currently available on the tdm-gcc web site. Older versions of tdm-gcc can be found on source forge [here](https://web.archive.org/web/20211205201254/http://sourceforge.net/projects/tdm-gcc/files/tdm-gcc%20installer/previous/1.1006.0). in particular the recommended versions tdm-gcc for 32 and 64 bit windows are as follows:

- 32bit tdm-gcc-4.7.1.exe
- 64bit tdm64-gcc-4.7.1-3.exe

The .exe downloaded will be an executable windows installer. Run the installer to install the compiler suite. There will be some options during the installation make sure the following items are checked

- gcc w/ the c++ sub-menu item
- binutils
- mingw64-runtime (or mingw-rutime if 32bit)
- mingw32-make
- gdb (if you want a debugger)
- start menu items
- add to path

After running the installer double check the compiler suite has been added to the windows path. For example, if the compiler suites is installed in the c:\mingw62 directory therefore in order to use the compiler suite you should have c:\mingw64\bin on my windows path. You can test that everything works by opening command line and trying to run “g++ -v” from the command line.

Note, newer versions of gcc may work as well - I have successfully built BIAS using version 4.8.2 on Linux. However the build has only been tested on windows with version 4.7.1.

## Download and install MSYS

MSYS will be used to configure and build openssl. Download MSYS from here <http://www.mingw.org/wiki/msys> Run the .exe to install.

## Download and install ActiveState Perl

Perl is used when building the qt library. I recommend using activestate’s version of perl. For my latest build I used version 5.16.3.1604. Note, I’m not sure if the verision matters - e.g. version 5.18.2.1802 might work fine. In any case activestate’s perl can be downloaded from here http://www.activestate.com/activeperl/downloads

Make sure that perl is added to the windows path. For example, if perl was installed in the c:\perl64 directory then the following two items should be on your windows path c:\perl64\bin and c:\perl64\site\bin.

## Download and install Openssl

Openssl can be downloaded from here here <https://www.openssl.org/source/> Note, some minor edits are required for building on a 64bt windows.

First edit config and add options=”$options no-asm” into it (~15th line from bottom).

Using MSYS run ./config as follows:

```shell
./config --prefix=<openssl install directory> --openssldir=<openssl source directory> no-asm
```

where `<openssl install directory>` and `<openssl source directory>` are something like

```PowerShell
<openssl install directory>  =  /c/Users/Will/work/openssl/openssl_install
<openssl source directory>   =  /c/Users/Will/work/openssl/openssl_files
```

Next edit the generated Makefile and change -march=i486 to -march=x86-64. With openssl-1.0.1p I also needed to edit the files md5test.c, rc5test.c and jpaketest.c in the test directory and change the line

```c
dummytest.c
```

to

```c
#include "dummytest.c"
```

Finally build and install - using MSYS.

```shell
mingw32-make.exe
mingw32-make.exe install
```

## Download, configure and build Qt

BIAS currently uses qt version 5.3.2. The repository can be download using git (from git bash).

```shell
git clone git://code.qt.io/qt/qt5.git
```

Next, using git bash checkout the 5.3.2 branch.

```shell
git checkout 5.3.2
```

Temporarily add git to the windows PATH. Make sure to add it after Perl. Then from within the qt5 directory run the “init-repository” script to fetch the sub-repositories. This should be run from either a window cmd shell or Powershell as running it from git bash will fail.

```shell
perl ./init-repository -no-webkit
```

When finished remove git from windows PATH.

Open a shell (cmd or Powershell) for building Qt. Set the QT_SRC_DIR environment variable to the location of the Qt source files and QTMAKESPEC to the appropriate spec file location. For example setting the environment variables can be done in Powershell as follows.

```PowerShell
$Env:QT_SRC_DIR = "<Path to qt source dir>"
$Env:QMAKESPEC = "$Env:QT_SRC_DIR\qtbase\mkspecs\win32-g++"
```

Next, from within the qt5 source directory configure Qt as follows:

```PowerShell
./configure  -developer-build  -opensource  -platform win32-g++ -c++11  -opengl desktop -openssl -I <path to openssl install>\include -L <apth to openssl install>\lib
```

Run mingw32-make to build and install.

```PowerShell
mingw32-make.exe -j4
mingw32-make.exe intall
```

Next add the Qt bin directory to the windows PATH. For a developer build the bin directory will in the qtbase sub-directory. Finally, Create the QTDIR environment variable and set it to the qtbase sub-directory.

Note, if you’ve made a mistake during configuration or building Qt and need to reconfigure run

```PowerShell
mingw32-make confclean
```

you can then re-run configure.exe with the desired options.

## Download and install CMake

CMake is required for building both OpenCV and BIAS. The latest version of CMake can be found here <http://www.cmake.org/cmake/resources/resources.html>.

- Download the Window (Win32 Installer) cmake-2.8.xx.x-win32-x86.exe and install the program as usual.
- After installing make sure that CMake is added to the windows PATH. You should see something like “C:Program Files(x86)Cmake 2.8bin” on your path. Also you should be able to run cmake from a command window.

## Downlad an unpack Eigen

Eigen is a C++ template library for linear algebra which is used by OpenCV. Eigen isn’t required, but it can speed up some computations. You can download the latest version of eigen from here <http://eigen.tuxfamily.org/index.php?title=Main_Page>

Note, eigen is a pure template library defined in the headers so we don’t need to build anything. Just unpack the latest version of the library to a convenient directory.


## Download Configure, and build OpenCV

### Download

Clone the latest version of OpenCV from GitHub here <https://github.com/Itseez/opencv.git>.

Note, as I haven’t updated the library in a while (about a year) it is possible that there have been sufficient changes to OpenCV such that the latest version is no longer compatible BIAS. In which case in might be necessary to roll back commit which is last know work - which is 0e7ca71dcc1b53430893362faf302c05c8695524. The following command should enable you to rollback the repository from this commit in tempory branch named ‘old-build-temp’.

```shell
git checkout -b old-build-temp 0e7ca71dcc1b53430893362faf302c05c8695524.
```

### Configure and Build

Start by creating a build directory. Note, this can be anywhere and named anything. However, I typically create a directory named ‘build’ in the parent directory of the opencv’s source directory - i.e., one directory up from the root of the source tree.

Configure OpenCV by running

```PowerShell
cmake-gui.exe
```

In the configuration tool perform the following steps

- Select the source and build directories
- Run configure
- Set the build type to MinGW
- Make sure that the following options are selected: WITH_QT, WITH_EIGEN
- Make sure that WITH_IPP is not selected.
- Set EIGEN_INCLUDE_PATH to point to the location of the Eigen library
- Re-run configure
- Run generate

Next, exit the configuration tool, cd to the build directory selected above, and build opencv by running the following command

```PowerShell
mingw32-make
```

After, building the library and the ‘bin’ sub-directory to the windows path, e.g.,

```PowerShell
C:\<PATH-TO-BUILD-DIRECTORY>\bin
```

## Install Point Grey’s FlyCapture2 libraray, a camera, etc.

Links and instructions for downloading and installing the FlyCapture2 library from Point Grey can be found here <http://ww2.ptgrey.com/sdk/flycap>

After installing the library use the FlyCap2 program to verify that the cameras are working.

Finally, add the FlyCapture2 library to the Windows PATH. Note, this may or may not be done automatically. Also, the path will very depending on whether the host system is 32bit or 64bit.

On 64bit Windows

```PowerShell
C:\Program Files\Point Grey Research\FlyCapture2\bin64
```

On 32bit Windows

```PowerShell
C:\Program Files\Point Grey Research\FlyCapture2\bin
```

## Install mercurial

BIAS uses the mercurial revision control system. Which can be downloaded from here <http://mercurial.selenic.com/downloads>

Download and install mercurial. Make sure that it is on the windows PATH e.g.,

```PowerShell
C:\Program Files\Mercurial\
```

and can be run from the command line using the ‘hg’ command.

## Download and build the latest version of BIAS

Download the latest version of BIAS w/ mercurial using the following command

```PowerShell
hg clone https://bitbucket.org/iorodeo/bias
```

Create a build directory. Again this can be named anything and located anywhere. However, I typically create a directory named ‘build’ in the root directory of BIAS’s source tree. This directory is the projects ”.hgignore” file so its contents won’t be tracked by mercurial.

In the build directory run the following command

```PowerShell
cmake -G "MinGW Makefiles"  <path to root of bias source>
```

For example if the build directory is in the root directory of the source tree this command would be

```PowerShell
cmake -G "MinGW Makefiles" ../
```

Next Build BIAS using the following command from inside the build directory

```PowerShell
mingw32-make
```
