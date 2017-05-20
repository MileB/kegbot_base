# Simple Makefile for Kegbot code
# Note: Requires these packages:
#  libmysqlcppconn-dev
#  wiringpi

TARGET=kegbot
CXX=g++

LXXFLAGS=-lwiringPi -lwiringPiDev `mysql_config --libs` -lmysqlcppconn
CXXFLAGS=-Wall -g -std=c++11 -pthread `mysql_config --cflags` -I/usr/include/cppconn
ALLOBJ=kegbot.o db_access.o config_parser.o base64.o

$(TARGET): $(ALLOBJ)
	$(CXX) $(ALLOBJ) $(LXXFLAGS) -o $(TARGET)

$(TARGET).o: $(TARGET).cpp db_access.o
	$(CXX) $(CXXFLAGS) -c $(TARGET).cpp

db_access.o: db_access.cpp config_parser.o base64.o

config_parser.o: config_parser.cpp

base_64.o: base64.cpp

.PHONY: clean cleaner

clean:
	rm -f *.o

cleaner: clean
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: $(TARGET)
	gdb -tui ./$(TARGET)
