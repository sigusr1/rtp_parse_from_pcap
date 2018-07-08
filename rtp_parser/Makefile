TARGET=./bin/rtp_parser
CPP=g++
CFLAGS	+=  -g -I ./include -DENABLE_TCPREASM
SRC=./src
OBJECTS=$(SRC)/rtp_parser.o $(SRC)/dynamic_ring_buffer.o $(SRC)/main.o
$(TARGET) : $(OBJECTS)
	$(CPP) $(OBJECTS)   -g -o $(TARGET) -L./lib/ -lnids -lpcap

$(SRC)/%.o : $(SRC)/%.cpp	
	$(CPP) -c $(CFLAGS) $^ -o $@
all:$(TARGET)
clean :
	rm -fr $(OBJECTS) $(TARGET) 
