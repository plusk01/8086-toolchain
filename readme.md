8086 Toolchain
==============

This project is for BYU ECEn 425, and was taking from the CAEDM servers (`/ee2/ee425/src/dist`). The purpose for posting on GitHub is to make the build process available for Ubuntu 12.04+ and Mac OS X, along with any minor modifications along the way.

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
 directories and running make individually so you can see the warnings/errors mo
re clearly.

To use the binaries from anywhere, see **Setting Your Path** below.

### Setting Your Path ###

Put this at the bottom of your `~/.profile` `~/.bashrc` (Or whatever your shells startup script is).

```bash
# ECEn 425 binaries
export PATH="/Users/plusk01/Documents/8086-toolchain/bin":$PATH
```
