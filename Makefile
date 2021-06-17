CFLAGS = -std=c++14 -O2 -Wall -g -lmysqlclient -L/usr/lib64/mysql -pthread
TARGET = server
OBJS = main.cpp ./code/buffer/*.cpp ./code/http/*.cpp ./code/log/*.cpp \
	./code/pool/*.cpp ./code/server/*.cpp ./code/timer/*.cpp \

all: $(OBJS)
	g++ $(CFLAGS) $(OBJS) -o $(TARGET)

clean:
	rm -rf $(TARGET) 