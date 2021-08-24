## Multi-Threaded Producer Consumer Pipeline
#####  An exploration in multi-thread programming in C

#### --- ABOUT ---
This program was part of my operating system studies at Oregon State University. 
I learned how to utilize the pthread.h library to create a basic multi-threaded
program.  

The program creates 4 threads to process input from stdin, each with their own 
functionality. Thread 1 reads in the lines of characters from stdin and passes 
the data to thread 2. Thread 2 replaces every newline character with a space 
character and passes the data to thread 3. Thread 3 replaces every pair of plus 
signs ('++') with a caret ('^') symbol and passes the data to thread 4. Thread 4 
writes the processed data to the stdout in lines of exactly 80 characters.  

Additionaly, if the program encounters the phrase 'STOP' immediately followed 
by a newline character, any further input will be ignored. Another important 
note is that if there are less than 80 characters in the thread 4 buffer when 
the input stream is finished, the processed data will not by outputted to stdout.  

#### --- SYSTEM REQUIREMENTS ---
The program runs on Linux-based operating systems.

#### --- COMPILING INSTRUCTIONS ---
The program can be compiled with the gcc compiler using the command:  
`gcc main.c -o mtpcp`  

#### --- FILES ---
There is only one file in this program: main.c  

This file is compiled into a 'mtpcp' executable , which is then run to use 
the program.  

#### --- HOW TO USE --- 
There are 2 ways to use this program. The first is to execute the program and 
then enter in your desired input text into the terminal/shell.  

The second method is to redirect the contents of a text file to the program. EXAMPLE:  
`./mtpcp < input.txt`  
This command will feed the contents of 'input.txt' to the mtpcp program.  
