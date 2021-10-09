@echo off

rem NOTE: Ninja's return value has been proven to be unreliable on Windows
rem       (there is a heisenbug where it sometimes returns EXIT_SUCCESS on a failed build)
rem       so now we are preventing spurious runs of previous versions of the program
rem       by deleting the executable beforehand so that there's nothing to run if the build fails.
rem       This is suboptimal, because it means the game will be re-linked every time
rem       even if nothing changed, but it's the best we're going to get until the bug gets fixed

pushd "%~dp0"
    call bob\build.bat
    bob\bob.exe -o build.ninja -c %1
    del game.exe
    set NINJA_STATUS=[%%f/%%t] %%es
    ninja.exe
    del build.ninja
    game.exe
popd

rem call bob\build.bat
rem rem echo bob error %errorlevel%
rem if %errorlevel% == 0 (
rem     bob\bob -t base.ninja -o build.ninja
rem     set NINJA_STATUS=[%%f/%%t] %%es
rem     ninja
rem     rem echo ninja error %errorlevel%
rem     if %errorlevel% == 0 (
rem         del build.ninja
rem         game.exe
rem     )
rem )
