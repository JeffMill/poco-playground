# Poco apps using VCPKG

## Install BUILD TOOLS

```PowerShell
Invoke-WebRequest -Uri 'https://aka.ms/vs/17/release/vs_BuildTools.exe' -OutFile "$env:TEMP/vs_BuildTools.exe"

& "$env:TEMP/vs_BuildTools.exe" --passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended
```

## Install VCPKG

```PowerShell
winget install 'Git.git'

cd \

git clone https://github.com/microsoft/vcpkg

.\vcpkg\bootstrap-vcpkg.bat
```

## Build

### Add CMAKE to path

```PowerShell
$env:PATH += ";${Env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
```

### Generate makefiles

```PowerShell
cmake.exe -B build -S . -D CMAKE_TOOLCHAIN_FILE='/vcpkg/scripts/buildsystems/vcpkg.cmake'
```

#### Build App

```PowerShell
cmake.exe --build build --parallel --config Debug
```

## Run/Debug

```PowerShell
winget install 'WinDbg Preview'

.\build\Debug\poco-test.exe

windbgx.exe -y '\vcpkg\packages\poco_x64-windows\debug\bin' -c 'bp sha256sum!Application::main; g' -xe eh .\build\Debug\sha256sum.exe *
```

## REFERENCES

### POCO docs

[POCO Reference](https://docs.pocoproject.org/current)

[POCO Presentation](https://pocoproject.org/slides/)

[namespace Net](https://docs.pocoproject.org/current/Poco.Net.html)

### VCPKG Manifest Mode

[Manifest Mode](https://learn.microsoft.com/en-us/vcpkg/users/manifests)

[vcpkg.json Reference](https://learn.microsoft.com/en-us/vcpkg/reference/vcpkg-json)

[Manifest mode: CMake example](https://learn.microsoft.com/en-us/vcpkg/examples/manifest-mode-cmake)
