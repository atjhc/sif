#!/bin/zsh
colima start && docker run -it -v ./:/mnt/build clang-image:latest bash