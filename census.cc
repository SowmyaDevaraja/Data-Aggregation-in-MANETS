/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mobility-helper.h"
#include "ns3/census-helper.h"
#include "ns3/census-application.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/census-nodeinfo.h"
#include "ns3/gnuplot.h"
#include <cmath>
#include <cassert>
#include <map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Census");

//Jesse stuff
template < class T >
std::string stringify(const T& t) {
	std::stringstream ss;
	ss << t;
	return ss.str();
}

//end Jesse stuff

static void printInfo(int x){
    //nodec->Get(1)->GetObject<VanetApplication>()->GetTurnNumber();
    NS_LOG_INFO("inside print "<< x);
    cout << "Inside Print info" << endl;
    int nt = CensusNodeInfo::getReport(x);
    
    if (x<2) nt=1;
    //NS_LOG_INFO("Inside print info " << x << " " << nodec->Get(1)->GetObject<VanetApplication>()->GetTurnNumber());
    if (!Simulator::IsFinished() && nt>0) {
        Simulator::Schedule(Seconds(snapinterval), &printInfo, x+1);
    }
}



int main (int argc, char *argv[])
{
  
  //int uniform_mobility=1;

  LogComponentEnable ("Census", LOG_LEVEL_LOGIC);
  LogComponentEnable ("CensusApplication", LOG_LEVEL_LOGIC);
  LogComponentEnable ("CensusNodeInfo", LOG_LEVEL_LOGIC);

  //CommandLine cmd;
  int MaxNodes;
  int runid;
  int numtoks;
  int desired_density;
  int desired_speed;
  CommandLine cmd;
  //cmd.AddValue("MaxNodes", "Number of nodes", MaxNodes);
  cmd.Parse (argc, argv);
  MaxNodes = atoi(argv[1]);
  runid = atoi(argv[2]);
  numtoks = atoi(argv[3]);
  desired_density = atoi(argv[4]);
  desired_speed = atoi(argv[5]);

  //now desired density is function of node size
  desired_density = 4*log(MaxNodes);
  cout << log(MaxNodes) << endl;
  cout << "desired_desity is now " << desired_density << endl;
  string speed;

  switch(desired_speed){
     case 0: speed = "Uniform:0:0"; break;  
     case 3: speed = "Uniform:2:4"; break;
     case 9: speed = "Uniform:8:10"; break;
     case 15: speed = "Uniform:14:16"; break;
     case 21: speed = "Uniform:20:22"; break;
  }
  //caliculate the world size for desired density
  
  double XBound = sqrt((MaxNodes*M_PI)/(desired_density)) * 60;
  double YBound = sqrt((MaxNodes*M_PI)/(desired_density)) * 60;

  cout << "\nXBound = "<< XBound << " YBound = " << YBound << endl;
  CensusNodeInfo::getXYNwBound(XBound, YBound);

  //census.SetAttribute ("MaxNodes", UintegerValue (MaxNodes));
  NS_LOG_INFO("Creating Nodes .." << MaxNodes);
  NodeContainer c;
  c.Create (MaxNodes);

  CensusNodeInfo::getNodecontainer(&c, MaxNodes, runid, numtoks);
  CensusNodeInfo::divideNetwork();

  /*Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", 
        StringValue ("2200"));*/
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", 
            StringValue ("100"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
        StringValue ("DsssRate11Mbps"));      
        
  MobilityHelper mobility;   

/* 1/4 commenting out mobility setting. As I want to use fixed allocation.*/
  std::ostringstream sstreamb;
  //sstreamb << "ns3::UniformRandomVariable[Min=0|Max=" << XBound <<"]";
  sstreamb << "Uniform:0:" << XBound <<"]";
  std::string boundAsString = sstreamb.str();

  mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                 "X", StringValue (boundAsString),
                                 "Y", StringValue (boundAsString));
                                
  std::ostringstream sstream;
  sstream << "0|" << XBound << "|0|" << YBound;
  std::string varAsString = sstream.str();
if(desired_speed !=0){
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Distance"),
                             "Distance", StringValue(stringify(30)),
                             "Bounds", StringValue (varAsString),
                             "Speed", StringValue(speed));
}

/*****************************************************************************************/


            
/* 1/4 commenting out mobility setting. As I want to use fixed allocation.*/
/*  std::ostringstream sstreamb;
  //sstreamb << "ns3::UniformRandomVariable[Min=0|Max=" << XBound <<"]";
  sstreamb << "Uniform:0:" << XBound <<"]";
  std::string boundAsString = sstreamb.str();

  mobility.SetPositionAllocator ("ns3::RandomRectanglePositionAllocator",
                                 "X", StringValue (boundAsString),
                                 "Y", StringValue (boundAsString));


  std::ostringstream sstream;
  sstream << "0|" << XBound << "|0|" << YBound;
  std::string varAsString = sstream.str();

if(desired_speed !=0){
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Mode", StringValue ("Distance"),
                             "Distance", StringValue(stringify(30)),
                             "Bounds", StringValue (varAsString),
                             "Speed", StringValue(speed));

} */ 
  mobility.Install(c);
  //VanetNodeInfo::updateDistances1();

  /*
  Ptr<SwChannel> swChan = CreateObject<SwChannel> ();
  SwMacCsmaHelper swMac = SwMacCsmaHelper::Default ();
  SwPhyBasicHelper swPhy = SwPhyBasicHelper::Default ();
  SwHelper sw;
  NetDeviceContainer staDevice = sw.Install (c, swChan, swPhy, swMac);
 */

  
    
  //YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  //wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel");
  //unit disk model
  wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel","MaxRange",DoubleValue (MaxTxDist));
  //wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  
  /*
  wifiChannel.AddPropagationLoss("ns3::TwoRayGroundPropagationLossModel",
		  	  	  	  	  	  	  "SystemLoss", DoubleValue(1),
   		  	  	  	  	  	  	  "HeightAboveZ", DoubleValue(1.5));
  */
  
  /*wifiChannel.AddPropagationLoss("ns3::NakagamiPropagationLossModel",
                                        "Distance1",DoubleValue (MaxTxDist),
                                        "Distance1",DoubleValue (MaxTxDist),
                                        "m0",DoubleValue (1.0),
                                        "m1",DoubleValue (1.0),
                                        "m2",DoubleValue (1.0));*/

  
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
 
  /* abhinay
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue(-88.3));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue(-92.3));
  wifiPhy.Set ("TxPowerStart", DoubleValue(33.0));
  wifiPhy.Set ("TxPowerEnd", DoubleValue(33.0));
  wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
  wifiPhy.Set ("TxGain", DoubleValue(0));
  wifiPhy.Set ("RxGain", DoubleValue(0));
  */
  
  wifiPhy.SetChannel (wifiChannel.Create ());

  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  //AsciiTraceHelper ascii; 
  //wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("vanets.tr")); 
  //wifiPhy.EnablePcap("vanets", staDevice,true);

  

//  std::string phyMode ("DsssRate1Mbps");
/* original abhinay
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager");
*/

  /*                              "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode),
                                "MaxSlrc", UintegerValue(7));
  */
  
  //Jesse
  
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "ControlMode", StringValue ("DsssRate11Mbps"),
                                "DataMode", StringValue ("DsssRate11Mbps"));
  
  // end jesse
  
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  //Jesse uses the foll
  wifiMac.SetType ("ns3::AdhocWifiMac");
  

  NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, c);
 //wifiPhy.EnablePcap("vanets", staDevice,true);
      
  InternetStackHelper internet; 
  internet.Install (c); 
  Ipv4AddressHelper ipv4; 
  NS_LOG_INFO ("Assign IP Addresses."); 
  ipv4.SetBase ("129.0.0.0", "255.0.0.0"); 
  Ipv4InterfaceContainer i = ipv4.Assign (staDevice);
  CensusNodeInfo::getAddresscontainer(&i);

  CensusHelper vapp("ns3::UdpSocketFactory");


//  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeCallback (&VanetNodeInfo::updateDistances));  

  
  ApplicationContainer nodeApps = vapp.Install(c);
  nodeApps.Start (Seconds (0.0));
  nodeApps.Stop (Seconds (SimTime));  

  //NS_LOG_INFO("from main "+c.Get(1)->GetObject<VanetApplication>()->GetTurnNumber());
  //Simulator::Schedule(Seconds(1), &printInfo, 5, &c);
  
cout << "Calling for termination check" << endl;
  printInfo(1);
  Simulator::Stop (Seconds (SimTime));

  Simulator::Run ();

  Simulator::Destroy ();

  CensusNodeInfo::fileClose();  
         
  return 0;
}
