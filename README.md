# NetworkStimulator-3
Network Simulator 3 (NS3)

### Problem Statement
Create N number of nodes and set them in a topology. Create a remote
host and place it such that it aligns symmetrically with all nodes. Establish
Uplink TCP communication between remote host and all other nodes.
Nodes will be sending data to the remote host. Set the positions of nodes
using the `MobilityHelper` Class. Simulate the whole scenario for 50
seconds. Implement this for `TcpNewReno` and `TcpCubic` and generate
Log Files. Log files should contain the data of congestion window,
throughput etc.,
a. Trace Congestion Window of each node and plot them using gnuplot
or any other tool.
b. Use `FlowMonitor` to show the stats like Tx Packets, Tx Bytes, Rx
Packets, Rx Bytes, Throughput, Delay, Packet Loss Rate.
c. Trace the Instantaneous Throughput of each node and plot them
d. Discuss on throughput of each node, if the throughput varies why?
e. Implement this setup for different RTT’s and observe how
TcpNewReno and TcpCubic performs
f. Write your observations and differentiate between TcpNewReno and
TcpCubic

Note:This was the Computer Networks Course assignment that we were given

## Instructions to Run
1) Make sure your device ( Linux Operating System) has NS-3 installed and properly configured.
2) Place the app.cc in a the `scratch` folder in the ns3-dev folder ( where ns-3 was installed)
3) Navigate to `scratch` folder in the terminal by running `cd ns3-dev/scratch`
4) Run `./waf --run app`
   
[Solution to the Problem Statement](https://docs.google.com/document/d/1y3Og03yxvdz6aXIJuOGYNQszdx2PxamN7qRQZiEcIwI/edit?usp=sharing)


