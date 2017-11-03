exe:
  gcc -x c shell.c -O3 -lreadline -o ./shell
  
debug:
  gcc -x c shell.c -O3 -lreadline -o ./shell -g
