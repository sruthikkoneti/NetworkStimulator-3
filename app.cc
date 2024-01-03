/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

#include "ns3/point-to-point-module.h"

#include "ns3/mobility-module.h"

#include "ns3/flow-monitor-module.h"

#include "ns3/tcp-cubic.h"

#include "ns3/bulk-send-helper.h"

#include "ns3/log.h"

using namespace ns3;

// Define a logging component for this script
NS_LOG_COMPONENT_DEFINE("FifthScriptExample");

// Definition of a custom application class, MyApp, derived from ns3::Application
class MyApp : public Application
{
public:
  MyApp(); // Constructor
  virtual ~MyApp(); // Destructor

  // Method to set up the application with specific parameters
  void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication(void); // Method called when the application starts
  virtual void StopApplication(void); // Method called when the application stops

  void ScheduleTx(void); // Schedule transmission of packets
  void SendPacket(void); // Send a packet

  Ptr<Socket> m_socket; // Socket for communication
  Address m_peer; // Address of the remote peer
  uint32_t m_packetSize; // Size of each packet
  uint32_t m_nPackets; // Number of packets to send
  DataRate m_dataRate; // Data rate of transmission
  EventId m_sendEvent; // Event ID for packet transmission
  bool m_running; // Flag indicating whether the application is running
  uint32_t m_packetsSent; // Number of packets sent
};

// Definition of the constructor for MyApp
MyApp::MyApp()
    : m_socket(0),
      m_peer(),
      m_packetSize(0),
      m_nPackets(0),
      m_dataRate(0),
      m_sendEvent(),
      m_running(false),
      m_packetsSent(0)
{
}

// Definition of the destructor for MyApp
MyApp::~MyApp()
{
  m_socket = 0;
}

// Method to set up the application with specific parameters
void MyApp::Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

// Method called when the application starts
void MyApp::StartApplication(void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind();
  m_socket->Connect(m_peer);
  SendPacket();
}

// Method called when the application stops
void MyApp::StopApplication(void)
{
  m_running = false;

  if (m_sendEvent.IsRunning())
  {
    Simulator::Cancel(m_sendEvent);
  }

  if (m_socket)
  {
    m_socket->Close();
  }
}

// Method to send a packet
void MyApp::SendPacket(void)
{
  Ptr<Packet> packet = Create<Packet>(m_packetSize);
  m_socket->Send(packet);

  if (++m_packetsSent < m_nPackets)
  {
    ScheduleTx();
  }
}

// Method to schedule transmission of packets
void MyApp::ScheduleTx(void)
{
  if (m_running)
  {
    Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
    m_sendEvent = Simulator::Schedule(tNext, &MyApp::SendPacket, this);
  }
}

// // Function to handle congestion window changes
static void
CwndChange(uint32_t oldCwnd, uint32_t newCwnd)
{
  NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "\t" << newCwnd << "\t" << oldCwnd);
}

// // Function to trace the instantaneous throughput
static void
ThroughputTrace(Ptr<const Packet> packet, const TcpHeader &tcpHeader, Ptr<const TcpSocketBase> socket)
{
  // Calculate throughput
  double throughput = packet->GetSize() * 8.0 / 1e6; // in Mbps
  NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "\tThroughput: " << throughput << " \tMbps");
}

// The main function where the simulation is set up and run
int main(int argc, char *argv[])
{
  CommandLine cmd(__FILE__);
  cmd.Parse(argc, argv);

  // Create nodes
  NodeContainer Node;
  Node.Create(4);

  // Set node positions using MobilityHelper
  MobilityHelper Mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  for (uint32_t i = 0; i < Node.GetN(); ++i)
  {
    double angle = (i * 2.0 * M_PI) / (Node.GetN()); // Place nodes symmetrically
    double x = 10.0 * std::cos(angle);
    double y = 10.0 * std::sin(angle);
    positionAlloc->Add(Vector(x, y, 0.0));
  }

  Mobility.SetPositionAllocator(positionAlloc);
  Mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  Mobility.Install(Node);

  // Create remote host
  NodeContainer RH;
  RH.Create(1);

  // Set remote host position
  Ptr<ListPositionAllocator> remotePositionAlloc = CreateObject<ListPositionAllocator>();
  remotePositionAlloc->Add(Vector(0.0, 0.0, 0.0)); // Center of the ring

  Mobility.SetPositionAllocator(remotePositionAlloc);
  Mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  Mobility.Install(RH);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));

  std::string delay = "2ms";
  p2p.SetChannelAttribute("Delay", StringValue(delay));

  // Run simulations for different RTTs
  std::cout << "Running simulation with RTT: " << delay << "\n";
  // install p2p communication on all nodes
  NetDeviceContainer devices;
  for (uint32_t i = 0; i < Node.GetN() - 1; ++i)
  {
    devices.Add(p2p.Install(Node.Get(i), Node.Get((i + 1) % Node.GetN())));
  }

  NetDeviceContainer remoteDevices;
  for (uint32_t i = 0; i < Node.GetN(); ++i)
  {
        //connecting all the nodes to remote host
    remoteDevices.Add(p2p.Install(Node.Get(i), RH.Get(0)));
  }

  // Install Internet stack
  InternetStackHelper stack;
  stack.Install(Node);
  stack.Install(RH);

  // Assign IP addresses
  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign(devices);

  // address.SetBase("10.1.2.0", "255.255.255.200");
  Ipv4InterfaceContainer interfaces2 = address.Assign(remoteDevices);
  
  // Set up TCP applications Cubic is one of the TCP congestion control algorithms
  Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));

  uint16_t sinkPort = 8080;
  ApplicationContainer Sinkapps;
  PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
  Sinkapps.Add(packetSinkHelper.Install(RH));
  Sinkapps.Start(Seconds(0.0));
  Sinkapps.Stop(Seconds(50.0));
  
  for(uint32_t i=0; i<Node.GetN(); i++) {
    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (Node.Get (i), TcpSocketFactory::GetTypeId ());
    ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));
    ns3TcpSocket->TraceConnectWithoutContext ("Tx", MakeCallback (&ThroughputTrace));


    Ptr<MyApp> app = CreateObject<MyApp> ();
    app->Setup (ns3TcpSocket, InetSocketAddress (interfaces2.GetAddress (1), sinkPort), 1040, 1000, DataRate ("1Mbps"));
    Node.Get (i)->AddApplication (app);
    app->SetStartTime (Seconds (1.));
    app->SetStopTime (Seconds (50.));
  }
  
  // -----------------------------------
  // for (uint32_t i = 0; i < Node.GetN(); ++i)
  // {
  //   // Address sinkAddress(InetSocketAddress(interfaces.GetAddress(i), sinkPort));
  //   // Sinkapps.Add(packetSinkHelper.Install(Node.Get(i)));
  //   std::cout << Node.Get(i)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal() << std::endl;
  // }
  // uint32_t port = 9;

  // ApplicationContainer appsBulk;
  // for (uint32_t i = 0; i < Node.GetN(); ++i)
  // {
  //   BulkSendHelper bulkSender("ns3::TcpSocketFactory", InetSocketAddress(RH.Get(0)->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal(), port));
  //   bulkSender.SetAttribute("MaxBytes", UintegerValue(10000000)); // Set maximum bytes to send
  //   appsBulk.Add(bulkSender.Install(Node.Get(i)));
  // }

  // appsBulk.Start(Seconds(1.0));
  // appsBulk.Stop(Seconds(50.0));
  // Sinkapps.Start(Seconds(0.0));
  // Sinkapps.Stop(Seconds(50.0));

  // ------------------------------------
  // Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(Node.Get(0), TcpSocketFactory::GetTypeId());
  // ns3TcpSocket->TraceConnectWithoutContext("CongestionWindow", MakeCallback(&CwndChange));
  // ns3TcpSocket->TraceConnectWithoutContext("Tx", MakeCallback(&ThroughputTrace));

  // Ptr<MyApp> app = CreateObject<MyApp>();
  // app->Setup(ns3TcpSocket, sinkAddress, 1040, 1000, DataRate("1Mbps"));

  // for (uint32_t i = 0; i < Node.GetN(); ++i)
  // {
  //   Node.Get(i)->AddApplication(app);
  //   app->SetStartTime(Seconds(1.));
  //   app->SetStopTime(Seconds(50.));
  // }

  // Create FlowMonitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();

  Simulator::Stop(Seconds(50));
  Simulator::Run();
  // Cleanup
  Simulator::Destroy();

  // Collect statistics Print flow statistics
  flowMonitor->CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
  std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator it = stats.begin(); it != stats.end(); ++it)
  {
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(it->first);

    std::cout << "Flow " << it->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
    std::cout << "  Tx Packets: " << it->second.txPackets << "\n";
    std::cout << "  Tx Bytes: " << it->second.txBytes << "\n";
    std::cout << "  Rx Packets: " << it->second.rxPackets << "\n";
    std::cout << "  Rx Bytes: " << it->second.rxBytes << "\n";
    std::cout << "  Throughput: " << it->second.rxBytes * 8.0 / (50.0 * 1000000.0) << " Mbps\n";

    // Add print statements for Delay and Packet Loss Rate
    std::cout << "  Delay: " << it->second.delaySum.GetSeconds() / it->second.rxPackets << " seconds\n";
    std::cout << "  Packet Loss Rate: " << static_cast<double>(it->second.lostPackets) / static_cast<double>(it->second.txPackets) << "\n";

    std::cout << t.sourceAddress << " -> " << t.sourcePort << " - " << t.destinationAddress << " -> " << t.destinationPort << "\n\n";
  }

  return 0;
}
