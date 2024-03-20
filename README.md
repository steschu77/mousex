# Mouse and Keyboard Extensions for Windows

## Summary

This lightweight utility enhances Windows experience by providing the following features:

*Measurement*: Track mouse movement distance, mouse wheel rotations, and keyboard keystrokes effortlessly.

*Coordinate Tracking*: Retrieve current mouse coordinates globally or relative to the active window.

*Anti-contact bounce*: Prevent accidental double clicks or multiple keypresses caused by faulty hardware.

*Window Control*: Seamlessly navigate windows under the mouse cursor, particularly useful for older Windows versions.

*Convenience* Simulate left-double-clicks using the middle mouse button or emulate Page Up or Page Down actions by pressing "ESC+Up/Down", like commonly found on notebooks with the "Fn + Up/Down" shortcut.

*Keep it Small*: Small binary with minimal memory consumption written with 500 lines of C++ code.

## How to build

Using CMake for Windows and Microsoft Visual Studio 2022 run the following steps:

Run the following commands in the repository:
```
mkdir _vs2022
cd _vs2022
cmake ..

```

In Microsoft Visual Studio 2022 open the solution `_vs2022/mousex.sln`
