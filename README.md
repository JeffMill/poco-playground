# Poco apps using VCPKG

WSL Note: WSL maps Windows drives. `~/Downloads` becomes `/mnt/c/Users/jeffmill/Downloads`

## Windows Instructions

### Install Visual Studio Build Tools (includes cmake)

```PowerShell
Invoke-WebRequest -Uri 'https://aka.ms/vs/17/release/vs_BuildTools.exe' -OutFile "$env:TEMP/vs_BuildTools.exe"

& "$env:TEMP/vs_BuildTools.exe" --passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended
```

### Add CMAKE to path

```PowerShell
$env:PATH += ";${Env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
```

### Install vcpkg

```PowerShell
winget install 'Git.git'

cd \

git clone https://github.com/microsoft/vcpkg

.\vcpkg\bootstrap-vcpkg.bat
```

### Build

#### Generate makefiles

```PowerShell
cmake.exe -B build -S . -D CMAKE_TOOLCHAIN_FILE='/vcpkg/scripts/buildsystems/vcpkg.cmake'
```

#### Build App

Note: You can also specify `Release` or `RelWithDebInfo` instead of `Debug`.

```PowerShell
cmake.exe --build build --parallel --config Debug
```

### Run

```PowerShell
./build/Debug/sha256sum.exe
```

### Debug

```PowerShell
winget install 'WinDbg Preview'

windbgx.exe -y '\vcpkg\packages\poco_x64-windows\debug\bin' -c 'bp sha256sum!Application::main; g' -xe eh .\build\Debug\sha256sum.exe *
```

## Linux Instructions

### Install build-essential cmake

```bash
sudo apt update --yes && sudo apt upgrade --yes

sudo apt-get install build-essential cmake --yes
```

### Install vcpkg

```bash
sudo apt-get install curl zip unzip tar pkg-config --yes

git clone https://github.com/Microsoft/vcpkg.git ~/vcpkg

~/vcpkg/bootstrap-vcpkg.sh
```

### Build

#### Generate makefiles

```bash
cmake -B build -S . -D CMAKE_TOOLCHAIN_FILE='~/vcpkg/scripts/buildsystems/vcpkg.cmake'
```

#### Build App

Note: You can also specify `Release` or `RelWithDebInfo` instead of `Debug`.

```bash
cmake --build build --parallel --config Debug
```

If building with Release, use `strip build/sha256sum` afterwards to strip binary of symbols. Unclear why this is needed for Release build.

### Run

```bash
# For sha256sum test app
mkdir ~/.local/tmp

chmod u+x build/sha256sum

build/sha256sum
```

### Debug

Use gdb. `catch throw` might catch C++ exceptions at source.

## References

### POCO docs

[POCO Reference](https://docs.pocoproject.org/current)

[POCO Presentation](https://pocoproject.org/slides/)

[namespace Net](https://docs.pocoproject.org/current/Poco.Net.html)

### POCO samples

POCO includes samples as well.  Enlist in the POCO repo:

```PowerShell
git clone https://github.com/pocoproject/poco.git`
```

then search for the projects:

```PowerShell
Get-ChildItem -Recurse -Directory | Select-Object FullName | Where-Object FullName -like '*\samples\*\src'
```

### VCPKG Manifest Mode

[Manifest Mode](https://learn.microsoft.com/en-us/vcpkg/users/manifests)

[vcpkg.json Reference](https://learn.microsoft.com/en-us/vcpkg/reference/vcpkg-json)

[Manifest mode: CMake example](https://learn.microsoft.com/en-us/vcpkg/examples/manifest-mode-cmake)

