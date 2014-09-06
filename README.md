CS6210 - Project 3

Girish Dhoble and Swapnil Deshpande

The project contains following files:
1. Project documentation
2. Thrift file
3. Server program and client program
4. Experiment Data
5. Experiment Result
6. Makefile

To compile and run the programs and experiments, perform following steps:
1. After extracting the code, open the directory gen-cpp. Make the files using following command: 
	# make
2. Run the server on one machine using following command:
	# ./server
3. Modify the client program to include ip address of the machine on which server program is running (by default it is set to localhost)
4. The experiment data is available in 'ExperimentData' folder. Enter the name of the txt file (each file represents one workload) in the client program to run experiments.
4. Run the client on another machine using following command:
	# ./client
5. The server program would measure the number of hits for a workload which would help in determining the cache ratio and the client program gives the service time latency.

