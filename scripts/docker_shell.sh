#!/bin/zsh
colima start && docker run --cap-add=SYS_PTRACE --security-opt seccomp=unconfined -it -v ./:/mnt/build clang-image:latest bash