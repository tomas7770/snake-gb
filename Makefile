LCC = lcc 

# You can uncomment the line below to turn on debug output
# LCC = $(LCC) -debug

# You can set the name of the .gb ROM file here
PROJECTNAME    = snake

BINS	    = $(PROJECTNAME).gb
CSOURCES   := $(wildcard *.c)
CSOURCES   := $(filter-out savedata.c, $(CSOURCES))
ASMSOURCES := $(wildcard *.s)

all:	$(BINS)

#compile.bat: Makefile
#	@echo "REM Automatically generated from Makefile" > compile.bat
#	@make -sn | sed y/\\//\\\\/ | grep -v make >> compile.bat

# Compile and link all source files
$(BINS):	$(CSOURCES) $(ASMSOURCES)
	$(LCC) -Wf-ba0 -c -o savedata.o savedata.c
	$(LCC) -Wl-yt3 -Wl-ya1 -o $@ savedata.o $(CSOURCES) $(ASMSOURCES)

clean:
	rm -f *.o *.lst *.map *.gb *.ihx *.sym *.cdb *.adb *.asm

