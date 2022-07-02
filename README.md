# Snake GB

An homebrew implementation of the classic Snake game for Nintendo Game Boy, a.k.a. "DMG". Written by [tomas7770](https://github.com/tomas7770) in C, using the GBDK-2020 API.

Tested on BGB 1.5.9 emulator, not tested on real hardware.

## Build instructions

1. Download/install a recent version of [GBDK-2020](https://github.com/gbdk-2020/gbdk-2020)

2. Add GBDK's "bin" directory to your system PATH.

3. On **Windows**, run the provided **compile.bat**.

4. On **other systems (Mac, Linux, etc.)**, install **Make**, open the terminal on the project directory, and enter `make`. **This method is untested**, but copying the contents of compile.bat and running them directly should work.

5. You should now have a ROM file that can run on a Game Boy or any compatible emulator.


