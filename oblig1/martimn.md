---
title: "Obligatorisk oppgave --- IN3230/IN4230"
date: "Høst 2022"
author: "Kristjon Ciko <kristjoc@ifi.uio.no>"
name: "Martin Mihle Nygaard <martimn@ifi.uio.no>"
username: "martimn"
submission date: "2022-09-04"
course code: "IN4230"
---


## 1. What does this function return?

    1  int f1(int a, int b) {
    2      return (a>b?a:b)
    3  }

(A) Compiler error.
(B) The smaller value of the two passed parameters.
(C) The greater value of the two passed parameters.
(D) Runtime error.

Answer: A.

Explanation: Forgot a semicolon after return statement. With the semicolon, it
returns the greater `int` (or `b` if equal).


## 2. Which is not a correct way to declare a string variable?

(A) `char str = "Hei IN3230"`
(B) `char *str = "Hei IN3230"`
(C) `char str[20] = {'H', 'e', 'i', ' ', 'I', 'N', '3', '2', '3', '0', '\0'};`
(D) `char str[] = "Hei IN3230";`

Answer: Definitely A. (Also B, if you're strict about forgotten semi-colons.)

Explanation:

(A) Just initialises a single `char`. A string is an array of `char`s. Besides,
    forgotten semi-colon.
(B) Would be fine with a semi-colon at the end. This initializes a pointer to
    a statically stored string somewhere in memory.
(C) Allocates space for 20 `char`s of continuous memory, which is more than
    enough for the 10 we need, plus a terminating null byte.
(D) Is equivalent to B., just different notation.


## 3. Which function do you use to deallocate memory?

(A) `dalloc()`
(B) `release()`
(C) `free()`
(D) `dealloc()`

Answer: C.

Explanation: To quote the manual, `malloc(3)`:

> The `free()` function frees the memory space pointed to by [a pointer
> argument] `ptr`, which must have been returned by a previous call to
> `malloc()`, `calloc()`, or `realloc()`.  Otherwise, or if `free(ptr)` has
> already been called before, undefined behavior occurs. If `ptr` is NULL, no
> operation is performed.

## 4. In which segment does dynamic memory allocation take place?

(A) BSS segment.
(B) Stack.
(C) Data segment.
(D) Heap.

Answer: D.

Explanation:

BSS segment
: is used for static global scope memory declarations that are not initialized
at runtime start.

stack
: is used for local scope automatic memory allocation, such as declarations
  within a function.

data segment
: is used for global (or static local) initializations.

heap
: used for dynamically allocated memory, in C typically managed with
  `malloc` and `free`. Needed for persistent memory, through multiple function
  calls, but with variable size (e.g. depending on user input).


## 5. What is not a valid command with this declaration?

    1 char *string[20] = {"one", "two", "three"};

(A) `printf("%c", string[1][2]);`
(B) `printf("%s", string[1][2]);`
(C) `printf(string[1]);`
(D) `printf("%s", string[1]);`

Answer: B.

Explanation: A. is fine, and just prints the character `o`. B., um, compiles,
but gives a warning: it interprets the `char` as pointer to the start of
a string. Since `string[1][2]` is actually a `char` with the number 0x6F (`o`
in ascii), `printf` will look for a printable string at the address 0x6F. This
is highly restricted memory and the OS will terminate the program; you'll get
a segmentation fault. C. gives a warning, since `printf` wants a `conts char
* restrict`, a string literal, but it still works, I guess. D. silences the
warning from C., and works.


## 6. Void pointer `vptr` is assigned the address of float variable `g`. What is a valid way to dereference `vptr` to assign its pointed value to a float variable named `f` later in the program?

(A) `f = *(float)vptr;`
(B) `f = (float)*vptr;`
(C) `f = *(float *)vptr;`
(D) `f = (float *)vptr;`

Answer: C.

Explanation: Not A., since you can't cast a pointer to a float. Not B., since
dereferencing a void pointer gives you a `void` type, which is not castable to
float. Not D., since this just gives a float _pointer_, not an actual float;
but dereference this and you get C., which is the answer.


## 7. What does the `strcmp(str1, str2);` function return?

(A) True (1) if `str1` and `str2` are the same; NULL if `str1` and `str2` are
    not the same.
(B) 0 if `str1` and `str2` are the same; a negative number if `str1` is less
    than `str2`; a positive number if `str1` is greater than `str2`.
(C) True (1) if `str1` and `str2` are the same; false (0) if `str1` and `str2`
    are not the same.
(D) 0 if `str1` and `str2` are the same; a negative number if `str2` is less
    than `str1`; a positive number if `str2` is greater than `str1`.

Answer: B.

Explanation: From the manual, `strcmp(3)`:

> The `strcmp()` and `strncmp()` functions return an integer less than, equal
> to, or greater than zero if `s1` (or the first n bytes thereof) is found,
> respectively, to  be less than, to match, or be greater than `s2`.


## 8. How many times does the code inside the while loop get executed in this program?

    1  void main() {
    2  int x=1;
    3      while (x++<100) {
    4          x*=x;
    5          if (×<10) continue;
    6          if (x>50) break;
    7      }
    8  }

(A) 5
(B) 50
(C) 100
(D) 3

Answer: D.

Explanation: After the first pass `x` is `2*2=4`, on the second it's `5*5=25`,
and on the last pass `26*26=676`, which activates the `break`. The `x++`
increment activates every time the `while` line is evaluated.


## 9. Which choice is an include guard for the header file `mylib.h`?

(A)     #ifndef MYLIB_H
        #define MYLIB_H
        //mylib.h content
        #endif /* MYLIB_H */
(B)     #define MYLIB_H
        #include "mylib.h"
        #undef MYLIB_H
(C)     #ifdef MYLIB_H
        #define MYLIB_H
        //mylib.h content
        #endif /* MYLIB_H */
(D)     #ifdef MYLIB_H
        #undef MYLIB_H
        //mylib.h content
        #endif /* MYLIB_H */

Answer: A. 

Explanation: A rough translation would be: _if_ the variable `MYLIB_H` is _not_
already defined, define it and import stuff, else do nothing. Makes sure we're
not declaring stuff multiple times.


## 10. What is the sequence printed by the following program?

    1  #include <stdio.h>
    2
    3  void print_3_ints(int *ip)
    4  {
    5      printf("%d %d %d ", ip[0], ip[1], ip[2]);
    6  }
    7
    8
    9  int main(void)
    10 {
    11     int array[] = { 5, 23, 119, 17 };
    12     int *p, *q, d;
    13
    14     p = array; q = p + 1;
    15
    16     print_3_ints(q);
    17
    18     d = *(p++);
    19     printf("%d ", d);
    20
    21     d = ++(*p);
    22     printf("%d ", d);
    23
    24     printf("\n");
    25     return 0;
    26 }

(A) 23 119 17 5 24
(B) 23 119 17 23 24
(C) 24 120 18 6 25
(D) 25 120 18 25 26

Answer: A.

Explanation: `p` points to the start of the integer array, `q` points to the
second element (since we added 1); so when we print `q` with our custom print
function, we start the count from `23`. Then, on line 18, we set `d` to 5,
increment `p` _after_ the assignment, and prints it (5). Lastly, we dereference
`p` to the 5 it's pointing to, increment it _before_ assigning `d`, and prints
`d` (now 24).


## 11. What is the output of the following program?

    1  #include <stdio.h>
    2
    3  void func(int *ptr)
    4  {
    5      *ptr = 30;
    6  }
    7
    8  int main()
    9  {
    10     int y = 20;
    11     func(&y);
    12     printf("%d\n", y);
    13
    14     return 0;
    15 }

(A) 20.
(B) 30.
(C) Compiler error.
(D) Runtime error.

Answer: B.

Explanation: The function sets the integer at the address the integer pointer
`ptr` refers to to 30. The initial value of `y` is irrelevant.


## 12. What is the output of the following program?

    1  #include <stdio.h>
    2  #include <stdlib.h>
    3
    4  int main()
    5  {
    6      int i, numbers[1];
    7      numbers[0] = 15;
    8      free(numbers);
    9      printf("Stored integers are ");
    10     printf("numbers[%d] = %d ", 0, numbers[0]);
    11
    12     return 0;
    13 }

(A) Runtime error.
(B) Compilation error.
(C) 0.
(D) Garbage value.

Answer: A.

Explanation: The variable `numbers` is not dynamically allocated memory (the
heap) and subsequently not free-able with `free()`. It is automatically
allocated memory (the stack).


## 13. What is the error of this program?

    1  #include <stdio.h>
    2  #include <stdlib.h>
    3
    4  int main()
    5  {
    6      char *ptr;
    7      *ptr = (char)malloc(8);
    8      strcpy(ptr, "RAM");
    9      printf("%s", ptr);
    10     free(ptr);
    11     return 0;
    12 }

(A) Error in `strcpy()` statement.
(B) Error in `*ptr = (char)malloc(8);`.
(C) Error in `free(ptr);`.
(D) No error.

Answer: A. (but also C.)

Explanation: The `string.h` header is not included, so `strcpy()` is not
defined, this gives a compilation error. Besides this, `malloc()` returns a
pointer to some allocated memory, and in line 7 we set the value at `ptr` to be
the address of this pointer. `strcpy()` (if it were included correctly)
overwrites this address with the string `RAM`. Now we've lost the address of
the dynamically allocated memory, and we try to `free()` the automatically
allocated memory declared on line 6, which gives a runtime error.


## 14. What is the output of the following program?

    1  #include <stdio.h>
    2
    3  struct result{
    4      char sub[20];
    5      int marks;
    6  };
    7
    8  int main()
    9  {
    10     struct result res[] = {
    11          {"IN3230",100},
    12          {"IN4230",90},
    13          {"Norsk",85}};
    14
    15     printf("%s ", res[1].sub);
    16
    17     printf("%d\n", (*(res+2)).marks);
    18
    19     return 0;
    20 }

(A) IN3230 100
(B) IN4230 85
(C) IN4230 90
(D) Norsk 100

Answer: B.

Explanation: On line 15 we print the `sub` of the second `result` structure.
On line 17 we step 2 away from the initial point of `res`, i.e. the third
`result` structure, dereference this, and print the `marks` integer.


## 15. What is the output of this code snippet?

    1  void main()
    2  {
    3      struct bitfields {
    4          int bits_1: 2;
    5          int bits_2: 9;
    6          int bits_3: 6;
    7          int bits_4: 1;
    8      }bit;
    9
    10     printf("%d\n", sizeof(bit));
    11 }

(A) 2
(B) 3
(C) 4
(D) 0

Answer: C.

Explanation: The `sizeof()` operator returns the memory size of its argument in
number of `char`s. The `struct` defines a bit field, assigning labels to
different sections of a single `int`. On my system `sizeof int` is 4, meaning
an `int` is the size of 4 `char`s; consequently `sizeof bit` returns the same.


## 16. In which line is the BUG in the following program?

    1  #include <stdio.h>
    2
    3  int main()
    4  {
    5      int a, *ptr;
    6      a = 25;
    7      *ptr = a + 5;
    8
    9      return 0;
    10 }

Answer: 1? Maybe 7?

Explanation: The include is superfluous? I don't know. This program runs fine
on my computer. At line 7, the pointer is dereferenced while there is an
uninitialized value where it's pointing --- you're not using that value for
anything in this case, but it is ~~bad practice~~ technically undefined
according to C11 6.5.3.2.


## 17. In which line is the BUG in the following program?

    1  #include <stdio.h>
    2  struct var {
    3      int value;
    4      int *address;
    5  };
    6
    7  int main()
    8  {
    9      struct var y;
    10     int a = 10;
    11     int *ptr = &a;
    12
    13     y.value = *ptr;
    14     (&y)->address = ptr;
    15
    16     printf("value %d\n", y.value);
    17     printf("address %p\n", y->address);
    18     return 0;
    19  }

Answer: 17

Explanation: `y` is not a pointer, it's a `struct var`, so you cannot
dereference it. `y->address` is shorthand for `(*y).address`.


## 18. What are the two mistakes in the following program?

    1   #include <stdio.h>
    2
    3   int main () {
    4       const char src[8] = "IN3230";
    5       char dest[8];
    6
    7       strcpy(dest, "Hellllo");
    8
    9       memcpy(dest, src, strlen(src));
    10
    11      printf("%s\n", dest);
    12      return(0);
    13  }

Answer: 7 and 9.

Explanation: Forgotten to include `string.h` for the `strcpy`, `memcpy`, and
`strlen` functions on lines 7 and 9. If the tailing `o` printed was undesired,
one could add a `+1` to the `strlen` call, to also copy the null byte in `src`.
