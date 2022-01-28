PRGNAME     = bmp2huc.elf
CC			= gcc

SRCDIR		= ./
VPATH		= $(SRCDIR)
SRC_C		= $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
OBJ_C		= $(notdir $(patsubst %.c, %.o, $(SRC_C)))
OBJS		= $(OBJ_C)

CFLAGS		= -O0 -g3 -Wall -Wextra

LDFLAGS     = -nodefaultlibs -lc -lgcc -lm -lSDL -lSDL_image -Wl,--as-needed -Wl,--gc-sections -flto

# Rules to make executable
$(PRGNAME): $(OBJS)  
	$(CC) $(CFLAGS) -o $(PRGNAME) $^ $(LDFLAGS)

$(OBJ_C) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(PRGNAME) *.o
