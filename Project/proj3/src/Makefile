CC := gcc
CFLAGS := -Wall -O0 -mavx512f -fopenmp -mavx512dq -mavx512cd -mavx512bw -mavx512vl -mavx512vbmi -mavx512ifma  -mavx512vbmi2 

SRCS := $(wildcard *.c)
OBJS := $(SRCS:%.c=%.o)

.PHONY: all clean

all:: $(OBJS)

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	
clean::
	rm -f *.o