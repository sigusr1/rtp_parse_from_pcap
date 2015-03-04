#
# Makefile for libnids samples.
#
# Dug Song <dugsong@monkey.org>

srcdir		= .


CC		= gcc
CFLAGS		= -g -O2 -D_BSD_SOURCE -DENABLE_TCPREASM -DENABLE_CHECKSUM
LDFLAGS		= 

PCAP_CFLAGS	= -I/home/huo_jianying/nfs1/network/libpcap-master -I/home/huo_jianying/nfs1/network/libpcap-master/bpf
PCAPLIB		= -L/home/huo_jianying/nfs1/network/libpcap-master -lpcap

LNET_CFLAGS	= 
LNETLIB		= 

LIBS_CFLAGS	= -I../src $(PCAP_CFLAGS) $(LNET_CFLAGS)
LIBS		= -L../src -lnids $(PCAPLIB) $(LNETLIB) -lnsl 

.c.o:
	$(CC) -c $(CFLAGS) -I. $(LIBS_CFLAGS) $<

all: overflows printall sniff
static shared: all

overflows: overflows.o
	$(CC) -o $@ overflows.o $(LDFLAGS) $(LIBS)

printall: printall.o
	$(CC) -o $@ printall.o $(LDFLAGS) $(LIBS)

sniff: sniff.o
	$(CC) -o $@ sniff.o $(LDFLAGS) $(LIBS)

static shared install installshared:
	@true

clean:
	rm -f *.o *~ overflows printall sniff

# EOF
