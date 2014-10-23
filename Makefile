###vars definations
target = NtpClient
#headers = *.h
cpps = *.cpp
objs = *.o
###path definations
boost_include_path = /usr/local/boost_1_56_0
boost_lib_path = /usr/local/boost_1_56_0/stage/lib
VPATH = ./
###dependence libs
boost_perfix = boost_
libs = -l$(boost_perfix)system \
       -l$(boost_perfix)date_time \
       -lpthread

###target starts here
NtpClient: $(objs)
	g++ $(objs) \
		-o $(target) \
		-L $(boost_lib_path) \
		$(libs) \
		-std=c++11 \
        -static
$(objs): $(cpps)
	g++ -I $(boost_include_path) -c $(cpps) -std=c++11

:PHONY: clean
clean:
	rm $(objs)
