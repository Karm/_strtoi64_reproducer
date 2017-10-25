# Investigates a CMake/cl/link strtoi64 quirk with APR tests

I encountered a weird test failure with ```_strtoi64``` with Microsoft Windows Server 2012 and Windows 10 Enterprise v1709, Visual Studio 2017, x86_64. To be more specific, it happens both with ```C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin/amd64/cl.exe``` and ```C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.11.25503/bin/Hostx64/x64/cl.exe``` What follows is a minimized example that passes the test, unfortunately not reproducing the issue. Next, there is the issue depicted on an APR test suite.

* CL used: ```C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.11.25503/bin/Hostx64/x64/cl.exe```
* ucrt headers from: ```C:\Program Files (x86)\windows kits\10\include\10.0.16299.0\ucrt\```

## Compile and link a small excerpt manually
```
C:\Users\karm\source\repos\_strtoi64_reproducer
λ vcvars64
**********************************************************************
** Visual Studio 2017 Developer Command Prompt v15.0.26730.16
** Copyright (c) 2017 Microsoft Corporation
**********************************************************************
[vcvarsall.bat] Environment initialized for: 'x64'

C:\Users\karm\source\repos\_strtoi64_reproducer
λ cl /Wall /O2 /Zi /c main.c
Microsoft (R) C/C++ Optimizing Compiler Version 19.11.25508.2 for x64
Copyright (C) Microsoft Corporation.  All rights reserved.

main.c
main.c(43): warning C4710: 'int printf(const char *const ,...)': function not inlined
C:\Program Files (x86)\Windows Kits\10\include\10.0.15063.0\ucrt\stdio.h(946): note: see declaration of 'printf'
c:\program files (x86)\windows kits\10\include\10.0.15063.0\ucrt\stdio.h(641) : warning C4710: '__local_stdio_printf_options': function not inlined
c:\program files (x86)\windows kits\10\include\10.0.15063.0\ucrt\stdio.h(641) : warning C4710: '__local_stdio_printf_options': function not inlined
c:\users\karm\source\repos\_strtoi64_reproducer\main.c(40) : warning C4710: 'printf': function not inlined
c:\users\karm\source\repos\_strtoi64_reproducer\main.c(38) : warning C4711: function 'apr_strtoi64' selected for automatic inline expansion

C:\Users\karm\source\repos\_strtoi64_reproducer
λ link kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib main.obj /INCREMENTAL:NO /machine:x64 /OUT:main.exe
Microsoft (R) Incremental Linker Version 14.11.25508.2
Copyright (C) Microsoft Corporation.  All rights reserved.

C:\Users\karm\source\repos\_strtoi64_reproducer
λ main.exe
result was: 8589934605
errnum was: 0
errnum expected: 0

result was: 9223372036854775807
errnum was: 34
errnum expected: 34

result was: -9223372036854775808
errnum was: 34
errnum expected: 34
```

The test passes, everything looks O.K.

## Build APR with CMake

### Dependencies
 * [OpenSSL](https://ci.modcluster.io/job/openssl-windows/arch=64,label=w2k12r2/33/artifact/OpenSSL_1_0_2h-64.zip)
 * [LibXML2](https://ci.modcluster.io/job/libxml2-windows/arch=64,label=w2k12r2/12/artifact/libxml2-v2.9.4-64.zip)
 * [APR 1.6.x head:e4fe1aa](https://github.com/apache/apr/tree/e4fe1aa5321ea9742b902db3763238b4c3b1a1c4)

### Get sources

```
C:\Users\karm\source
λ git clone https://github.com/apache/apr.git
Cloning into 'apr'...

C:\Users\karm\source
λ cd apr

C:\Users\karm\source\apr (trunk)
λ git checkout origin/1.6.x
HEAD is now at e4fe1aa53... Also affects clang 5 (and maybe older versions too)

C:\Users\karm\source\apr (HEAD detached at e4fe1aa)
λ mkdir build-default

C:\Users\karm\source\apr (HEAD detached at e4fe1aa)
λ cd build-default\

C:\Users\karm\source\apr\build-default (HEAD detached at e4fe1aa)
λ vcvars64
**********************************************************************
** Visual Studio 2017 Developer Command Prompt v15.0.26730.16
** Copyright (c) 2017 Microsoft Corporation
**********************************************************************
[vcvarsall.bat] Environment initialized for: 'x64'
```

### Default CMAKE_C_FLAGS_RELEASE

Pertinent excerpt from the [full log](https://gist.githubusercontent.com/Karm/d089dca61dfb4be1ef8437b5e7ad4f5d/raw/c33c4b654e07058a72dc2017e57079be91b0b388/gistfile1.txt):
```
λ cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ... <SNIP>
<SNIP>
C:\Users\karm\source\apr\build-default (HEAD detached at e4fe1aa)
λ testall.exe -v teststr
teststr             : SUCCESS
All tests passed.
```

### CMAKE_C_FLAGS_RELEASE set to /O2 /Wall /Zi

Pertinent excerpt from the [full log](https://gist.githubusercontent.com/Karm/ef024f2d67dbbc0923357930055dacf0/raw/697e726de3ddb40cc5d7b330340c76a1f33aa7c7/gistfile1.txt):
```
λ cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS_RELEASE="/O2 /Wall /Zi" ... <SNIP>
<SNIP>
C:\Users\karm\source\apr\build-overwritten-flags (HEAD detached at e4fe1aa)
λ testall.exe -v teststr
teststr             : /Line 263: for '999999999999999999999999999999999': errno was 0 not 34
FAILED 1 of 14
Failed Tests            Total   Fail    Failed %
===================================================
teststr                    14      1      7.14%
```

## Difference between CMakeCache.txt
### /MD
The difference is just this, as expected, [see the diff file](http://www.mergely.com/38UMnLC3/).

```
- CMAKE_C_FLAGS_RELEASE:STRING=/MD /O2 /Ob2 /DNDEBUG
+ CMAKE_C_FLAGS_RELEASE:STRING=/O2 /Wall /Zi
```
### WinDbg - APR Test
When executed within WinDbg, it is apparent that the difference ```/MD``` brings is the addition (or the lack thereof) of ```C:\WINDOWS\SYSTEM32\VCRUNTIME140.dll```, [see the diff](http://www.mergely.com/b5GTkE39/).
### WinDbg - main.exe "reproducer"
As I look at the reproducer from the beginning of this article, that actually doesn't reproduce the problem, I can see there is neither ```ucrtbase.dll``` nor ```VCRUNTIME140.dll``` loaded, it is just:
```
ModLoad: 00007ff7'b1350000 00007ff7'b1370000   image00007ff7'b1350000
ModLoad: 00007ffd'f9600000 00007ffd'f97db000   ntdll.dll
ModLoad: 00007ffd'f7430000 00007ffd'f74de000   C:\WINDOWS\System32\KERNEL32.DLL
ModLoad: 00007ffd'f6570000 00007ffd'f67b9000   C:\WINDOWS\System32\KERNELBASE.dll
```
, if I add ```/MD``` to the ```cl``` flags, [according to the manual on CRT LIbrary features](https://msdn.microsoft.com/en-us/library/abx4dbyh.aspx?f=255&MSPPError=-2147217396), both and are loaded:
```
ModLoad: 00007ff7'd4fe0000 00007ff7'd4fe6000   image00007ff7'd4fe0000
ModLoad: 00007ffd'f9600000 00007ffd'f97db000   ntdll.dll
ModLoad: 00007ffd'f7430000 00007ffd'f74de000   C:\WINDOWS\System32\KERNEL32.DLL
ModLoad: 00007ffd'f6570000 00007ffd'f67b9000   C:\WINDOWS\System32\KERNELBASE.dll
ModLoad: 00007ffd'f6470000 00007ffd'f6566000   C:\WINDOWS\System32\ucrtbase.dll
ModLoad: 00007ffd'e8830000 00007ffd'e8846000   C:\WINDOWS\SYSTEM32\VCRUNTIME140.dll
```
, so I am unable to reproduce the situation with APR lib test, when only ```ucrtbase.dll``` is loaded without any ```VCRUNTIME140.dll```.

## Conclusion
There is none, I don't know what's causing the glitch but for the fact that there is probably a buggy ```_strtoi64``` loaded if the one that is probably correct in ```VCRUNTIME140.dll``` is not used. When I use ```-DCMAKE_C_FLAGS_RELEASE="/MD /O2 /Ob2 /Wall /Zi"``` the test passes. If I remove [/MD](https://docs.microsoft.com/en-us/cpp/build/reference/md-mt-ld-use-run-time-library), i.e. ```-DCMAKE_C_FLAGS_RELEASE="/O2 /Ob2 /Wall /Zi"```, the test fails in the aforementioned fashion.

If anyone could enlighten me, drop a line to dev apr.apache.org, cc michal.babacek gmail. THX.
