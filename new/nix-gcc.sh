#!/usr/bin/env bash

nix-shell --pure --run 'make clean && make -j && make test' viua.nix
