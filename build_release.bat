@echo off
REM Run your copy of vcvars64.bat or vcvarsall.bat to setup command-line compiler.

if not exist "bin" mkdir bin
pushd bin
set INCLUDES=/I..\src\reuse /I..\src\reuse\glfw\include /I..\src\reuse\gl3w /I..\src\reuse\imgui
set CFLAGS=/nologo /Zi /MT /W3 /WX -D_CRT_SECURE_NO_WARNINGS
set LIBS=/LIBPATH:..\src\reuse\glfw\lib-vc2010-64 /NODEFAULTLIB:LIBCMT.lib glfw3.lib opengl32.lib gdi32.lib shell32.lib user32.lib

REM Note: We disable the console by compiling as a Win32 application
set LFLAGS=/MACHINE:X64 /subsystem:WINDOWS /ENTRY:mainCRTStartup ..\res\resources.res

REM This packs the application icon into a resource.res file, which gets linked in to the final exe
rc -nologo ..\res\resources.rc

cl ..\src\app_glfw.cpp %CFLAGS% %INCLUDES% /link %LIBS% %LFLAGS% /out:fraktal.exe
popd

if not errorlevel 1 bin\fraktal.exe
