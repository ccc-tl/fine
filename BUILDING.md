## Building on Windows

- Install MSYS2 MinGW 64-bit toolchain including `cmake`
- Pull submodules
```
git submodule update --init --recursive
```
- Prepare project build system
```
cmake -G "MSYS Makefiles" .
```
- Build executable
```
make
```

### Visual Studio Code
Visual Studio Code is the recommended code editor for this project.

Recommended `.vscode/settings.json`

```json
{
    "terminal.integrated.shell.windows": "C:\\msys64\\usr\\bin\\bash.exe",
    "terminal.integrated.shellArgs.windows": [
        "--login",
    ],
    "terminal.integrated.env.windows": {
        "CHERE_INVOKING": "1",
        "MSYSTEM": "MINGW64",
    },
    "files.trimTrailingWhitespace": true,
    "files.insertFinalNewline": true,
    "files.trimFinalNewlines": true,
}
```

Recommended `.vscode/c_cpp_properties.json`

```json
{
    "configurations": [
        {
            "name": "MINGW64",
            "includePath": [
                "${workspaceFolder}/**"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE",
                "PICOJSON_USE_INT64"
            ],
            "compilerPath": "C:/msys64/mingw64/bin/gcc.exe",
            "cStandard": "c11",
            "cppStandard": "c++17",
            "intelliSenseMode": "gcc-x64"
        }
    ],
    "version": 4
}
```
