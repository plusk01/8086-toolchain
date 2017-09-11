8086 Toolchain
==============

This project is for BYU ECEn 425, and was taken from the CAEDM servers (`/ee2/ee425/src/dist`). The purpose for posting on GitHub is to make the build process available for Ubuntu 12.04+ and Mac OS X, along with any minor modifications along the way.

**Disclaimer:** For ECEn 425 projects, this may or may not compile exactly the same as the CAEDM machines

## Installing ##

You can either clone this repo, or on the side click 'Download ZIP'.

To clone:

```bash
$ git clone https://github.com/plusk01/8086-toolchain.git
```

**Prerequisites for Linux:**

There are a few dependencies needed for this toolchain to be built correctly.

On Debian based systems (Ubuntu, etc), run the following in your terminal:

```bash
sudo apt-get install gcc-multilib g++-multilib libc6-dev-i386 lb32ncurses5-dev
```

*Tested on Ubuntu 12.04 x86_64.*

**Prerequisites for Mac OS X:**

Your system will depend on the X11 Window system, which is now called XQuartz. It can be found [here](http://xquartz.macosforge.org/). (Tested with version 3.7.7)

*Tested on Mac OS X Yosemite.*

**Building on Linux/Mac:**

Then, `cd` into the repo and run `make`:

```bash
$ cd 8086-toolchain/
$ make
```

If all goes as planned, you will have 5 binaries in your `./8086-toolchain/bin/`
 directory. If there are any problems, you can try `cd`ing into each of the tool
 directories and running make individually so you can see the warnings/errors more clearly.

To use the binaries from anywhere, see **Setting Your Path** below.

### Setting Your Path ###

Put this at the bottom of your `~/.profile` `~/.bashrc` (Or whatever your shells startup script is).

```bash
# ECEn 425 binaries
export PATH="/Users/plusk01/Documents/8086-toolchain/bin":$PATH
```

Make sure to reload your shells startup by either closing your shell and reopening it, or using the `source` command:

```bash
source ~/.profile
```

This will actually add your toolchain bin to the path.

## Common Problems ##

**Linux:**
+ `cdefs` not defined: Make sure Linux prereqs are installed

**Mac:**
+ `cpp` does not remove `//` comments correctly: See Makefile example below, make sure to add `-xc++` flag to `cpp` to remove c99 style comments.

### Makefile Example ###

```Makefile
lab1.bin:       lab1final.s
                nasm lab1final.s -o lab1.bin -l lab1.lst  # Step 4, Assemble

lab1final.s:    clib.S lab1asm.S lab1.s
                cat clib.S lab1asm.S lab1.s > lab1final.s # Step 3, Concatenate

lab1.s:         lab1.c
                cpp -xc++ lab1.c lab1.i    # Step 1, Preprocess
                c86 -g lab1.i lab1.s       # Step 2, Compile

clean:  
                rm lab1.bin lab1.lst lab1final.s lab1.s lab1.i

test:
                emu86 lab1.bin
```

**Makefile error:** If you copy this directly and paste into a Makefile, the tabs will often be converted to spaces. You will get a "Missing separator" error from make. Just replace the indentions with two tabs and it should work.
