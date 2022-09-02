% Title: Obligatorisk oppgave - IN3230/IN4230 - Høst 2022

% Author: Kristjon Ciko <kristjoc@ifi.uio.no>

% ------

% Name: 

% Username: 

% Submission date: 

% Course code: 

% ------

## 1. What does this function return?

    1  int f1(int a, int b) {
    2      return (a>b?a:b)
    3  }

A. compiler error

B. the smaller value of the two passed parameters

C. the greater value of the two passed parameters

D. runtime error

Answer:

Explanation:


## 2. Which is not a correct way to declare a string variable?

A. `char str = "Hei IN3230"`

B. `char *str = "Hei IN3230"`

C. `char str[20] = {'H', 'e', 'i', ' ', 'I', 'N', '3', '2', '3', '0', '\0'};`

D. `char str[] = "Hei IN3230";`

Answer:

Explanation:


## 3. Which function do you use to deallocate memory?

A. `dalloc()`

B. `release()`

C. `free()`

D. `dealloc()`

Answer:

Explanation:


## 4. In which segment does dynamic memory allocation take place?

A. BSS segment

B. stack

C. data segment

D. heap

Answer:

Explanation:


## 5. What is not a valid command with this declaration?

    1 char *string[20] = {"one", "two", "three"};

A. `printf("%c", string[1][2]);`

B. `printf("%s", string[1][2]);`

C. `printf(string[1]);`

D. `printf("%s", string[1]);`

Answer:

Explanation:


## 6. Void pointer `vptr` is assigned the address of float variable `g`. What is a valid way to dereference `vptr` to assign its pointed value to a float variable named `f` later in the program?

    1  float g;
    2  void *vptr = &g;

A. `f = *(float)vptr;`

B. `f = (float)*vptr;`

C. `f = *(float *)vptr;`

D. `f = (float *)vptr;`

Answer:

Explanation:


## 7. What does the `strcmp(str1, str2);` function return?

A.
• true (1) if str1 and str2 are the same
• NULL if str1 and str2 are not the same

B.
• 0 if str1 and str2 are the same
• a negative number if str1 is less than str2
• a positive number if str1 is greater than str2

C.
• true (1) if str1 and str2 are the same
• false (0) if str1 and str2 are not the same

D.
• 0 if str1 and str2 are the same
• a negative number if str2 is less than str1
• a positive number if str2 is greater than str1

Answer:

Explanation:


## 8. How many times does the code inside the while loop get executed in this program?

    1  void main() {
    2  int x=1;
    3      while (x++<100) {
    4          x*=x;
    5          if (×<10) continue;
    6          if (x>50) break;
    7      }
    8  }

A. 5

B. 50

C. 100

D. 3

Answer:

Explanation:


## 9. Which choice is an include guard for the header file mylib.h?

A.
```
#ifndef MYLIB_H
#define MYLIB_H
//mylib.h content
#endif /* MYLIB_H */
```

B.
```
#define MYLIB_H
#include "mylib.h"
#undef MYLIB_H
```

C.
```
#ifdef MYLIB_H
#define MYLIB_H
//mylib.h content
#endif /* MYLIB_H */
```

D.
```
#ifdef MYLIB_H
#undef MYLIB_H
//mylib.h content
#endif /* MYLIB_H */
```

Answer:

Explanation:


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

A. 23 119 17 5 24

B. 23 119 17 23 24

C. 24 120 18 6 25

D. 25 120 18 25 26

Answer:

Explanation:


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

A. 20

B. 30

C. Compiler error

D. Runtime error

Answer:

Explanation:


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

A. Runtime error

B. Compilation error

C. 0

D. Garbage value

Answer:

Explanation:


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

A. Error in `strcpy()` statement.

B. Error in `*ptr = (char)malloc(8);`

C. Error in `free(ptr);`

D. No error

Answer:

Explanation:


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
    12    		{"IN4230",90},
    13    		{"Norsk",85}};
    14
    15     printf("%s ", res[1].sub);
    16
    17     printf("%d\n", (*(res+2)).marks);
    18
    19     return 0;
    20 }

A. IN3230 100

B. IN4230 85

C. IN4230 90

D. Norsk 100

Answer:

Explanation:


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

A. 2

B. 3

C. 4

D. 0

Answer:

Explanation:


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

Answer:

Explanation:


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

Answer:

Explanation:


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

Answer:

Explanation:
