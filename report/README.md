# hw4 report

|||
|-:|:-|
|Name|洪慎廷|
|ID|110550162|

## How much time did you spend on this project

> e.g. 2 hours.

about 20 hours

## Project overview

> Please describe the structure of your code and the ideas behind your implementation in an organized way.
> The point is to show us how you deal with the problems. It is not necessary to write a lot of words or paste all of your code here.

In this project, I mostly modified the "SemanticAnalyzer.hpp" and  "SemanticAnalyzer.cpp" for doing the semantic analysis, added some codes in the AST nodes to retrieve some information for semantic analysis, and also added some codes in the scanner and parser to pass on the input P language code for reporting the errors.

In "SemanticAnalyzer.hpp", I defined the datastructures of the Symbol entry and the Symbol table, and implement the operations such as "insert", "lookup" and "print" the symbol table. Besides, I also implement the datastructures for information propagation. When a child's or parent's node information needs to be propagate, the information will be stored in the structure and push into a stack, and will be popped when it is needed.

In "SemanticAnalyzer.cpp", just only implement the functions provided by using the tools in "SemanticAnalyzer.hpp". The codes are mostly just doing the construction and deconstruction of the Symbol table, type checking and report errors according to the given rules.

## What is the hardest you think in this project

> Not required, but bonus point may be given.

Think and implement the method of information propagation, because different node needs different kind of information for semantic analysis, some information are from the parents' node, and some are from the childs'. 

## Feedback to T.A.s

> Not required, but bonus point may be given.

It seems that if the program meets segmantation fault, the result of the test cases won't print out anything and the code will be extremely hard to debug.