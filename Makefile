
TARGET=kegbot_base
CXX=g++

LXXFLAGS=-lwiringPi -lwiringPiDev `mysql_config --libs` -lmysqlcppconn
CXXFLAGS=-Wall -g -std=c++11 -pthread `mysql_config --cflags` -I/usr/include/cppconn
ALLOBJ=kegbot_base.o db_access.o config_parser.o base64.o

$(TARGET).out: $(ALLOBJ)
	$(CXX) $(ALLOBJ) $(LXXFLAGS) -o $(TARGET).out

$(TARGET).o: $(TARGET).cpp db_access.o
	$(CXX) $(CXXFLAGS) -c $(TARGET).cpp

db_access.o: db_access.cpp config_parser.o base64.o

config_parser.o: config_parser.cpp

base_64.o: base64.cpp

kegbot_base.o: kegbot_base.cpp db_access.o

.PHONY: clean cleaner

clean:
	rm -rf *.o

cleaner: clean
	rm -rf ./$(TARGET).out

run: $(TARGET).out
	./$(TARGET).out

debug: $(TARGET).out
	gdb -tui ./$(TARGET).out 
	
#Sam was here
#Yue was not here
#Michael was sometimes here
