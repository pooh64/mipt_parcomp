mkdir -p build
gcc -O3 -g life_core.c life_gui.c single_threaded.c -lcurses -D LIFE_ENABLE_GUI -o ./build/single_gui
gcc -O3 -g life_core.c life_gui.c single_threaded.c -lcurses -D LIFE_ENABLE_GUI -D EXPLODER -o ./build/exploder
gcc -O3 -g life_core.c life_gui.c single_threaded.c -lcurses -o ./build/single_perf
mpicc -O3 -g life_core.c life_gui.c mpi_worker.c -lcurses -o ./build/mpi_worker
gcc -O3 -g life_core.c life_gui.c gui_server.c -lcurses -o ./build/gui_server
