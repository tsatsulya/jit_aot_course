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

```sh
Test 1:
CFG: A: B, B: C, D, C: , D: E, E: B

Loop Analysis for Function: test1
========================================
Loop (Depth: 0):
  Header: B
  Latch: E
  Blocks: D E B
  Exit blocks: B
  Innermost: Yes
```

```sh
Test 2:
CFG: A: B, B: C, C: D, F, D: E, F, E: B, F:

Loop Analysis for Function: test2
========================================
Loop (Depth: 0):
  Header: B
  Latch: E
  Blocks: D C E B
  Exit blocks: C D
  Innermost: Yes
```

```sh
Test 3:
CFG: A: B, B: C, D, C: E, F, D: F, E: , F: G, G: B, H, H: A

Loop Analysis for Function: test3
========================================
Loop (Depth: 0):
  Header: A
  Latch: H
  Blocks: D B C F G H A
  Exit blocks: C
  Innermost: No

  Loop (Depth: 1):
    Header: B
    Latch: G
    Blocks: C F G D B
    Exit blocks: G C
    Innermost: Yes

Loop (Depth: 1):
  Header: B
  Latch: G
  Blocks: C F G D B
  Exit blocks: G C
  Innermost: Yes
```

```sh
Test 4:
CFG: A: B, B: C, F, C: D, D: , E: D, F: E, G, G: D

Loop Analysis for Function: test4
========================================
No loops found.
```

```sh
Test 5:
CFG: A: B, B: C, J, C: D, D: E, C, E: F, F: E, G, G: H, I, H: B, I: K, J: C, K:

Loop Analysis for Function: test5
========================================
Loop (Depth: 0):
  Header: B
  Latch: H
  Blocks: J C E F G D H B
  Exit blocks: G
  Innermost: No

  Loop (Depth: 1):
    Header: C
    Latch: D
    Blocks: D C
    Exit blocks: D
    Innermost: Yes

  Loop (Depth: 1):
    Header: E
    Latch: F
    Blocks: F E
    Exit blocks: F
    Innermost: Yes

Loop (Depth: 1):
  Header: C
  Latch: D
  Blocks: D C
  Exit blocks: D
  Innermost: Yes

Loop (Depth: 1):
  Header: E
  Latch: F
  Blocks: F E
  Exit blocks: F
  Innermost: Yes
```