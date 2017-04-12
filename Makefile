
TARGET=kegbot_base
CXX=g++
LXXFLAGS= `mysql_config --libs` -lmysqlcppconn
CXXFLAGS=-Wall -g  -std=c++11 -pthread `mysql_config --cflags` -I/usr/include/cppconn
ALLOBJ=db_access.o db_test.o config_parser.o base64.o

$(TARGET).out: $(ALLOBJ)
	$(CXX) $(ALLOBJ) $(LXXFLAGS) -o $(TARGET).out

$(TARGET).o: $(TARGET).cpp
	$(CXX) $(CXXFLAGS) -c $(TARGET).cpp

db_access.o: db_access.cpp config_parser.o base64.o

db_test.o:	db_test.cpp db_access.o


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
