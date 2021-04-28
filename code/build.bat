@echo off

mkdir ..\build
pushd ..\build
cl -Zi -FC ..\code\win32_tom.c Kernel32.lib User32.lib Gdi32.lib
popd
