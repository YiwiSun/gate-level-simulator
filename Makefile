# Makefile for vcdparser
# /usr/lib64/libboost_serialization.a
EXEC=process # simulate

HEADS=$(shell find ./ -name "*.h")
SRC=$(shell find ./ -name "*.cpp")
SRC_PROCESS=PreProcess.cpp VCDParser.cpp VCDValue.cpp process.cpp
#SRC_SIMU=PreProcess.cpp VCDParser.cpp VCDValue.cpp simulate.cpp
OBJ_PROCESS=$(SRC_PROCESS:%.cpp=%.o)
#OBJ_SIMU=$(SRC_SIMU:%.cpp=%.o)
OBJ=$(SRC:%.cpp=%.o)

CPP=g++
OPTS=-std=c++11 -O2 -Wall -g -L/usr/local/include
# LIBS=-L./lib -lbcminisat220core -l_release -lload_tr0 -lerror -lmanager -lparser -ldata_model

all: ${EXEC}

process: ${OBJ_PROCESS}
	${CPP} ${OPTS} -L/usr/local/lib -lboost_serialization  -o $@ ${OBJ_PROCESS}

#simulate: ${OBJ_SIMU}
#	${CPP} ${OPTS} -o $@ ${OBJ_SIMU} -lboost_serialization

%.o: %.cpp ${HEADS}
	${CPP} ${OPTS} -c -o $@ $<

clean:
	rm -f ${OBJ} ${EXEC}
