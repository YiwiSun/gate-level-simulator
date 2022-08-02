# gate-level-simulator
A gate-level simulator based on GPU, using CUDA C/C++.  

This project contains two implementations of gate-level simulating, a simulator accelerated via GPU, and a serial simulator based on CPU for comparison. 
# Installation
This simulator uses boost library and CUDA parallel computing platform for GPU acceleration. You need to install all required dependencies and related tools to use it. 
General dependencies such as a C++ compiler and build-essential are necessary for building. 
## Boost Library  
Please visit [Boost Library](https://www.boost.org/) for more introduction and download link. You need to build and install it with the following commands.  
```
cd ./boost_x_xx_x  
./bootstrap.sh  
sudo ./b2 install
```  
## CUDA Toolkit  
Please check the GPU model, and make sure your GPU supports CUDA programming in [CUDA-enabled products](https://developer.nvidia.com/cuda-gpus).  
```
lspci | grep -i nvidia  
```  
Check the CUDA toolkit and corresponding driver versions in [release notes](https://docs.nvidia.com/cuda/cuda-toolkit-release-notes/index.html), 
and download the release of [CUDA toolkit](https://developer.nvidia.com/cuda-toolkit) appropriate for your hardware configuration. Take CUDA-9.2 as an example. 
```
sudo ./cuda_9.2_linux.run  
```  
To modify environment variables PATH and LD_LIBRARY_PATH.  
```
vim ~/.bashrc  
export PATH=$PATH:/usr/local/cuda-9.2/bin  
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/cuda-9.2/lib64  
```  
## Building from Source  
To build serial simulator simply type 'make'. For the GPU based simulator it might be necessary to make some changes to the Makefile.  
```
make  
vim Makefile  
```  
# Getting Started  
This simulator has two steps (process and simulate). Take the given design as an example.  
```  
./process ./examples/GENERIC_STD_CELL.vlib \
          ./examples/default-rando/ExampleRocketSystem_GEN.gv \
          ./examples/default-rando/ExampleRocketSystem_GEN.sdf \
          ./examples/default-rando/inter.dat \
          > ./examples/default-rando/process_log  
          
./simulate ./examples/default-rando/inter.dat \
           ./examples/default-rando/ExampleRocketSystem_rando_input.vcd \
           0 20000001 \
           ./examples/default-rando/out.saif \
           > ./examples/default-rando/log_simulate
