Circle
======

> Raspberry Pi is a trademark of the Raspberry Pi Foundation.

> If you read this file in an editor you should switch line wrapping on.

Overview
--------

Circle is a C++ bare metal programming environment for the Raspberry Pi. It should be useable on all existing models (tested on model A+, B, B+, on Raspberry Pi 2 and 3 and on Raspberry Pi Zero). It provides several ready-tested C++ classes which can be used to control different hardware features of the Raspberry Pi. Together with Circle there are delivered some samples which demonstrate the use of its classes.

Please note that the included USB library was developed in a hobby project. There are known issues with it (e.g. no dynamic attachments, no error recovery, limited split support). For me it works well but that need not be the case with any device and in any situation.

Circle includes bigger (optional) third-party C-libraries for specific purposes in addon/ now. This is the reason why GitHub rates the project as a C-language-project. The main Circle libraries are written in C++ using classes instead. That's why it is named a C++ programming environment.

A Real-Time OS?
---------------

Circle is not a real-time OS. That means different simultaneous operations may interfere in respect of its timing behaviour. The provided samples are tested to work but if you try different combinations of hardware support classes this has to be tested by yourself.

Nevertheless real-time applications based on Circle are possible. Have a look at *doc/realtime.txt* for more information!

The 32nd Step
-------------

In this step a console class has been added to Circle, which allows the easy use of an USB keyboard and a screen together as one device. The console class provides a line editor mode and a raw mode and is demonstrated in *sample/32-i2cshell*, a command line tool for interactive communication with I2C devices. If you are often experimenting with I2C devices, this may be a tool for you. See the *README* file in this directory for details.

Furthermore the Circle USB HCI driver provides an improved compatibility for low-/full-speed USB devices (e.g. keyboards, some did not work properly). Because this update changes the overall system timing, it is not enabled by default to be compatible with existing applications. You should enable the system option *USE_USB_SOF_INTR*, if the improved compatibility is important for your application.

The system options can be found in the file *include/circle/sysconfig.h*. This file has been completely revised and each option is documented now.

Finally there are some improvements in the SPI0 master drivers (polling and DMA) included in this step.

The options to be used for *cmdline.txt* are described in *doc/cmdline.txt*.

In Step 1-31 the following features were introduced:

* C++ build environment
* Simple delay functionality
* Get properties (model, memory size) from VideoCore
* new and delete
* Using GPIO pins
* Manipulating Act LED
* Set pixel on screen
* Use kernel options
* Switch on MMU
* Formatting strings
* Using devices
* Writing characters to screen
* Writing characters to UART
* Logging output to screen or UART
* Using assertions and debug hexdump
* Using interrupts
* Timer class with clock, timers and calibrated delay loop
* Exception handler
* USB host controller interface (HCI) driver
* USB device class (basic device initialization)
* USB hub driver (dectects and enables the supported devices)
* Driver for the on-board Ethernet device (receiving and transmitting frames)
* Driver for USB mass storage devices (bulk only, read and write)
* Detects low- and full-speed devices
* Driver for USB keyboards
* Using GPIO interrupts
* Driver for USB mice
* Using GPIO clock
* Simple 12 MHz GPIO sampling routine
* PWM sound device
* DMA controller support
* PWM output (2 channels)
* Simple USB printer support
* FAT file system support (reduced)
* I2C (master and slave) support
* Multi-core support on Raspberry Pi 2
* Experimental (UDP only) TCP/IP network stack
* Setting system time from an Internet time (NTP) server
* Cooperative non-preemtive scheduler
* TCP support
* DHCP support
* Simple HTTP webserver class
* GDB debug support on Raspberry Pi 2 using rpi_stub
* Bluetooth device inquiry support
* SPI0 master support
* Driver for hardware random number generator
* SPI0 master support using DMA
* CPU clock rate management support
* Basic support for standard USB HID class gamepads
* TFTP file server support
* Basic support for the internal Bluetooth host controller of the Raspberry Pi 3
* Official Raspberry Pi touch screen support
* Supporting GUI creation using uGUI (by Achim Doebler)
* USB Audio Class MIDI input support
* FIQ (fast interrupt) support
* QEMU support
* HTTP client support
* I2S support (output only)

Building
--------

Building is normally done on PC Linux. If building for the Raspberry Pi 1 you need a [toolchain](http://elinux.org/Rpi_Software#ARM) for the ARM1176JZF core (with EABI support). For Raspberry Pi 2/3 you need a toolchain with Cortex-A7/-A53 support. A toolchain, which works for all of these, can be downloaded [here](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads). Circle has been tested with the version *6-2017-q1-update* from this website.

First edit the file *Rules.mk* and set the Raspberry Pi version (*RASPPI*, 1, 2 or 3) and the *PREFIX* of your toolchain commands. Alternatively you can create a *Config.mk* file (which is ignored by git) and set the Raspberry Pi version and the *PREFIX* variable to the prefix of your compiler like this (don't forget the dash at the end):

`RASPPI = 1`  
`PREFIX = arm-none-eabi-`

The following table gives support for selecting the right *RASPPI* value:

| RASPPI | Target         | Models                   | Optimized for |
| ------ | -------------- | ------------------------ | ------------- |
|      1 | kernel.img     | A, B, A+, B+, Zero, (CM) | ARM1176JZF-S  |
|      2 | kernel7.img    | 2, 3, (CM3)              | ARMv7-A       |
|      3 | kernel8-32.img | 3, (CM3)                 | Cortex-A53    |

For a binary distribution you should do one build with *RASPPI = 1* and one with *RASPPI = 2* and include the created files *kernel.img* and *kernel7.img*. Optionally you can do a build with *RASPPI = 3* and add the created file *kernel8-32.img* to provide an optimized version for the Raspberry Pi 3.

Then go to the build root of Circle and do:

`./makeall clean`  
`./makeall`

By default only the latest sample (with the highest number) is build. The ready build *kernel.img* file should be in its subdirectory of sample/. If you want to build another sample after `makeall` go to its subdirectory and do `make`.

You can also build Circle on the Raspberry Pi itself on Raspbian but you need some method to put the *kernel.img* file onto the SD(HC) card. With an external USB card reader on model B+ or Raspberry Pi 2/3 model B (4 USB ports) this should be no problem.

Building Circle from a non-Linux host is possible too. Maybe you have to adapt the shell scripts in this case. You need a cross compiler targetting (for example) *arm-none-eabi*. OSDev.org has an [excellent document on the subject](http://wiki.osdev.org/GCC_Cross-Compiler) that you can follow if you have no idea of what a cross compiler is, or how to make one.

Installation
------------

Copy the Raspberry Pi firmware (from boot/ directory, do *make* there to get them) files along with the kernel.img (from sample/ subdirectory) to a SD(HC) card with FAT file system. Put the SD(HC) card into the Raspberry Pi.

Directories
-----------

* include: The common header files, most class headers are in the include/circle/ subdirectory.
* lib: The Circle class implementation and support files (other libraries are in subdirectories of lib/).
* sample: Several sample applications using Circle in different subdirectories. The main function is implemented in the CKernel class.
* addon: Contains contributed libraries and samples (has to be build manually).
* app: Place your own applications here. If you have own libraries put them into app/lib/.
* boot: Do *make* in this directory to get the Raspberry Pi firmware files required to boot.
* doc: Additional documentation files.

Classes
-------

The following C++ classes were added to Circle:

USB library

* CDWHCITransactionQueue: Queues coming USB transactions (with USE_USB_SOF_INTR enabled)

Input library

* CConsole: Console device using screen/USB keyboard or alternate device (e.g. CSerialDevice)
* CKeyboardBuffer: Buffers characters entered on the USB keyboard
* CLineDiscipline: Implements line editor function

The available Circle classes are listed in the file *doc/classes.txt*. If you have doxygen installed on your computer you can build a class documentation in *doc/html/* using:

`./makedoc`

At the moment there are only a few classes described in detail for doxygen.
