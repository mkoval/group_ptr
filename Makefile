.PHONY: all

all: group_ptr_test

group_ptr_test: group_ptr_test.cpp
	c++ --std=c++11 -o $@ $^
