CC = gcc -lncurses -std=gnu99

# Executable Name Replace With Your Own
EXE = smallsh

SRCDIR = .
SRCEXT = c
SOURCES = $(shell find $(SRCDIR) -type f -name "*.$(SRCEXT)")
OBJECTS = $(SOURCES:.c=.o)
DEP = $(OBJECTS:.o=.d)

$(EXE): $(OBJECTS)
	$(CC) $^ -o $(EXE)

-include $(DEP)

%.d: %.$(SRCEXT)
	$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.$(SRCEXT)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.out *.o *.d $(EXE)

.PHONY: clean