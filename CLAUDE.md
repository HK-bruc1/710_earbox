# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a TWS (True Wireless Stereo) earphone firmware project based on JieLi AC710N chip SDK version 3.0.0. The project includes heart rate monitoring functionality migration from color screen warehouse communication system.

## Architecture

- **Target Platform**: JieLi AC710N chip (pi32v2 architecture)
- **Project Type**: TWS earphone firmware with charging case support
- **SDK Version**: AC710N TWS SDK 3.0.0
- **Main Application**: Located in `SDK/apps/earphone/`
- **Heart Rate Integration**: Heart rate sensor support with sport data collection via RCSP protocol

### Key Components

- **Main Application**: `SDK/apps/earphone/app_main.c` - Main application entry point
- **Audio Processing**: `SDK/apps/earphone/audio/` - Audio stream handling and effects
- **Battery Management**: `SDK/apps/earphone/battery/` - Charging and power management
- **Board Configuration**: `SDK/apps/earphone/board/` - Hardware-specific configurations
- **Bluetooth Stack**: Integrated TWS and classic Bluetooth support
- **RCSP Protocol**: `SDK/apps/common/third_party_profile/jieli/rcsp/` - JieLi communication protocol
- **Heart Rate Sensors**: Sport data collection modules in RCSP server functions

## Build System

### Windows Development Environment

The project uses a custom Makefile-based build system with JieLi toolchain:

- **Toolchain Path**: `C:/JL/pi32/bin/`
- **Compiler**: `clang.exe`
- **Linker**: `pi32v2-lto-wrapper.exe`
- **System Libraries**: `C:/JL/pi32/pi32v2-lib/r3`

### Common Build Commands

```bash
# Build the entire project (run from SDK directory)
make

# Clean build artifacts
make clean

# Build with verbose output
make VERBOSE=1

# Download firmware to device
make download

# Format flash and download
make format_flash
```

### VSCode Integration

The project includes VSCode configuration:
- **Build Task**: Ctrl+Shift+P → "Tasks: Run Task" → "all"
- **Clean Task**: Ctrl+Shift+P → "Tasks: Run Task" → "clean"

## Configuration Files

The project uses JSON-based configuration system located in `src/`:

- **板级配置.json**: Hardware board configuration
- **电源配置.json**: Power management settings
- **蓝牙配置.json**: Bluetooth stack configuration
- **按键配置.json**: Key/button mapping configuration
- **功能配置.json**: Feature enable/disable settings
- **音频配置.json**: Audio processing parameters
- **升级配置.json**: OTA update configuration

## Heart Rate Functionality

Heart rate monitoring is integrated through the RCSP protocol system:

- **Continuous Heart Rate**: `sport_info_continuous_heart_rate.h`
- **Exercise Heart Rate**: `sport_info_exercise_heart_rate.h` 
- **Heart Rate Data**: `sport_data_heart_rate.h`

These modules are part of the sensor data collection system that communicates with mobile apps.

## Key Development Guidelines

### Code Organization
- Main application code should be in `SDK/apps/earphone/`
- Hardware-specific code goes in `SDK/apps/earphone/board/br56/`
- Common utilities are in `SDK/apps/common/`
- Audio processing modules are in `SDK/audio/`

### Configuration Management
- Use JSON configuration files in `src/` for user-configurable parameters
- Hardware configurations should be in board-specific files
- Avoid hardcoding values in source code

### Memory and Performance
- This is an embedded system with limited resources
- Use appropriate compiler optimizations (-O2 is default)
- Be mindful of stack usage and memory allocation
- Audio processing requires real-time performance

### TWS Synchronization
- Pay attention to left/right earphone synchronization
- Use TWS-aware audio processing where applicable
- Consider charging case communication protocols

## Testing and Debugging

- Use JieLi development tools for debugging
- Serial output available for logging
- Audio testing requires proper audio flow configuration
- TWS functionality needs paired device testing

## Important Notes

- This project requires JieLi proprietary toolchain and libraries
- Some audio processing uses hardware-accelerated codecs
- Heart rate integration requires proper sensor calibration
- TWS pairing and synchronization is handled by the SDK framework