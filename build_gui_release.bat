@echo off
REM Run your copy of 64-bit vcvarsall.bat to setup command-line compiler.

pushd bin

REM This packs the application icon into a resource.res file, which gets linked in to the final exe
rc -nologo ..\res\resources.rc

set INCLUDES=/I..\src\reuse /I..\src\reuse\glfw\include /I..\src\reuse\gl3w /I..\src\reuse\imgui
set CFLAGS=/nologo /MD /W3 /WX /O2 /D_CRT_SECURE_NO_WARNINGS
set LIBS=/LIBPATH:..\src\reuse\glfw\lib-vc2010-64 glfw3.lib opengl32.lib gdi32.lib shell32.lib user32.lib
set LFLAGS=/MACHINE:X64 /subsystem:CONSOLE ..\res\resources.res
cl ..\src\gui_glfw.cpp %CFLAGS% %INCLUDES% /link %LIBS% %LFLAGS% /out:fraktal.exe

popd

if not errorlevel 1 bin\fraktal.exe
