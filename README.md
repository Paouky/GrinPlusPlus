<p align="center">
  <img width="400" height="94" src="https://user-images.githubusercontent.com/8020386/93075419-593ff300-f6b8-11ea-94c2-2b532d2cedbd.png" alt="Grin++">
</p>

![Demo](https://user-images.githubusercontent.com/8020386/92248412-39137580-eefb-11ea-836c-1b4dfcc5a1c1.png)

Website: https://grinplusplus.github.io/

Wiki: https://github.com/GrinPlusPlus/GrinPlusPlus/wiki

### Project Status
The node and wallet have been released on Mainnet for 64-bit Windows & Mac OS X beta-testing!

## Build Instruction

### Linux ARM64

```
<<<<<<< HEAD
$ docker build -t grinpp-linux-arm64 .
$ docker create -ti --name dummy grinpp-linux-arm64 /bin/bash
=======
$ docker build -t grinpp-arm64 .
$ docker create -ti --name dummy grinpp-arm64 bash
>>>>>>> 5b215e6e... adding tools
$ docker cp dummy:/work/bin/Release output
$ cd output
$ mv Release linux-arm64
```

\* The folder called `linux-arm64` should contain statically compiled binary called **`GrinNode`** and a `tor` folder inside with a statically compiled tor **`tor`** binary.