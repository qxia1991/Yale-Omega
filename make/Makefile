# Very basic makefile
CXXFLAGS = -fPIC -g  -O2 -W -Wall -Wextra -Wshadow 
SOMAKER = g++
SOFLAGS = -shared 
SOSUFFIX = so 

ROOT_INCLUDE_FLAGS := $(shell root-config --cflags) 
ROOT_LIB_FLAGS := $(shell root-config --libs) -lMinuit
ROOTCINT := $(shell root-config --bindir)/rootcint
ROOT := $(shell root-config --bindir)/root

EXO_INCLUDE_FLAGS := $(shell exo-config --cflags)
EXO_LIB_FLAGS := $(shell exo-config --libs) -lEXOCalibUtilities -lEXOUtilities



# --- Directories -------------------------------------------------------------------
SRCDIR := ../src
BINDIR := ../bin
BUILDDIR := ../build
INCDIR := ../EXOEnergy
LIBDIR := ../lib
LINKDEFIN := $(wildcard $(INCDIR)/*LinkDef.h.in)
APPDIR := ../app

LIBNAME := EXOEnergy

# Make dependencies
MAKEDEPEND = rm -f $@; \
	$(CXX) $(CCFLAGS) -MM $(INCLUDEFLAGS) $< > $@.$$$$;                           \
        cp $@.$$$$ $@.temp.$$$$; sed -e 's/\#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
        -e '/^$$/ d' -e 's/$$/ :/' < $@.$$$$ >> $@.temp.$$$$;                         \
	sed 's,\($*\)\.o[ :]*,$(BUILDDIR)/\1.o $@ : ,g' < $@.temp.$$$$ > $@ ;         \
        rm -f $@*.$$$$;


EXECUTABLES = 
SHLIB := $(LIBDIR)/lib$(LIBNAME).$(SOSUFFIX)
AUTOLINKDEF := $(BUILDDIR)/LinkDef_$(LIBNAME).h
PYTHON := python
ROOTSOSUFFIX := so
ROOTDICTCCNAME := $(BUILDDIR)/$(LIBNAME)DICT.C
ROOTDICTHHNAME := $(BUILDDIR)/$(LIBNAME)DICT.h
ROOTDICTOBJECT := $(ROOTDICTCCNAME:.C=.o)
INCLUDEFLAGS = $(ROOT_INCLUDE_FLAGS) $(EXO_INCLUDE_FLAGS) -I$(INCDIR)
LIBFLAGS = $(ROOT_LIB_FLAGS) $(EXO_LIB_FLAGS) 

ifdef EXOFITINC
  $(info *** Using EXO_Fitting headers in $(EXOFITINC))
  INCLUDEFLAGS += -I$(EXOFITINC)
else
  $(info *** Please set EXO_Fitting version using the setup.[c]sh file ***)
endif

ifdef EXOFITLIB
  $(info *** Using EXO_Fitting library in $(EXOFITLIB))
  LIBFLAGS += -L$(EXOFITLIB) -lEXOFitting
else
  $(info *** Please set EXO_Fitting version using the setup.[c]sh file ***)
endif


SOURCES := $(wildcard $(SRCDIR)/*.cxx)
OBJECTS := $(patsubst $(SRCDIR)/%.cxx, $(BUILDDIR)/%.o, $(SOURCES))
DEPCCFILES := $(patsubst $(SRCDIR)/%.cxx, $(BUILDDIR)/%.d, $(SOURCES))
#OBJECTS := $(BUILDDIR)/$(SOURCES:.cxx=.o)
ROOTDICTHEADERS := $(shell grep -l ClassDef $(wildcard $(INCDIR)/*.hh) | xargs) 
BUILDDIRCONF := $(BUILDDIR)/.buildconf


.PHONY: all depend lib clean static shared

all: exe lib 

exe: $(EXECUTABLES) lib

lib: shared 

shared: $(SHLIB)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPCCFILES)
endif

$(BUILDDIRCONF):
	@if [ ! -d $(BUILDDIR) ]; then mkdir -p $(BUILDDIR); fi
	@touch $@

$(DEPCCFILES): $(BUILDDIR)/%.d: $(SRCDIR)/%.cxx $(BUILDDIRCONF)
	@echo "Update dependencies........ $(<F)"
	@$(MAKEDEPEND)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cxx 
	@echo "Compiling file............. $(<F)"
	@$(CXX) -c $(CXXFLAGS) $(INCLUDEFLAGS) $< -o $@

%.o: %.C
	@$(CXX) -c $(CXXFLAGS) $(INCLUDEFLAGS) $< -o $@

$(EXECUTABLES): $(OBJECTS) $(ROOTDICTOBJECT) 
	@echo "Compiling file............. $(<F)"
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBFLAGS)

$(ROOTDICTCCNAME): $(ROOTDICTHEADERS) $(AUTOLINKDEF) 
	@echo "Rootifying files........... $(@F)"
	@rm -f $(ROOTDICTCCNAME) $(ROOTDICTHHNAME) 
	@$(ROOTCINT) $@ -c -p $(CXXFLAGS) $(ROOTDICTINCLUDE) $(INCLUDEFLAGS) $^ 
 
$(SHLIB): $(OBJECTS) $(ROOTDICTOBJECT)
	@echo "Building library........... $(@F)"
	@if [ ! -d $(LIBDIR) ]; then mkdir -p $(LIBDIR); fi
	@$(SOMAKER) $(SOFLAGS) -o $@ $^ $(LIBFLAGS)

$(AUTOLINKDEF): $(LINKDEFIN) $(ROOTDICTHEADERS) 
	@echo "Auto-making Linkdef........ $(@F)"
	@$(PYTHON) makeLinkDef.py\
        --output=$@  \
        --basebuild=$(INCDIR)    \
        --input_linkdef=$(LINKDEFIN) $(ROOTDICTHEADERS)

clean:
	@rm -f $(SHLIB) $(ROOTDICTCCNAME) $(ROOTDICTHHNAME) $(BUILDDIR)/*.o *~ $(BUILDDIR)/*.d
	@rm -f $(AUTOLINKDEF) 
	@rm -f $(EXECUTABLES)


