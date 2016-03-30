objs = tlv.o tlv_common.o tx_sm.o lldp_linux_framer.o \
		lldp_debug.o common_func.o lldp_neighbor.o

CCFLAGS= -g -Wall

all:main.o $(objs)
	gcc -o a.out -g main.o $(objs)

clean:
	rm *.o a.out
