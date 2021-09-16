CC = clang++

LIBS = ../unpv13e/libunp.a

TARGET = tftp.out

all: $(TARGET)

$(TARGET): main.cpp tftp_srv.cpp tftp_srv.h
	$(CC) -o $(TARGET) main.cpp tftp_srv.cpp $(LIBS)

clean:
	$(RM) $(TARGET)