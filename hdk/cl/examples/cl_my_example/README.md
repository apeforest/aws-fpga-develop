# My CL Example

## Table of Contents

1. [Overview](#overview)
2. [Functional Description](#description)
3. [Hello World Example Metadata](#metadata)


<a name="overview"></a>
## Overview

This simple *my_example* example builds a Custom Logic (CL) that will enable the instance to "peek" and "poke" registers in the Custom Logic (C). These registers will be in the memory space behind AppPF BAR0, which is the ocl\_cl\_ AXI-lite bus on the Shell to CL interface.

All of the unused interfaces between AWS Shell and the CL are tied to fixed values, and it is recommended that the developer use similar values for every unused interface in the developer's CL.

Please read here for [general instructions to build the CL, register an AFI, and start using it on an F1 instance](./../README.md).


<a name="description"></a>
## Functional Description

This example is built on top of the cl_hello_world. Instead of simply return the register, I made a simple multiplication between the upper half and lower half of the 32-bit register and return their product as an unsigned integer.

Please refer to the [FPGA PCIe memory space overview](../../../docs/AWS_Fpga_Pcie_Memory_Map.md)
```
   input[15:0] sh_cl_status_vdip,               //Virtual DIP switches.  Controlled through FPGA management PF and tools.
   output logic[15:0] cl_sh_status_vled,        //Virtual LEDs, monitored through FPGA management PF and tools
```
