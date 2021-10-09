@echo off
pushd "%~dp0"
    rem lack of space before > and >> is important because echo will copy the space if it is there
    echo builddir = build/win> build.ninja
    echo executable = bob.exe>> build.ninja
    type template.ninja>> build.ninja
    set NINJA_STATUS=[%%f/%%t] %%es
    "../ninja.exe"
    if %errorlevel% neq 0 popd && exit /b %errorlevel%
    :reallydeleteforrealplease
    del build.ninja >nul 2>&1
    if exist build.ninja goto :reallydeleteforrealplease
popd
