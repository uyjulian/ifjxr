# JXR Susie plugin

This plugin allows reading of JXR / JPEG XR images in Susie plugin-compatible image viewers.

## Downloads

The following binaries are available:  
* [Win32 (Intel 32-bit GCC)](https://github.com/uyjulian/ifjxr/releases/latest/download/ifjxr.intel32.gcc.7z)  
* [Win32 (Intel 64-bit GCC)](https://github.com/uyjulian/ifjxr/releases/latest/download/ifjxr.intel64.gcc.7z)  
* [Win32 (Intel 32-bit Clang)](https://github.com/uyjulian/ifjxr/releases/latest/download/ifjxr.intel32.clang.7z)  
* [Win32 (Intel 64-bit Clang)](https://github.com/uyjulian/ifjxr/releases/latest/download/ifjxr.intel64.clang.7z)  
* [Win32 (ARM 32-bit Clang)](https://github.com/uyjulian/ifjxr/releases/latest/download/ifjxr.arm32.clang.7z)  
* [Win32 (ARM 64-bit Clang)](https://github.com/uyjulian/ifjxr/releases/latest/download/ifjxr.arm64.clang.7z)  

## Preparing the environment

Install the `act` tool: https://nektosact.com/installation/index.html

## Building

```
$ git clone https://github.com/uyjulian/ifjxr.git
$ cd ifjxr
$ act run --artifact-server-path $PWD/build-artifacts
```
Output artifacts will be in `build-artifacts` folder.

## How to use

Susie plugins are compatible with many programs, including these:

- [A to B converter](http://www.asahi-net.or.jp/~KH4S-SMZ/spi/abc/index.html)  
- [DYNA](https://hp.vector.co.jp/authors/VA004117/dyna.html)  
- [Linar](http://hp.vector.co.jp/authors/VA015839/)  
- [Susie](http://www.digitalpad.co.jp/~takechin/betasue.html#susie32)  
- [picture effecter](http://www.asahi-net.or.jp/~DS8H-WTNB/software/index.html)  
- [stereophotomaker](http://stereo.jpn.org/eng/stphmkr/)  
- [vix](http://www.forest.impress.co.jp/library/software/vix/)  

## License

This project is licensed under the MIT license. Please read the `LICENSE` file for more information.
