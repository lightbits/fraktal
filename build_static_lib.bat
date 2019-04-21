@echo off
REM Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
REM When linking against fraktal.lib in your application you also need to link against glfw3.lib gdi32.lib shell32.lib user32.lib opengl32.lib

if not exist "lib" mkdir lib
pushd lib

del *.obj
del *.lib
del *.exp
del *.pdb

set INCLUDES=/I..\src\reuse /I..\src\reuse\glfw\include /I..\src\reuse\gl3w
cl ..\src\fraktal.cpp /c /nologo /MD /W3 /WX %INCLUDES%
lib fraktal.obj /nologo /out:fraktal.lib

popd
