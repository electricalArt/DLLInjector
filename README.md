# DLLInjector

DLLInjector is an Windows application that searches for specified process and injects specified DLL file. The utility creates new thread within target process starting at DLL entry point.

## Install

To receive ready-to-use binaries, take a look to [releases](https://github.com/elektrikArt/DLLInjector/releases/) page.

To build the binaries by yourself, you should have `easylogging++` library installed and linked. Use Visual Studio standard build routine. 

## Usage

```
DLLInjector.exe <ProcessName> <DLLPath> 
```

## Testing

There is `DummyLibraryDll` project within solution. Build it to receive a very simple DLL file. After that run:
```PowerShell
.\Test.p1
```
If everything is fine, `Notepad` should be opened and MessageBox should appear with message about success.

## Contributing

PRs are accepted.
