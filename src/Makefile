#-------------------------------------------------------------------------------
#-- Makefile
#-- requires environment variables WT_HOME and LD_LIBRARY_PATH are set,
#-- where LD_LIBRARY_PATH=$WT_HOME/build_posix/.libs
#-------------------------------------------------------------------------------


#-- macros ---------------------------------------------------------------------
INCPATH  = -I. -I$(WT_HOME)/src/include -I$(WT_HOME)/build_posix -Ithird_party
OBJDIR  := obj

CPP      = g++
CPPFLAGS = $(INCPATH) -Wall -m64 -std=c++14 -pipe -Wno-unused


#-- executables ----------------------------------------------------------------

TARGET = wiredindex


#-- link libraries -------------------------------------------------------------

LD      = $(CPP)
LDLIBS  = -lwiredtiger -lbsoncxx
LDFLAGS = -L../libs -L$(WT_HOME)/build_posix/.libs


#-- build flags ----------------------------------------------------------------

DEBUG ?= 0

ifeq ($(DEBUG),1)
LDFLAGS  += -g
CPPFLAGS += -g -O0 -DDEBUG
else
CPPFLAGS += -g -O3 -DNDEBUG
endif


#-- goal-specific settings -----------------------------------------------------

ifndef MAKECMDGOALS
endif


#-- sources --------------------------------------------------------------------

SRCDIRS = .


#-- macros ---------------------------------------------------------------------

SRCS := $(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.cpp))
OBJS := $(patsubst %.cpp,$(OBJDIR)/%.o,$(notdir $(SRCS)))
DEPS := $(patsubst %.o,%.d,$(OBJS))


#-- rules ----------------------------------------------------------------------
#-- the macros $@ and $^ are the left and right sides of the : -----------------

vpath %.cpp $(SRCDIRS)

$(OBJDIR)/%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@

$(OBJDIR)/%.d: %.cpp | makedir
	@echo generating dependency file $@
	@$(CPP) -MM -MG $(CPPFLAGS) $< | sed -e 's,^\([^:]*\)\.o[ ]*:,$(@D)/\1.o $(@D)/\1.d:,' > $@


#-- targets --------------------------------------------------------------------

.PHONY: clean all

all: $(TARGET)

$(TARGET): $(OBJS)
	$(LD) $^ -o $@ $(LDFLAGS) $(LDLIBS)

makedir: $(OBJDIR)

$(OBJDIR):
	mkdir -p $@

clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGET)


#-- deps -----------------------------------------------------------------------

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif


#-------------------------------------------------------------------------------
