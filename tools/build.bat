@echo off

cmake -DCMAKE_GENERATOR_PLATFORM=x64 -DCMAKE_TOOLCHAIN_FILE=%UserProfile%\github.com\microsoft\vcpkg\scripts\buildsystems\vcpkg.cmake ..
