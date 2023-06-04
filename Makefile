LINK_TARGET = \
	client \
	server1

OBJS = \
	client.o \
	server1.o

REBUILDABLES = $(OBJS) $(LINK_TARGET)

all : $(LINK_TARGET)

clean: 
	rm -f $(REBUILDABLES)

client : client.o
	cc -g3 -o  $@ $< -lpthread

server : server.o
	cc -g3 -o  $@ $< -lpthread

%.o : %.c
	cc -g3 -Wall -Werror -o $@ -c $<


