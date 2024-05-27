# CC = gcc
# CXX = g++
# ECHO = echo
# RM = rm -f

# TERM = "S2019"

# CFLAGS = -Wall -Werror -ggdb -funroll-loops -DTERM=$(TERM)
# CXXFLAGS = -Wall -Werror -ggdb -funroll-loops -DTERM=$(TERM)

# LDFLAGS = 

# BIN = poke327
# OBJS = poke327.o heap.o

# all: $(BIN) etags

# $(BIN): $(OBJS)
# 	@$(ECHO) Linking $@
# 	@$(CC) $^ -o $@ $(LDFLAGS)

# -include $(OBJS:.o=.d)

# %.o: %.c
# 	@$(ECHO) Compiling $<
# 	@$(CC) $(CFLAGS) -MMD -MF $*.d -c $<

# %.o: %.cpp
# 	@$(ECHO) Compiling $<
# 	@$(CXX) $(CXXFLAGS) -MMD -MF $*.d -c $<

# .PHONY: all clean clobber etags

# clean:
# 	@$(ECHO) Removing all generated files
# 	@$(RM) *.o $(BIN) *.d TAGS core vgcore.* gmon.out

# clobber: clean
# 	@$(ECHO) Removing backup files
# 	@$(RM) *~ \#* *pgm

# etags:
# 	@$(ECHO) Updating TAGS
# 	@etags *.[ch]



all: poke327

poke327: poke327.o heap.o
	gcc -lm poke327.o heap.o -o poke327

poke327.o: poke327.c heap.h
	gcc -Wall -Werror -ggdb -lm poke327.c -c

heap.o: heap.c heap.h
	gcc -Wall -Werror -ggdb -lm heap.c -c

clean:
	rm poke327 poke327.o heap.o