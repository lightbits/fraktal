@echo off
REM Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.

if not exist "lib" mkdir lib
pushd lib

del *.obj
del *.lib
del *.exp
del *.pdb

set INCLUDES=/I..\src\reuse /I..\src\reuse\glfw\include /I..\src\reuse\gl3w
set LIBS=/LIBPATH:..\src\reuse\glfw\lib-vc2010-32
cl ..\src\fraktal.cpp /nologo /DFRAKTAL_BUILD_DLL /MD /W3 /WX /LD %INCLUDES% /link %LIBS% glfw3.lib gdi32.lib shell32.lib user32.lib opengl32.lib /out:fraktal.dll

popd
