@echo OFF
REM		config.bat
REM		~~~~~~~~~~
REM	This file checks if the current CONFIG.H header file
REM     is the same as the header file given as the parameter.
REM     If not it copies the given header file over the CONFIG.H
REM     header and deletes all files that are now out of date.

diff %1 config.h >NUL
if NOT ERRORLEVEL 1 goto END
copy %1 config.h
del c86.rs?
del *.obj
del *.exe
:END
