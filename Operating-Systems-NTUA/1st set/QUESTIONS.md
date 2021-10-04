1.1	Connect to an object file
This exercise asks you to create an executable file (named zing) that will call the zing () function. This function is declared in the zing.h file. Additionally, the object file zing.o is available. These files are in the / home / oslab / code / zing directory and should be copied to your work directory.
Steps:
1. Copy zing.h and zing.o files to your work directory.
2. Create a main.o object file for the main () function.
3. Linking the two object files.
Questions:
1. What is the purpose of the heading?
2. A suitable Makefile is required to create the executable of the exercise.
3. Generate your own zing2.o, which will contain zing () that displays different message but similar to he zing () of zing.o. Consult the manual page of getlogin (3). Change Makefile to produce two executables, one with zing.o, one with zing2.o, reusing the common object file main.o.
4. Suppose you have written your program in a file containing 500 functions. You are currently making changes to only one function. The work cycle are: changes to the code, compilation, execution, changes to the code, and so on. Compilation time is long, which delays you. How can this problem be addressed?
5. Your co-worker and you worked in the foo.c program all the previous week. While you were taking a break and your partner was working in the code. You hear a desperate cry. You ask what happened and he tells you that the foo.c file has been lost! You look at the history of the shell and the last command was gcc ‐Wall ‐o foo.c foo.c. What happened? hint # 1: Because errors are human, we recommend that you back up your data and use Makefile.

Merge two files into a third 
This exercise requires a program to create a file (output file). The contents of the output file will be merged by merging the contents of two input files. The program (fconc) will accept two or three arguments.
Specifically:
-If the program is called without the appropriate arguments, a help message will be displayed.
-The first and second arguments are the input files.
-The third argument is optional and is the output file.
-The default value for the output file is fconc.out
-If one of the input files does not exist, the program should display an appropriate error message.
Examples of execution:

$ ./fconc A

Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)]

$ ./fconc A B

A: No such file or directory

$ echo 'Goodbye,' > A

$ echo 'and thanks for all the fish!' > B

$ ./fconc A B

$ cat fconc.out

Goodbye,
and thanks for all the fish!

$ ./fconc A B C

$ cat C

Goodbye,

and thanks for all the fish!

Proposed implementation skeleton (functions)

-void doWrite (int fd, const char * buff, int len)
Function that undertakes the registration in the fd file descriptor.
-void write_file (int fd, const char * infile)
Function that writes the contents of an archive file name to the descriptor fd file. Uses doWrite ().

Questions:
Run an example of fconc using the strace command. Copy the output part of the strace that results from the code you wrote.

