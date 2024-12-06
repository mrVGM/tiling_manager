@echo off

set ROOT=%~dp0%

if exist "%ROOT%ready" (
    rmdir /s /q "%ROOT%ready" || goto :error
)
mkdir "%ROOT%ready" || goto :error

cmake -G "Ninja" -S . -B out -DCMAKE_BUILD_TYPE=RELEASE || goto :error
cmake --build out || goto :error

echo SUCCESS
exit(0)

:error
echo FAIL
exit(1)

