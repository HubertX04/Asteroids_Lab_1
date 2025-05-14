@echo off

where /q cl || (
  echo ERROR: could not find "cl" - run the "build.bat" from the MSVC x64 native tools command prompt.
  exit /b 1
)

set warnings=/WX /W4 /wd4201 /wd4100 /wd4189 /wd4505 /wd4101 /wd4324 /wd4244
set includes=/I ../my_lib/ /I ../external/raylib/
set linkerFlags=/OUT:Main.exe /INCREMENTAL /CGTHREADS:6 /STACK:0x100000,0x100000 
set linkerLibs=winmm.lib user32.lib shell32.lib gdi32.lib opengl32.lib
set compilerFlags=/std:c++20 /MP /arch:AVX2 /Oi /Ob3 /EHsc /fp:fast /fp:except- /nologo /GS- /Gs999999 /GR- /FC /Z7 

if "%~1"=="-Debug" (
	echo [[ debug build ]]
	set compilerFlags=%compilerFlags% /Od /MTd /D_DEBUG
	set rayname=d_raylib
)
if "%~1"=="" (
	echo [[ debug build ]]
	set compilerFlags=%compilerFlags% /Od /MTd /D_DEBUG
	set rayname=d_raylib
)
if "%~1"=="-Release" (
	echo [[ release build ]]
	set compilerFlags=%compilerFlags% /O2 /MT 
	set rayname=raylib
)

IF NOT EXIST .\build mkdir .\build
pushd .\build
del *.pdb > NUL 2> NUL

IF NOT EXIST %rayname%.lib (
echo building raylib
REM Had to go to platforms directory and change path for GLFW include headers
cl.exe /w /c /D PLATFORM_DESKTOP /D GRAPHICS_API_OPENGL_33 %compilerFlags% ../external/raylib/*.c
lib /OUT:%rayname%.lib rcore.obj raudio.obj rglfw.obj rmodels.obj rshapes.obj rtext.obj rtextures.obj utils.obj
del /Q *.obj
)

cl.exe %compilerFlags% %warnings% %includes% ../source/Main.cpp /link %linkerFlags% %rayname%.lib %linkerLibs%
popd