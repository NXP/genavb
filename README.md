GenAVB/TSN
----------
GenAVB/TSN is a generic AVB/TSN stack developed by NXP for NXP MCUs and MPUs.
It is cross-platform, currently supporting Linux, FreeRTOS and Zephyr.


Supported hardware targets and configurations
---------------------------------------------
This project supports several hardware targets and several different
configuration modes. The table below summarizes the supported combinations,
and their status for this specific release:
- **O**: supported by the project and validated for this release
- x: supported by the project, but not validated for this release
- empty cell: unsupported combination

| Compilation target     | Hardware target                                | AVB endpoint | TSN endpoint | AVB/TSN bridge | AVB hybrid |
| :--------------------: | :--------------------------------------------: | :----------: | :----------: | :------------: | :--------: |
| `linux i.MX6`          | `i.MX 6Q`, `i.MX 6QP`, `i.MX 6D`, `i.MX 6SX`   |      x       |              |                |            |
| `linux i.MX6ULL`       | `i.MX 6ULL`                                    |    **O**     |              |                |            |
| `linux i.MX8`          | `i.MX 8MM`                                     |    **O**     |              |                |            |
| `linux i.MX8`          | `i.MX 8MP`, `i.MX 8DXL`, `i.MX 93`             |    **O**     |    **O**     |                |            |
| `linux i.MX8`          | `i.MX 95`                                      |              |    **O**     |                |            |
| `linux i.MX8`          | `i.MX 8DXL + SJA1105Q`                         |              |              |      **O**     |            |
| `linux i.MX8`          | `i.MX 93 + SJA1105Q`                           |              |              |      **O**     |   **O**    |
| `linux LS1028`         | `LS1028A`                                      |              |              |      **O**     |            |
| `freertos RT1052`      | `i.MX RT1052`                                  |      x       |      x       |                |            |
| `freertos RT1176`      | `i.MX RT1176`                                  |      x       |      x       |                |            |
| `freertos RT1187 M33`  | `i.MX RT1187`                                  |              |              |        x       |            |
| `freertos RT1187 M7 `  | `i.MX RT1187`                                  |              |      x       |                |            |
| `freertos RT1189 M33`  | `i.MX RT1189`                                  |              |              |        x       |            |
| `freertos RT1189 M7 `  | `i.MX RT1189`                                  |              |      x       |                |            |
| `freertos i.MX8MM A53` | `i.MX 8MM`                                     |    **O**     |    **O**     |                |            |
| `freertos i.MX8MN A53` | `i.MX 8MN`                                     |    **O**     |    **O**     |                |            |
| `freertos i.MX8MP A53` | `i.MX 8MP`                                     |    **O**     |    **O**     |                |            |
| `freertos i.MX93  A55` | `i.MX 93`                                      |    **O**     |    **O**     |                |            |
| `zephyr i.MX8MM A53`   | `i.MX 8MM`                                     |    **O**     |    **O**     |                |            |
| `zephyr i.MX8MN A53`   | `i.MX 8MN`                                     |    **O**     |    **O**     |                |            |
| `zephyr i.MX8MP A53`   | `i.MX 8MP`                                     |    **O**     |    **O**     |                |            |
| `zephyr i.MX93  A55`   | `i.MX 93`                                      |    **O**     |    **O**     |                |            |

Features
--------
- IEEE-802.1AS-2020 implementation, both time-aware Bridge and Endpoint support.
- IEEE-802.1Q-2022 implementation, both Bridge and Endpoint Support
    - VLAN/FDB
    - Stream Reservation Protocol (Qat-2010)
    - Scheduled Traffic (Qbv-2015)
    - Frame preemption (Qbu-2016)
    - Per Stream Filtering and Policing (Qci-2017)
    - Forwarding and Queuing for Time-Sensitive Streams (Qav-2009)
- IEEE 802.1CB-2017 implementation for Frame Replication and Elimination for Reliability.
- IEEE 802.3br-2016 implementation for Interspersing Express Traffic.
- IEEE-1722-2016 implementation, with MAAP support.
- IEEE-1722.1-2013 implementation (with initial support for IEEE-1722.1-2021)
- Milan v1.2 implementation.
- IEC 62439-3:2022
    - High-availability Seamless Redundancy (HSR)
- Protocol stacks running in standalone userspace processes for Linux, and dedicated threads for FreeRTOS.
- Public C API provided by library plus header files.
- Example applications.


Repository structure
--------------------
- api:      the public API
- apps:     source code and makefiles for example applications
- avdecc:   IEEE 1722.1-2013/Milan v1.2 component stack
- avtp:     IEEE 1722-2016 component stack
- common:   common code
- configs:  configuration files
- doc:      documentation
- rtos:     RTOS specific code
- gptp:     IEEE 802.1AS-2020 component stack
- hsr:      IEC 62439-3:2022 HSR component stack
- linux:    Linux specific code
- maap:     MAAP component code
- public:   common code shared between applications and stack
- srp:      IEEE 802.1Qat-2010 component stack


Build
-----
GenAVB/TSN is using CMake to generate build system.

Some preliminary configuration is required to provide path to the toolchain,
staging directory and external components.

Local configuration files can be included by the build system in order to
to define some variables specific to the developer environment.
The local config file name is `./local_config_${target}.cmake`


Build requirements
------------------
- Linux host system and development tools (git, make, doxygen for the docs, ...)
- An ARM gcc toolchain is sufficient for the stack. However building the linux
applications requires a more complete cross-compilation SDK (because of
additional dependencies such as alsa and gstreamer).
- AVB endpoint builds depend on custom changes to the Linux kernel and specific Yocto distribution: https://www.nxp.com/design/software/development-software/real-time-edge-software:REALTIME-EDGE-SOFTWARE
- Both FreeRTOS and Zephyr builds require the usage of an [RTOS abstraction layer](https://github.com/NXP/rtos-abstraction-layer)
- FreeRTOS builds require additional MCUXpresso SDK software: https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools-/wired-communications-middleware-for-nxp-microcontrollers:WIRED-COMM-MIDDLEWARE?fpsp=1&#avb-tsn
- Zephyr builds require additional NXP software. Refer to [Harpoon Software](https://github.com/NXP/harpoon-apps) for reference.

### FreeRTOS
Currently GenAVB/TSN stack support only ARM gcc toolchain for FreeRTOS targets.
To be able to build the stack, ARMGCC_DIR environment variable pointing
to arm-gcc toolchain must be defined.
```
export ARMGCC_DIR=/path/to/gcc-arm-none-eabi-xxx
```

The CMake build system must also set:
```
set(MCUX_SDK "/path/to/freertos_MCUXpresso_SDK" CACHE PATH "Path to MCUXpresso SDK")
set(RTOS_APPS "/path/to/freertos_application" CACHE PATH "Path to FreeRTOS application repository")
set(RTOS_ABSTRACTION_LAYER_DIR "/path/to/rtos-abstraction-layer/freertos" CACHE PATH "Path to RTOS Abstraction Layer repository, freertos directory")
```

This can be done using the local config file or the parent CMake project.

### Zephyr
The GenAVB/TSN stack can be built as a CMake subdirectory of a top level Zephyr application
Refer to [Harpoon Software](https://github.com/NXP/harpoon-apps) for an example application.

### Linux
The Linux target is usually built using a complete toolchain, which helps
setting most of the environment variables required for cross-compilation.
In the case of a Yocto SDK/toolchain for example, the following command will
setup $CROSS_COMPILE as well as other related environment variables:

```
source /path/to/yocto/toolchain/environment-setup-xxx
```

The local config should however define:
```
set(KERNELDIR "/path/to/linux_avb" CACHE PATH "Path to Linux kernel")
```


Build commands
--------------
Syntax is:
```
cmake . -B<build directory> -DTARGET=<target name> -DCONFIG=<config name>
make -C <build directory> install
```

The GenAVB/TSN stack also provides a couple shell functions with auto-completion
to facilitate the build process.
Usage is
```
# Setup environment, see Build paragraph above
source environment-genavb
make_genavb [target] [config_list]
clean_genavb [target]
```
where target and config_list are optional. If no config_list is defined, all available
configurations for specified target are built. If no target is specified the default
is linux_imx6.
config_list is of the form: configA configB ..., with one or more members.

To generate doxygen HTML documentation:
```
cmake . -B<build directory> -DBUILD_DOC=ON -DTARGET=<target name> -DCONFIG=<config name>
make -C <build directory> doc_doxygen
```

The generated documentation is available under `<build_directory>/doc`
To install documentation under a custom path: use CMake variable `-DDOC_OUTPUT_DIR=<custom absolute path>`

### Generated binaries
The generated binaries are installed under `<build directory>/`

### Installing binaries to target

#### Linux
Copy the content of `<build directory>/target/` to the root directory of the target filesystem

#### FreeRTOS
Refer to the FreeRTOS application README.

#### Zephyr
Refer to the Harpoon application README.
