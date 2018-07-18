all: euc_distance histogram

euc_distance: euc_distance.cpp
	g++ -g -Wall -O3 -mavx -o euc_distance euc_distance.cpp

histogram: histogram.cpp histogram.h
	g++ -g -Wall -O3 -mavx -o histogram histogram.cpp -fopenmp

clean:
	$(RM) euc_distance histogram
