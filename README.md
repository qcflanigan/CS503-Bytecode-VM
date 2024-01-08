## Project Description:

    This project reflects a complete interpreter for the Lox programming language. The interpreter is written in the C programming language and follows Robert Nystrom's C implementation in his textbook 'Crafting Interpreters'. This implementation is built on the foundation of a Bytecode Virtual Machine, essentially forming a bytecode compiler, to interpret the Lox input rather than the Abstract Syntax Tree (AST) implementation. The bytecode machine allows for full implementation of any Lox program, including support for variable declaration, loops, classes, superclasses and inheritance. 

## Usage:

    -Extract the Bytecode Zip file into your directory of choice. 
    
    -Open the directory containing all of the provided folders (includeFiles, sourceCodeFiles, bin, buildFiles, Testing) in your IDE of choice.

    -There will already be a compiled executable of the project, `bytecodeVM.exe`, in the provided zip file. If you would like to generate a new executable, run the command "make" to use the provided Makefile to compile the project. 

    -The command "make clean" can also reset the built files in the buildFiles directory and remove the most recent executable. "make" will then generate a new version of all of the necessary files. 

    -Execute the command "./bytecodeVM.exe <lox_file_name>" to interpret an entire file, or just "./bytecodeVM.exe" to generate a REPL prompt which will interpret Lox code one line at a time. 

## Testing:

    This project uses Robert Nystrom's unit tests (https://github.com/munificent/craftinginterpreters/tree/master/test), included in the Testing/test directory, to test the interpreter for various Lox program cases. Run the command "./bytecodeVM.exe run test" within your IDE terminal to generate and run each test file. The output of each file will be printed in the terminal, all of which have been verified to be correct (262/262). Many of the tests produce errors, all of which are the expected outcome of the test file to ensure the Lox Interpreter does not allow for any parsing errors. Each individual test file describes the expected output, including each expected error. These expected outcomes can be verified by following the path Testing/test/<directory_name>/<file_name> to each file. This path (with each file's name) is printed with its corresponding output in the terminal upon each file's execution. 

    **
    There is also a collection of images verifying the functionality of the program in the Testing/"Testing Screenshots - Final Program" directory. 



