# C Coding Guidelines

[[_TOC_]]

## Introduction

### Purpose

This document defines the minimum common coding practices for development of embedded product software.

### Definitions

| Term | Description |
| - | - |
|ANSI | American National Standards Institute. |
|ASCII | American Standard for Coded Information Interchange. |
|Should | General Rule to follow unless readability is adversely affected. |
|Shall | Explicit Rule to follow. |

# Basics

### General Rules

1. Keep it **SIMPLE**. Break down complexity into simpler chunks. If complexity is necessary, clearly comment reasons for complexity.
2. Be **EXPLICIT**. Avoid implicit or obscure features of the language. This helps for portability and readability.
3. Be **CONSISTENT**. Use the same rules as broadly as possible.
4. Minimize **SCOPE**. Don't try to incorporate all functionality into one call.

### Other Considerations

- Remain focused on specific requirements per module. Evaluate optimization against the overall system requirement.
- Error recovery or detection should be included on all system calls.
- Compilation should have all warnings turned on. Before checking in changes, there should be no warnings during compilation.

## C Language Guidelines

### Preprocessor Commands

1. A consistent set of project wide defined types shall be used and shall be instituted in a header file.
2. Predefined types such as 'int', 'char', 'long', etc shall be avoided except when used with the following restrictions.
    * *char* - May be used only for ASCII data.
    * *int* - May be used if variable contains data in the range of -128 to 127
    * *unsigned int* - May be used if variable contains data in the range of  0  to 255

3. The following standard types shall be used where feasible (C99 data types as defined in stdbool.h and stdint.h).

```C
    int8_t          8-bit, signed
    uint8_t         8-bit, unsigned
    int16_t         16-bit, signed
    uint16_t        16-bit, unsigned
    int32_t         32 bit signed
    uint32_t        32 bit unsigned
    int64_t         64-bit, signed
    uint64_t        64-bit, unsigned
    bool            int32_t type assigned either 'true' or 'false'
```

4. The `#define` directive shall not be used to create syntax extensions that make C code look like the code of another language.
5. The `#define` directive shall not be used to redefine reserved words or standard functions.
6. Symbolic constants shall be used; hard-coded numeric values shall not be used except for 0 and 1.
7. `true` shall be defined as 1 and `false` shall be defined as 0.
8. If one symbolic constant definition affects another, the relationship shall be made evident in the definition. There shall not be two independent definitions.

Example:
```C
    #define PI s3.14157
    #define TWO_PI (2*PI)
```

9. Any `#define` definition that forms an expression containing spaces, arguments, or operators shall enclose the expression within parentheses.
10. Each appearance of a formal parameter in the replacement text of a macro definition shall be enclosed in parentheses.
11. The replacement text of macro definitions containing more than one statement or line shall be enclosed in braces.
12. A macro shall never be invoked with arguments containing assignment, increment or decrement operators, or function calls.

### Data Representation

13. The type of data shall always be explicitly stated; do not allow the compiler to default the data type to integer.
14. No more than one variable shall be declared on a single line.
15. Type conversions should not be made from a longer to a shorter type.

### Constants

16. Variables that are used to hold read-only data should be declared using the const type qualifier.
17. An escape sequence (the "\" character) shall be used to encode non-printable or extended ASCII characters within character constants and strings.

### Structures and Typedefs

18. Structure and union members shall only be referred to using their symbolic names rather than the numeric values of the structure / union offsets.
19. Either all, only the first, or none of the enumeration constants within an enumeration type shall be assigned a value, except for a MAX entry for boundary validation.

Example:
```C
    typedef enum _DIAG_DEBUG_CMD_T
    {
        DIAG_DEBUG_GO    = 1,
        DIAG_DEBUG_PAUSE = 6,
        DIAG_DEBUG_STEP  = 7,
        DIAG_DEBUG_STEPO = 8,
        DIAG_DEBUG_COMP  = 9,
        DIAG_DEBUG_MAX
    } DIAG_DEBUG_CMD;
```

### Variables

20. Prototype or module global variables shall only be declared prior to the first function definition within a module.
21. Variable types, names, and their descriptive comments should be aligned vertically.
22. Global variables should be avoided as they introduce unnecessary coupling.
23. Global variables shall not be redefined, either as local variables or as different data type.
24. All inter-module or subsystem global variables should be defined in a single subsystem structure.
25. An auto-incremented or decremented variable shall only be referenced once on a line.
26. To avoid problems with the compiler's optimizer, the type qualifier volatile shall be used when declaring variables that reference memory-mapped hardware registers or values that may be changed in an interrupt function.
27. Use of the storage class register should be minimized to avoid limiting the number of registers available to the compiler.

### Functions

28. All functions shall be explicitly prototyped prior to use except main() and local functions that are defined prior to being used.
29. The names of formal parameters to functions shall be specified and shall be the same in both the declaration and definition of the function.
30. The return type of a function shall be stated explicitly. If a function does not return a value, then its type shall be specified as void.
31. A function used only within the file that it is defined shall be typed as static.
32. A function should have a single point of return whenever possible.

### Pointers

33. Pointers shall not have more than 2 levels of indirection.
34. Only variables that are declared as pointer variables shall be used to store pointer values.
35. Each pointer variable shall be declared as a pointer to the same type as the variable to which it will point. If the type of the variable that a pointer will point to is not known at compile time, the pointer shall be declared as a void *.
36. All pointers shall be initialized to point to valid storage or NULL before they are used.
37. NULL pointers shall not be passed to standard library de-allocating functions.
38. Pointer type conversion should not be made from a byte to longer type as a pointer to a char (located at an odd address) may cause the program to crash when the pointer is converted to a word or long_int pointer and dereferenced.

### Operators

39. Programs shall not depend on the order of evaluation of expressions, except as guaranteed for the following operators:

```C
    (a, b)     // comma operator (note the comma between arguments)
    (a && b)   // logical AND
    (a || b)   // logical OR
```

40. Parentheses shall be used to clarify the order of evaluation for operators in expressions.
41. Multiplication and division in 'C' shall not be performed using shift operators.
42. Bitwise operations shall only be performed on unsigned data.

### Control Flow

43. Test expressions should use explicit tests for pointers and integers, and not rely on a default comparison to zero.

```C
SYS_STATUS FunctionOne(int8_t var1)
{
    SYS_STATUS Status = SYS_STATUS_BAD;
    /* var1 is checked if it contains a value */
    if (var1 == 0)
    {
        printf("Variable is empty");
        Status = SYS_STATUS_GOOD;
    }
    return (Status);
}
```

44. With the exception of for statements, there should be no more than one statement per line.
45. A boolean value will only be `true` or `false`.
46. Switch statements should not be nested.
47. Switch constructs shall always contain a default branch that handles unexpected cases.
48. Each case shall end with a break statement unless the case label has no code or is explicitly noted with the comment /* fall-through */
49. In cases where multiple exits are necessary, the description in the function header should indicate that the function has more than one return statement.
50. A break should not be used to exit a loop unless it avoids using flags.
51. **The goto statement should not be used.**
52. **Setjmp() and longjmp() shall not be used.**

### Memory Use

53. Absolute addresses shall be used only for mapping entry points to hardware and processor registers. They should also be defined in only one file if possible, or at most one subsystem.
54. The use of memory management functions for allocating memory should not be used on a dynamic basis. Explicit Array declarations or one time memory allocation on initialization should be used instead.

### Comments

55. Code and comments shall be visually separate.
56. All comments shall be kept up to date with the code changes.
57. Comments that refer to blocks of code or data should be placed above the block of code or data and indented to the same level as the code to which they refer. When a comment refers to an individual line, it may be placed to the right on the same line as the code.
58. Comments for control flow shall be on separate lines from the code and lined up in the same column.
59. All NULL bodies for while and for statements shall be commented so that it is clear that the NULL body is intentional and not missing code.
60. The expression `#if 0 / #endif` may be used to comment out sections of code. It shall be used for **latent features only** and shall not be used to remove test or development code.
61. Comments shall be opened and closed within the same file. That is, comments may not be opened in one file and closed through the use of include files.

### General

62. The sizeof() operator shall be used to obtain the size of a data element / structure. Preferring to use sizeof() on variable names over types.
63. Code specific to an operating system or hardware shall be separated from code that is not system or hardware specific by using function calls. Vendor language extensions shall be isolated and surrounded with #ifdef's or not used.
64. Code that is used for debugging should be compiled with conditional assembly such as follows:

```C
    #ifdef DEBUG
        /* debugging code */
    #endif /* DEBUG */
```

65. If available, the assert() function should be used during development to detect internal errors.
66. A function should include related executable statements, so that its algorithm or statements collectively perform an operation. Avoid long and complex functions.
67. Language extensions not contained in the ANSI C standard shall not be allowed except that pragma-like qualifiers required to implement the language on embedded microcontrollers may be used if surrounded by #ifdefs.
68. A program shall never rely on data size or casts to truncate expressions to a specific number of bits, use explicit logical operators instead.
69. The use of #ifdefs shall not obscure the readability of the code.
70. Repo provided formatting tools should be used as frequent as possible.

### Style

#### Operator Spacing

71. The Primary operators "dot" (.) and "arrow" (->), shall not have spaces around them., e.g.:

```C
    Structure.Member
    StructurePointer->Member
```

72. The Primary operators "subscipt"([]), and parenthesis(()) should not have spaces around them., e.g.:

```C
    Array[Index] sizeof(TYPEDEF_NAME)
```

73. A space should be used on each side of parenthesis when they are used for grouping, e.g.:

```C
    Len = (End - Beg) * Len;
    for (ii = 0; ii < nn; ii++)
```

74. Unary operators are "logical not" (!), "bitwise not"(~), "increment" (++), "decrement"(--), "negative" (-), "cast"((type)), "indirection"(*), "address of" (&), and "sizeof" (sizeof). The unary operators shall be written with no space between them and their operands:

```C
    !BooleanVariable         ~IntegerVariable
    IntegerVariable++        --IntegerVariable
    *pPointer                &VariableOrFunction
    Minus = -Pos;            R2 = sqrt((double)Area);
```

75. All Arithmetic, bitwise, and logical operators should be written with one  or more spaces on each side of the operator. e.g:

```C
    Var1  = Var2;
    Var1 &= Var2;
    Var1  = Var2 + Var3;
```

76. Commas and semicolons shall have one space or newline after them and no space before them:

```C
    Function(Param1, Param2, Param3);
    for (Ii = 0; Ii < nn; Ii++);
```

#### Indentation

77. Spaces shall be used instead of tabs to provide for horizontal separation of code elements.
78. Each indentation level should be 4 spaces to the right of the enclosing level. In the case of grandfathered files already using something other than 4 spaces, the number of spaces should remain consistent within the file.

Below is an example of indentation within a function. The return type, function type, function name, and opening and closing braces shall be at the outermost level. Formal function parameter types shall be indented one level from the function name.

```C
uint8_t FunctionOne(int8_t Var1, uint8_t Var2, uint8_t *pLocPtr)
{
    SYS_STATUS Status = SYS_STATUS_BAD;

    /* Body of code indented one level from opening brace */
    if (Var1 == 0)
    {
        Var1 = Var2;
        pLocPtr = &Var2;
        Status = SYS_STATUS_OK;
    }
    return (Status);
}
```

79. To make long logical expressions easier to read, each clause should be placed on a separate indented line with the operators aligned on the right:

```C
if ((LongVar1 < LongVar2)     ||
    ((LongVar1 >= LongVar3)   &&
    (LongVar1 != 0)))
    {
    /* Code */
}
```

#### Braces

80. Braces should not be omitted for single line operations within the do/while/for/if/else/switch statements.
81. The following style of bracing should be used to enclose all do/while/for/if/else/switch statements, as well as struct/union/enum definitions

```C
if (aa > bb)
{
    /* Code */
}
else if (aa < bb)
{
    /* Code */
}
else
{
    /* Code */
}

do
{
    /* Code */
} while (aa < bb);

for (ii = 0; ii < COUNT_10; ii++)
{
    /* Code */
}

switch (Month)
{
case TM_JANUARY:
    GoSkiing();
    break;
case TM_JUNE:
    GoHiking();
    break;
default:
    Work();
    break;
}

struct
{
    uint8_t aa;
    bool bb;
} STRUCT1;

enum
{
    SYS_COUNT1,
    SYS_COUNT2,
    SYS_COUNT3
    SYS_COUNT_MAX;
} SYS_ENUM;
```

### Function and Macro Headers

82. Each function definition shall have a common header containing

* A high level description
* A list of inputs
* A list of outputs

#### Function and Macro Template

Below is shown for documentation purposes. There are copy/paste ready samples in `doc/templates`

```C
/**
 *  This description is a high level statement of what the function does,
 *  not how the function works.
 *
 *  @param $variable
 *      This section lists and describes the input arguments to
 *      the function.  Each Argument must have a @param described
 *      in this section.
 * 
 *  @param $variable
 *       Example of a pointer variable this read and written/written to.
 *
 *  @return
 *      This section lists and describes the return of the
 *      function.
 */
```

83. A Macro should have a minimum header describing the function of the macro, the inputs, and the outputs.

### File Layout

#### Common File Headers

84. All File (Module)  headers shall contain the following;

* A copyright notice and legal disclaimer
* The filename
* Description

### Source Files

85. The contents of all C source files shall follow this order:

* File Header
* Standard #include files with <name.h> convention
* Specific #include files with "name.h" convention
* Project Specific #include files ("name"_project.h)
* Private #include files ("name"_i.h) (if a Private header file is included, the following rules are not required in source file)
* Symbolic constants and macro definitions
* Type definitions
* Prototypes for function used within the scope of the file
* Global Variable definitions
* Function definitions

86. Each source implementation file shall list its #include files at the head of the file before any declarations or function definitions.
87. A .c file shall not have function prototypes for non-static functions.
88. A .c file shall not have externs for global data items.

#### Source File Template

```C
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
* @file
* This section is used to describe the file and any relevant
* information related to it purpose and how it should be used.
*
*/

/*------------- Includes -----------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
```

Refer to Section Source Files for example.

### Include files

89. The contents of all C include files shall be arranged as follows

* File header
* Wrapper preventing multiple inclusions
* Nested includes
* Symbolic constant and macro definitions
* Type definitions
* Extern variable definitions
* Function prototypes
* End of wrapper

90. Each include file should use a method to prevent repeated inclusion similar to the following example

```C
#pragma once
/* Place content of file here */
```

91. A .h file shall not have static function prototypes.
92. A .h file shall not have static data items.
93. All export variables shall be explicitly declared in the public header file.
94. Global header files should not include any private module definitions including function prototypes,  symbolic constants, macros, type definitions, or variables.

#### Include File Template

See templates here as well: [Templates](../../templates/).

```C
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * This section is used to describe the file and any relevant
 * information related to it purpose and how it should be used.
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
```

Refer to project specific examples or templates as needed.

## Naming Conventions

95. A consistent naming convention shall be used across each project.

### Filenames

96. The following filename suffixes should be used:

```
        .c            C source files
        _project.c    Project specific source file (Example fsvc_kingsgate.c)
        .h            C include files
        _i.h          Private C Include Files
        .o            Object files  (filename compiler dependent)
```

97. If a common shared file between projects becomes too encumbered (subjective definition) with project specific dependencies, then the project specific differences shall be moved to a separate project source file.
98. Common shared files shall follow standard naming convention.
99. Project specific files derived from a shared file shall have a _<project> appended to the common shared name.
100. Unique files (files only used in one project) shall follow standard naming convention.

```
    Example:
        Common files:  router.c, router.h, router_i.h
        Derived project files: router_raven.c, router_raven.h, router_raven_i.h
        New unique project files: sonetrouter.c, sonetrouter.h, sonetrouter_i.h
```

#### Variable Names

101. Identifiers can be given any number of characters.
102. Single letter identifiers (like I and j) should be avoided because it is difficult to perform global searches for them.
103. The first word of an identifier should start with a upper case character (e.g. VarName). TODO: Do we want to use Pascal Case?
104. Identifiers composed of more than one word are simply run together using camel case. This applies even to acronyms. For example:

```C
    char HubId[] = " C. Manson";
    int16_t IndexNum;
    uint8_t UartReg;
```

105. Global identifiers or identifiers shared between tasks should carry a subsystem identifier as a prefix. For Example:

```C
    SbQueIndex
    EtStack
    MgFanStatus
```

106. For shared variables, the subsystem that declares the value should determine the prefix used.
107. All pointer variables (besides arrays) shall be prepended with lower case p.
108. All global variables (variables used outside the scope of a module) shall be prepended with a g_.
109. All static variables shall prepend a s_ to the name. Example:

```C
static uint8_t s_VersionNum;
static char *s_pUnitName;
```

#### Function Names

110. Function names shall follow the same conventions as variable names.
111. Function names shall not conflict with the names in the standard library.

#### Constants and Macros

112. All symbolic constants (those defined with the preprocessor directive #define, const, or enum) shall be composed of only uppercase characters.
113. Macros should use the same naming conventions as  functions.
114. Typedef and structure names shall be composed of only uppercase characters.
115. Words composing a typedef or structure name shall be composed of only uppercase characters.

Example:
```C
typedef struct _CMD_PKT
{
    int32 *pId;
    int32 *pCrc;
} CMD_PKT;
```

116. Module specific constants shall have the module prefix prepended to the constant(s).

Example:
```C
#define SYS_MAX_FRAME 256
#define SYS_MAX_TIMEOUT 10
```

117. Module / subsystem prefix shall be prepended to the module specific typedef names.
118. Module / subsystem prefix shall be prepended to the module specific enumerated declarations.
119. Module / subsystem prefix shall be prepended to the preprocessor defines.
120. Enumerated typedef names shall follow the module / subsystem identifier.

Example:
```C
typedef enum
{
    SYS_NUMBER,
    SYS_CONST,
    SYS_STRING,
    SYS_MAX
} SYS_NAME_TYPE;
```

121. The suffix 'U' or 'L' shall be appended to any define equal to or greater than 16 bits.

Example:
```C
#define SYS_CONTROL_REG 0x19000014U
```

# Examples

## Source Files

The following is an example of a Source code file:

```C
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * This file contains all of the functional
 * calls for initializing the platform hardware.
 */

/*------------- Includes -----------------*/
#include <timers.h>
#include <stdLib.h>  
#include <lstlib.h>
#include <taskLib.h>
#include <tickLib.h>
#include <logLib.h>

#include "machdeps.h"
#include "sys.h"
#include "timer_api_i.h"

/*-- Symbolic Constant Macros (defines) --*/
#define TIMER_IDENT          0x524D4954U

/* Timer type */
#define SINGLE_SHOT_TIMER    0x11111111U
#define PERIODIC_TIMER       0x22222222U

/*------------- Typedefs -----------------*/

// User Task Desciptor
typedef struct _TIMER_TASK
{
    NODE     TaskNodeLink;         /* Link Task nodes */
    int32_t  TaskId;               /* initiator task id */
    uint32_t TaskDeltaTick;        /* delta timer tick 1ms */
    uint32_t TaskLastSystemTick;   /* last system tick */
    LIST     TaskTimerEventList;   /* Single Shot Timer */
    LIST     TaskExpireTimerList;  /* nodes that have expired */
};

// Timer Event Descriptor
typedef struct _TIMER_EVENT
{
    NODE          TimerNodeLink;      /* Link Timer nodes */
    uint32_t      TimerId;            /* timer ID "TIMR" */
    uint32_t      TimerType;          /* Timer state wait or expired */
    TIMER_TASK    *pTimerTaskPtr;     /* Pointer to task table */
    uint32_t      TimerState;         /* Timer state wait or expired */
    void (*TimerRoutine)(char *);     /* Re-entrant function pointer */
    char *        TimerParameter;     /* program parameter */
    uint32_t      TimerRequestedTime; /* Initial timer period */
    uint32_t      TimerCycle;         /* Maximum number of retries */
}TIMER_EVENT;

typedef enum
{
    TS_WAIT_TO_EXPIRE   = 0,
    TS_EXPIRED          = 1,
    TS_SCHEDULED        = 2,
    TS_STOPPED_DELETE   = 6,
    TS_FREE             = 8
} TIMER_STATE;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

/**
 * This routine will start a periodic timer. A cycle count of 0 is
 * treated the same as a cycle count of 1.
 *
 *  @param TaskNode      Task table
 *  @param TimeReq       Timer period (ms)
 *  @param CycleCnt      Number of timer event
 *  @param ParamPtr      Parameters for callback function
 *  @param (*Routine)(char *)) Callback funciton
 *
 *  @return
 *     NO_FREE_TIMERS (-1)      Out of memory
 *     *pTimerNode              Pointer to user timer Id
 */

static uint32_t InsertTimer (
                        _TIMER_TASK *pTaskNode,
                        uint32_t TimeReq,
                        uint32_t CycleCnt,
                        char *pParam,
                        void (*Routine)(char *))
{
    UNUSED(pTaskNode);
    TIMER_EVENT *pTimerNode;
    uint32_t RetStatus;

    // allocate a user timer block 
    if ((TimerNode = TimerMemPool())= NULL)
    {
        // timer event table alloc failed
        RetStatus = NO_FREE_TIMERS;
    }
    else
    {
        // store timer ID into user timer block
        TimerNode->TimerId = TIMER_IDENT;
        TimerNode->TimerType = SINGLE_SHOT_TIMER;
        TimerNode->TimerTaskPtr = TaskNode ;
        TimerNode->TimerState = TS_WAIT_TO_EXPIRE;
        TimerNode->TimerRoutine = Routine;
        TimerNode->TimerParameter = ParamPtr;
        TimerNode->TimerRequestedTime = TimeReq;
        TimerNode->TimerCountdownTime = TimeReq;
        TimerNode->TimerCycle = CycleCnt;


        // insert into single timer with one physical timer event
        LstAdd (&TaskNode->TaskTimerEventList, &TimerNode->TimerNodeLink );
        RetStatus = (uint32_t) TimerNode;
    }

    // return user timer Id (address of user data pointer) or error
    return (RetStatus);
}
```

## Header file

```C
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * This is the public header file for the timer API.
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/
/* Timer status codes */
#define TM_NO_FREE_TIMERS  (uint32_t) 0xFFFFFFFFU
#define TM_SCHEDULED    2    /* Single-shot timer */
#define TM_PERIOD       4    /* Periodic timer */
#define TM_ERROR        8    /* Invalid timer */

/* Timer constants */
#define TM_1_MSECOND    1    /* System tick resolution*/
#define TM_10_MSECOND   10
#define TM_100_MSECOND  100
#define TM_500_MSECOND  500
#define TM_1_SECOND     1000
#define TM_2_SECOND     2000
#define TM_4_SECOND     4000
#define TM_5_SECOND     5000
#define TM_10_SECOND    10000
#define TM_1_MINUTE     60000U
#define TM_5_MINUTE     300000U
#define TM_FOREVER      0xFFFFFFFFU

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/* Single-shot timer */
uint32_t TmTimerStart (uint32_t TimeReq, char * ParamPtr,
  void (*Routine)(char *));

/* Stop and delete an active timer */
uint32_t TmTimerStop (uint32_t TimerIndex);

/* Adjust period of active timer */
uint32_t TmAdjustTimer(uint32_t TimerIndex, uint32_t TimeReq);

/* Return timer type- singleshot or periodic */
uint32_t TmTimerStatus (uint32_t TimerIndex);

/* Timer tick routine */
void TmTimerTickUpdate (void);
```

# Sample sheet

```C
// Enumerations
// Inter-module or Inter-subsystem (Public)
typedef enum
{
    SYS_RED_ONE,
    SYS_BLUE_ONE,
    SYS_BLACK_ONE,
    SYS_MAX_ONE
}SYS_EXAMPLE_DEMO_ONE;

// Module or subsystem specific (Private)
typedef enum
{
    SYS_RED_TWO,
    SYS_BLUE_TWO,
    SYS_BLACK_TWO,
    SYS_MAX_TWO
} SYS_EXAMPLE_DEMO_TWO;

// Module (Static)
typedef enum
{
    RED_ONE,
    BLUE_ONE,
    BLACK_ONE,
    MAX_ONE
} EXAMPLE_DEMO_ONE;

// Structures
// Inter-module or Inter-subsystem (Public)
typedef struct
{
    int8_t RedOne;
    int16_t BlueOne;
    uint8_t BlackOne;
} SYS_EXAMPLE_DEMO;

//Module or subsystem specific (Private)
typedef struct
{
    int8_t RedOne;
    int16_t BlueOne;
    uint8_t BlackOne;
} SYS_EXAMPLE_DEMO;

// Module (Static)
typedef struct
{
    int8_t RedOne;
    int16_t BlueOne;
    uint8_t BlackOne;
} SYS_EXAMPLE_DEMO;

// Unions
// Inter-module or Inter-subsystem (Public)
typedef union
{
    int8_t redOne;
    int16_t blueOne;
    uint8_t blackOne;
} SYS_EXAMPLE_DEMO;

// Module or subsystem specific (Private)
typedef union
{
    int8_t redOne;
    int16_t blueOne;
    uint8_t blackOne;
} SYS_EXAMPLE_DEMO;

// Module (Static)
typedef union
{
    int8_t redOne;
    int16_t blueOne;
    uint8_t blackOne;
} EXAMPLE_DEMO;

// Constants
// Inter-module or Inter-subsystem (Public)
#define    SYS_EXAMPLE_ONE

// Module or subsystem specific (Private)
#define    SYS_EXAMPLE_ONE

// Module (Static)
#define    EXAMPLE_ONE

// Variables (Globals - All globals are contained in structures)
// Inter-module or subsystem (Public)
struct sys_exampleSample    /* Note, can not contain private typedefs */
{
    SYS_EXAMPLE_DEMO  ExampleOne;    /* Enumeration type */
    SYS_EXAMPLE_DEMO  ExampleTwo;    /* Structure type */
    SYS_EXAMPLE_DEMO  ExampleThree;  /* Union type */
};

// Module or subsystem specific (Private)
struct sys_exampleSample
{
    SYS_EXAMPLE_DEMO  exampleOne;    /* Public enumeration */
    SYS_EXAMPLE_DEMO  exampleTwo;    /* Private structure */
    SYS_EXAMPLE_DEMO  exampleThree;  /* Public union */
};


// Module (Static)
static struct exampleSample
{
    SYS_EXAMPLE_DEMO  exampleOne;    /* Public enumeration */
    SYS_EXAMPLE_DEMO  exampleTwo;    /* Private structure */
    SYS_EXAMPLE_DEMO  exampleThree;  /* Public union */
};

// Macros and Functions
// Inter-module or subsystem (externs)
int8_t SysExampleOne(void);

// Module or subsystem specific (Private)
int8_t SysExampleOne(void);

// Module (static)
static int8_t ExampleOne(void);
```
