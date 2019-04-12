@echo off

REM Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.

set INCLUDES=/Isrc\reuse /Isrc\reuse\glfw\include /Isrc\reuse\gl3w /Isrc\reuse\imgui
set CFLAGS=/nologo /Zi /MT /W3 /WX -D_CRT_SECURE_NO_WARNINGS
set LIBS=/LIBPATH:src\reuse\glfw\lib-vc2010-32 glfw3.lib opengl32.lib gdi32.lib shell32.lib user32.lib

REM Note: We disable the console by compiling as a Win32 application
set LFLAGS=/MACHINE:X86 /subsystem:WINDOWS /ENTRY:mainCRTStartup res\resources.res

REM This packs the application icon into a resource.res file, which gets linked in to the final exe
rc -nologo res\resources.rc

cl src\fraktal.cpp %CFLAGS% %INCLUDES% /link %LIBS% %LFLAGS%

if not errorlevel 1 fraktal.exe
