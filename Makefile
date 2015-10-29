CC = gcc
CFLAGS = -Wall -m32 -g -pg -std=gnu99
C = $(CC) $(CFLAGS)

rubi: engine.o codegen.o
	$(C) -o $@ $^

minilua: dynasm/minilua.c
	$(CC) -Wall -std=gnu99 -O2 -o $@ $< -lm

engine.o: engine.c rubi.h
	$(C) -o $@ -c engine.c

codegen.o: parser.h parser.dasc expr.dasc stdlib.dasc minilua
	cat parser.dasc expr.dasc stdlib.dasc | ./minilua dynasm/dynasm.lua -o codegen.c -
	$(C) -o $@ -c codegen.c

run:rubi
	# for pi
	@echo "echo 1 > /proc/sys/vm/drop_caches" | sudo sh    
	@./rubi ./progs/pi.rb
	@ gprof rubi gmon.out -p -b >> analysis_pi_ins.txt
	# for primetable
	@echo "echo 1 > /proc/sys/vm/drop_caches" | sudo sh    
	@./rubi ./progs/primetable.rb
	@ gprof rubi gmon.out -p -b >> analysis_primetable_ins.txt
	# for fib
	@echo "echo 1 > /proc/sys/vm/drop_caches" | sudo sh    
	@./rubi ./progs/fib.rb
	@ gprof rubi gmon.out -p -b >> analysis_fib_ins.txt
	# For Calculate the cache misses for rubi on pi
	@perf stat -r 5 -e cache-misses,cache-references,L1-dcache-load-misses,L1-dcache-store-misses,L1-dcache-prefetch-misses,L1-icache-load-misses ./rubi ./progs/pi.rb
	# For Calculate the cache misses for rubi on primetable
	@perf stat -r 5 -e cache-misses,cache-references,L1-dcache-load-misses,L1-dcache-store-misses,L1-dcache-prefetch-misses,L1-icache-load-misses ./rubi ./progs/primetable.rb
	# For Calculate the cache misses for rubi on fib
	@perf stat -r 5 -e cache-misses,cache-references,L1-dcache-load-misses,L1-dcache-store-misses,L1-dcache-prefetch-misses,L1-icache-load-misses ./rubi ./progs/fib.rb
clean:
	$(RM) a.out rubi minilua *.o *~ text *.txt codegen.c
