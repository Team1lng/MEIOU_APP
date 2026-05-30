clear;
rm Test -f
gcc -o Test main.c LogModule.c CmdModule.c DebugModule.c -lpthread
# gcc RShell.c -o RShell