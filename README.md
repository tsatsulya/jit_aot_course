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

## Results

### BASIC FUNCTION DUMP:
```sh
Function simple_factorial(n)
entry:
  return one
exit:


Function conditional_factorial(n)
entry:
  jump check
check:
  %v0 = cmp.le n, one
  if (%v0 = cmp.le n, one) ( jump base ) else ( jump recurse )
base:
  return one
recurse:
  %v1 = sub n, one
  %v2 = mul n, %v1
  return %v2


Function just_factorial(n)
entry:
  jump check
check:
  %v0 = cmp.le n, one
  if (%v0 = cmp.le n, one) ( jump base_case ) else ( jump recursive )
base_case:
  return one
recursive:
  %v1 = sub n, one
  jump compute
compute:
  %v2 = mul n, %v1
  jump exit
exit:
  return %v2


Function iterative_factorial(n)
entry:
  %v0 = add one, zero
  %v1 = add n, zero
  jump loop_check
loop_check:
  %v2 = cmp.gt %v1, zero
  if (%v1 = cmp.gt %v0, zero) ( jump loop_body ) else ( jump exit )
loop_body:
  %v3 = mul %v0, %v1
  %v4 = sub %v1, one
  jump loop_check
exit:
  return %v3
```

### CFG ANALYSIS:

```sh
Function simple_factorial CFG:
entry:
  Predecessors:
  Successors:
entry:
  return one

exit:
  Predecessors:
  Successors:
exit:

Function conditional_factorial CFG:
entry:
  Predecessors:
  Successors: check
entry:
  jump check

check:
  Predecessors: entry
  Successors: recurse base
check:
  %v0 = cmp.le n, one
  if (%v0 = cmp.le n, one) ( jump base ) else ( jump recurse )

base:
  Predecessors: check
  Successors:
base:
  return one

recurse:
  Predecessors: check
  Successors:
recurse:
  %v1 = sub n, one
  %v2 = mul n, %v1
  return %v2

Function just_factorial CFG:
entry:
  Predecessors:
  Successors: check
entry:
  jump check

check:
  Predecessors: entry
  Successors: recursive base_case
check:
  %v0 = cmp.le n, one
  if (%v0 = cmp.le n, one) ( jump base_case ) else ( jump recursive )

base_case:
  Predecessors: check
  Successors:
base_case:
  return one

recursive:
  Predecessors: check
  Successors: compute
recursive:
  %v1 = sub n, one
  jump compute

compute:
  Predecessors: recursive
  Successors: exit
compute:
  %v2 = mul n, %v1
  jump exit

exit:
  Predecessors: compute
  Successors:
exit:
  return %v2

Function iterative_factorial CFG:
entry:
  Predecessors:
  Successors: loop_check
entry:
  %v0 = add one, zero
  %v1 = add n, zero
  jump loop_check

loop_check:
  Predecessors: loop_body entry
  Successors: exit loop_body
loop_check:
  %v2 = cmp.gt %v1, zero
  if (%v1 = cmp.gt %v0, zero) ( jump loop_body ) else ( jump exit )

loop_body:
  Predecessors: loop_check
  Successors: loop_check
loop_body:
  %v3 = mul %v0, %v1
  %v4 = sub %v1, one
  jump loop_check

exit:
  Predecessors: loop_check
  Successors:
exit:
  return %v3
```

### Tests for CFG analysis

```sh
Dominators for example1:
F: F B A
E: E F B A
D: D B A
G: F G B A
C: C B A
B: B A
A: A

Dominators for example2:
I: I G F E A B C D
H: H G F E A B C D
G: G F E A B C D
F: F E A B C D
E: E A B C D
K: I G F E A B C K D
D: D C B A
J: J B A
C: C B A
B: B A
A: A

Dominators for example3:
I: I A B
H: H F E B A
G: G A B
F: F E B A
E: E B A
D: D A B
C: A C B
B: B A
A: A
```


### LOOP ANALYSIS:

### Tests for loop analysis

```
```