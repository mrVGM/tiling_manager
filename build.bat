@echo off

set ROOT=%~dp0%
echo %ROOT%

if exist "%ROOT%ready" (
    rmdir /s /q "%ROOT%ready" || goto :error
)
mkdir "%ROOT%ready" || goto :error

cmake -G "Ninja" -S . -B out -DCMAKE_BUILD_TYPE=RELEASE || goto :error
cmake --build out || goto :error

copy "%ROOT%out\Manager\Manager.exe" "%ROOT%ready" || goto :error
copy "%ROOT%out\Bridge\Bridge.dll" "%ROOT%ready" || goto :error
copy "%ROOT%out\Tiling\Tiling.exe" "%ROOT%ready" || goto :error

echo SUCCESS
exit(0)

:error
echo FAIL
exit(1)

