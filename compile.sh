#!/bin/sh

./format.sh
qmake
sudo make -j$(nproc) install
