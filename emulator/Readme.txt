This directory contains the code for the Emu86 simulator. To compiler,
edit the file SysConfig.h to configure it for the OS (Unix or
Linux). If compiled for Windows using Visual C++ (6.0 was used to
develop it) then it will detect that the target is windows and not
changes to SysConfig.h are needed.

To configure for HP Unix, make sure that HPUX_CONSOLE is defined in
SysConfig.h and that LINUX_X86_CONSOLE is undefined. For x86 Linux,
make sure that the opposite is true. Non x86 versions of Linux are not
supported.

Wade Fife
December 24, 2004
