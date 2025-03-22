#!/bin/bash

sudo apt update

sudo apt install -y build-essential
sudo apt install -y libssl-dev liblapack-dev libblas-dev
sudo apt install -y openssl

sudo snap install cmake --classic

# python3 pip
# google cloud storage
sudo apt install -y python3-pip
pip3 install google-cloud-storage google-cloud-compute paramiko gdown
pip3 install tcconfig 
