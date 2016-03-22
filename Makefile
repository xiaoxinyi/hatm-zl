# The Makefile for the C++ implementation of HATM.

COMPILER = g++
OBJS = utils.o topic.o tree.o document.o corpus.o gibbs.o hatm_main.o
SOURCE = $(OBJS:.o=.cc)

FLAGS = -g -Wall  -I/usr/local/Cellar/gsl/1.16/include

# GSL library
LIBS = -lgsl -lgslcblas -L/usr/local/Cellar/gsl/1.16/lib

default: hatm

hatm: $(OBJS) 
	$(COMPILER) $(FLAGS) $(OBJS) -o hatm  $(LIBS)

%.o: %.cc
	$(COMPILER) -c $(FLAGS) -o $@  $< 

.PHONY: clean
clean: 
	rm -f *.o