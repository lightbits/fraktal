@echo off
REM Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.

pushd bin

set INCLUDES=/I..\src\reuse /I..\src\reuse\glfw\include /I..\src\reuse\gl3w /I..\src\reuse\imgui
set CFLAGS=/nologo /Zi /MD /W3 /WX /D_CRT_SECURE_NO_WARNINGS
set LIBS=/LIBPATH:..\src\reuse\glfw\lib-vc2010-32 glfw3.lib opengl32.lib gdi32.lib shell32.lib user32.lib
cl ..\src\gui_glfw.cpp %CFLAGS% %INCLUDES% /link %LIBS% /out:fraktal.exe

popd

if not errorlevel 1 bin\fraktal.exe
