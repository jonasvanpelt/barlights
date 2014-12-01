PICOTCP =../picotcp
PICOTCP_BUILD = .

#TODO only build necessary modules
#PICOTCP_OPTIONS="HTTP_SERVER=1"

CC=gcc 
INCLUDE_PATHS = -I. -I$(PICOTCP_BUILD)/ -I$(PICOTCP_BUILD)/build/include -I$(PICOTCP_BUILD)/build/modules
LIBRARY_PATHS = -L$(PICOTCP_BUILD)/build/lib/
LIBRARIES = -lpicotcp
OBJ = main.o pico_http_server.o pico_http_util.o www_files.o

CFLAGS= $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(LIBRARIES)
CFLAGS += -ggdb -Wall -Wextra
.PHONY: clean

all: barlights 


%.o: %.c $(PICOTCP_BUILD)/build/lib/libpicotcp.a
	$(CC) -c -o $@ $< $(CFLAGS)

barlights: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)
	
$(PICOTCP_BUILD)/build/lib/libpicotcp.a:
	@make -C $(PICOTCP) posix 
	@make -C $(PICOTCP)
			
tun:
	sudo ifconfig tun0 192.168.2.10/24
        
clean:
	rm -f barlights *.o
	rm -f -R build
	