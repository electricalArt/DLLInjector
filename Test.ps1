$TargetProcess = Start-Process notepad.exe -PassThru
Sleep 1

.\x64\Release\DllInjector.exe Notepad.exe .\x64\Release\DummyLibraryDll.dll
