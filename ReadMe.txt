The file final.tgz is submitted along with FinalReport.pdf.

Directory Structure:

final.tgz ---------|------etraffic.tgz
		|
		|------ ered_tc.tgz
		|
		|------FinalReport.pdf
		|
		|------testdata
		|
		|------Readme

etraffic.tgz contains the code for etraffic module. etraffic module is implemented as C code for emergency traffic client (etraffic.o) and emergency traffic server (emergency_server.o). 
The package also includes scripts to generate traffic by invoking etraffic.o and emergency_server.o

Extracted etraffic.tgz file will give following directory structure:

etraffic-----------|-----etraffic.c
		|
		|-----emergency_server.c
		|
		|-----Makefile
		|
		|-----nonEmerGen.sh
		|
		|-----nonEmerServ.sh
		|
		|-----send_emergency.sh
		|
		|-----etraffic.o
		|
		|----emergency_server.o




ered_tc.tgz contains the code for ered queuing discipline as kernel module and the iproute2 open source package with our modifications for tc to support ered in tc command utility.
Ered_tc directory structure:
Ered_tc---------|-------- ered----|-----ered.c
		|		|
		|		|-----ered.h
		|		|
		|		|------Makefile
		|		|
		|		|------ered.ko
		|
		|--------iproute2-3.16.0-----tc

ered.c contains the implementation of all the queue related functions. It also contains the statistics module functions and data structure.
ered.c code is extension of sch_red.c and adaptation of sch_gred.c 
ered.h contains the implementation of emergency packet detection module and various APIs used for ered.
iproute2-3.16.0 contains the open source distribution of iproute2 package [8] with our modification for enabling parsing of ered in tc command utility. 
ered.ko is generated after building source code. This kernel module enables the ered queuing discipline in linux networking stack.

Platform: Ubuntu 14.04 (Linux 3.13)
Software packages: Required for build environment

libdb5.3-dev
flex
	bison
	
