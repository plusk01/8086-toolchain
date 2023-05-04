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
sudo apt-get install gcc-multilib g++-multilib libc6-dev-i386 lib32ncurses5-dev xterm
```

*Tested on Ubuntu 16.04 x86_64, Ubuntu 17.04 x86_64 & Ubuntu 17.10*

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
+ Parsing errors with Perl generated headers/macros: At some point the files were saved with 'DOS' line endings instead of Unix, so if you see a `^M` (Windows carriage return char) that is likely the issue and saving (or overwriting) the file correctly should fix it. Make sure to blow away any generated files.
+ If on Ubuntu 17.04 or higher, you may get the following error: `xterm: cannot load font '-misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-iso10646-1'`. emu86 will still work fine. If you wish to fix the error, you can try following the steps in this form:  http://forum.porteus.org/viewtopic.php?f=53&t=1013


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


## Website ##

The ECEn 425 website is stored in directory `ece425web.groups.et.byu.net/` and
contains lab instructions, 8086 resources, homework assignments, and class
slides.

### Browsing the website ##

You can open file `ece425web.groups.et.byu.net/index.html` locally with a web
browser, and all the links should work.

Another option is to host the website locally. In a terminal, do:

```shell
cd ece425web.groups.et.byu.net/
python3 -m http.server 8000
```
Then, access the local website in your browser at `localhost:8000`.
This option is nice because it allows Chrome extensions (like DarkReader) to
work (Chrome extensions are not allowed to run on local file paths).

### Website history

After Dr. Archibald retired in August 2020, the BYU ECEn department
discontinued ECEn 425. At some point in 2022, BYU also stopped hosting
[the original class website][1]. Luckily,
[the class website was archived by Wayback Machine][2], and the website was
saved to this repo on 2023-05-03 using [this tool][3].


[1]: http://ece425web.groups.et.byu.net/
[2]: https://web.archive.org/web/20220126003933/http://ece425web.groups.et.byu.net/
[3]: https://github.com/hartator/wayback-machine-downloader
