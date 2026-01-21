# Steven Sun, 100816207 - Operating Systems Lab 2
# Operating Systems Lab 2 TA: Chenyu NING
# Makefile for myshell

myshell: myshell.c utility.c myshell.h
	gcc -Wall myshell.c utility.c -o myshellexecutable