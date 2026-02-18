# AICopilot

AICopilot is an Unreal Engine editor plugin that adds tooling and UI to assist workflows, including settings management, commands, and custom Slate windows.

## Features
- Editor commands and toolbar/menu integration
- Custom Slate windows for interaction
- Configurable plugin settings

## Requirements
- Unreal Engine 5.x
- Windows (tested with Win64 editor builds)
- Visual Studio toolchain (matching your UE version)

## Installation
1. Copy the plugin folder to your project:  
   `YourProject/Plugins/AICopilot`
2. Open the project in Unreal Editor.
3. Enable **AICopilot** in **Edit → Plugins**.
4. Restart the editor if prompted.

## Usage
- Open the plugin window from the editor menu/toolbar (as provided by the plugin).
- Adjust settings in **Project Settings** (if the plugin exposes a settings panel).

## Build
- Build the project in Visual Studio or from the Unreal Editor.
- The plugin will compile as part of the project build.
