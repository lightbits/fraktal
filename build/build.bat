@echo off
call C:\Applications\vs2012\VC\vcvarsall.bat
pushd .\build
set VERSION=/DDEBUG
set COMPILE_OPTIONS=/Zi /nologo /MT /EHsc /O2 /Oi /fp:fast %VERSION%
set INCLUDE_PATHS=/I"..\..\..\sdl\include" /I"..\..\..\glew\include"
set LIB_PATHS=/LIBPATH:"..\..\..\glew\lib\x86" /LIBPATH:"..\..\..\sdl\lib\x86"
set DEPENDENCIES="SDL2.lib" "SDL2main.lib" "opengl32.lib" "glew32.lib"
set LINKER_OPTIONS=/link %LIB_PATHS% %DEPENDENCIES% /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE 

del *.pdb > NUL 2> NUL
cl %COMPILE_OPTIONS% %INCLUDE_PATHS% ..\main.cpp %LINKER_OPTIONS%
popd
START .\build\main.exe