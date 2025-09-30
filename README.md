## Build

Simpol simpol

```sh
cd jit_aot_course
mkdir build
cd build
cmake -GNinja ..
ninja
./jit_aot_course
```

## Result

```sh
Function factorial(%n)
entry:
  jump loopCond
loopCond:
  %v0 = phi [%n, entry], [%v1, loopBody]
  %v2 = phi [1, entry], [%v3, loopBody]
  %v4 = cmp.gt %v0, 1
  cjump %v4, loopBody, afterLoop
loopBody:
  %v3 = mul %v2, %v0
  %v1 = sub %v0, 1
  jump loopCond
afterLoop:
  return %v2
```
