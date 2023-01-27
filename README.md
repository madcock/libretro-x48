# libretro-crocods

Look for .kcr file format to use CrocoDS at its best: https://github.com/redbug26/crocods-core/wiki/kcr

## Install on Recalbox

- SSH on your recalbox
- Type the following command:
``` 
curl https://raw.githubusercontent.com/redbug26/libretro-crocods/master/install_recalbox.sh | sh
``` 
- Restart your recalbox
- Choose crocods core in the Amstrad settings in Emulation Station

(Repeat each time you update your recalbox)

## Build instructions on other systems

``` 
git clone https://github.com/redbug26/libretro-crocods.git
cd libretro-crocods/
git submodule update --init --recursive
``` 

Compile for osX
``` 
make -f Makefile.libretro platform="osx" -j2 CC="cc" 
```

Compile for linux
``` 
make -f Makefile.libretro platform="linux" -j2 CC="cc" 
```

Compile for win (via MSYS2 MingW)
``` 
make -f Makefile.libretro platform="win" -j2 CC="cc"
```

Compile for raspberry (from Ubuntu)
```
sudo apt-get install gcc-arm-linux-gnueabihf make
make -f Makefile.libretro platform="unix" -j2 CC="arm-linux-gnueabihf-gcc"
```


http://www.hpcalc.org/hp48/pc/emulators/gxrom-k.zip
# libretro-x48
