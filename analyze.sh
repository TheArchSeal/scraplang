#!/bin/bash
make clean && scan-build make all && valgrind --leak-check=full make test
