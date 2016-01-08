all: common client serverC serverG serverT

common: common.h common.cpp
	$(CXX) -c common.cpp

client: common client.cpp
	$(CXX) client.cpp -o client common.o

serverC: common serverC.cpp
	$(CXX) serverC.cpp -o serverC common.o

serverG: common serverG.cpp
	$(CXX) serverG.cpp -o serverG common.o

serverT: common serverT.cpp
	$(CXX) serverT.cpp -o serverT common.o

clean:
	rm common.o client serverC serverG serverT
