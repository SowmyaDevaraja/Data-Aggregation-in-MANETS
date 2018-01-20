#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/inet-socket-address.h"
#include "ns3/mobility-module.h"
#include <math.h>

#include "census-nodeinfo.h"

#include "census-application.h"

NS_LOG_COMPONENT_DEFINE ("CensusApplication");

using namespace std;

/* 11/27 this version removes split requests */

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (CensusApplication);

TypeId CensusApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CensusApplication")
    .SetParent<Application> ()
    .AddConstructor<CensusApplication> ()
    .AddAttribute ("NewTxTime", "A RandomVariable used to pick the duration of the 'Tx' state.",
                   RandomVariableValue (UniformVariable(0.0,FwdTxDelay)),
                   MakeRandomVariableAccessor (&CensusApplication::m_newTxTime),
                   MakeRandomVariableChecker ())
    .AddAttribute ("MaxBytes", 
                   "The total number of bytes to send. Once these bytes are sent, "
                   "no packet is sent again, even in on state. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&CensusApplication::m_maxBytes),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&CensusApplication::m_tid),
                   MakeTypeIdChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor(&CensusApplication::m_txTrace))
  ;
  return tid;
}


CensusApplication::CensusApplication ()	
{
  //NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
 	
  TurnNumber=1;

  m_lastStartTime = Seconds (0);

}

CensusApplication::~CensusApplication()
{
  //NS_LOG_FUNCTION_NOARGS ();
}

int CensusApplication::GetTurnNumber()
{
	//TurnNumber=1;
	//NS_LOG_INFO("reached getturnnumber" << TurnNumber);
	//int y=1;
	return TurnNumber;
	
}

void CensusApplication::SetMaxBytes (uint32_t maxBytes)
{
  NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

Ptr<Socket> CensusApplication::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

void CensusApplication::DoDispose (void)
{
  m_socket = 0;
  
  Application::DoDispose ();
}


// Application Methods
// Sowmya : invoked as the number of nodes being created.

void CensusApplication::StartApplication () // Called at time specified by Start
{
  waitInterval = Seconds (m_newTxTime.GetValue ());
  seqnum=0;
  // Create the socket if not already
  //NS_LOG_INFO("inside app start");
  if (!m_socket)
  {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      m_socket->Bind (InetSocketAddress(Ipv4Address::GetAny(),88));

      m_socket->SetAllowBroadcast (true);
      
      m_socket->Connect (InetSocketAddress(Ipv4Address::GetBroadcast(),88)); 
	
      m_socket->SetRecvCallback (MakeCallback(&CensusApplication::receivingMsg,this));
  }

  CancelEvents ();
  weight=1.0;
  MaxNodes = CensusNodeInfo::getMaxNodes();
  numtoks = CensusNodeInfo::getToks();
  levelCount = CensusNodeInfo::getMaxLevelCount();

  requestsent=0;
  requestpending=0;
  holder=0;
  reached=0;
  announce_trials=0;
  safety_mode=0;
  safety_receiver=-1;
  safety_sends=0;
  holder_nearby=0;
  num_token_requestors=0;
  rqw_set=0;
  ack_mode=0;
  ack_send_pending=0;
  ack_receiver=-1;
  num_visited = 0; // clear number of times this node has visited.
  flood_result = true;
  level = -1;
// SOWMYA : what are these requestor_weight and id used for?
  for (int i=0; i<max_token_requestors; i++) // Sowmya : loop 20times to initialize
  {
        requestor_weight[i]=0.0;
        requestor_id[i]=-1;
	requestor_cellId[i]=-1;
	requestor_level[i] = -1;
  }

  /*int token_factor=1;
  if (numtoks!=0)
  {
        token_factor = (int)(MaxNodes/numtoks)+1;
  }
  else
  {
        token_factor = (int)(sqrt(MaxNodes)) + 1;
  }

  if ((GetNode()->GetId())%token_factor == 0) //only token starters
  {
        starter = GetNode()->GetId();
        holder=1;
        num_visited++;  //token holder, increment number of visited
        CensusNodeInfo::update_visited_num(GetNode()->GetId());
        //debuging
        cout << "node id " << GetNode()->GetId() <<  "num_visited: " << num_visited << endl;
        reached=1;
        CensusNodeInfo::updateCount(GetNode()->GetId());
        CensusNodeInfo::updateHolder(GetNode()->GetId());
                                        }
        Simulator::Schedule (Seconds(0.1*MaxNodes+1), &CensusApplication::SendAnnounce, this);
        announce_trials=1;
  }*/
  double x_pos = GetNode()->GetObject<MobilityModel>()->GetPosition().x;
  double y_pos = GetNode()->GetObject<MobilityModel>()->GetPosition().y;
  
  cout << "Node ID " << GetNode()->GetId() << " X = " << GetNode()->GetObject<MobilityModel>()->GetPosition().x << " Y = " << GetNode()->GetObject<MobilityModel>()->GetPosition().y << endl;
  std::pair <int, int> cellID_token;// CellID : Level
  cellID_token.first = -1;
  cellID_token.second = -1;

  cellID_token = CensusNodeInfo::checkForTokenGeneration(x_pos, y_pos);
  
  if(cellID_token.first >= 0 && cellID_token.second >= 0)
  {
        cout << "Pair_cellIDToken = " << cellID_token.first << " Pair_Level = " << cellID_token.second << endl;
	starter = GetNode()->GetId();
	level_starter = cellID_token.second;
        holder=1;
        num_visited++;  //token holder, increment number of visited
        CensusNodeInfo::update_visited_num(GetNode()->GetId());
        //debuging
        cout << "node id " << GetNode()->GetId() <<  "num_visited: " << num_visited << endl;
        reached=1;
  	std::pair<int, int> ID_level = CensusNodeInfo::getCellId(x_pos, y_pos, cellID_token.second);
	cout << "Pair_cellID = " << ID_level.first << " Pair_Level = " << ID_level.second << "Node ID : " << GetNode()->GetId() << endl;

		bool ret = CensusNodeInfo::updateCount_perCell(GetNode()->GetId(), ID_level);

		if (ret == true)
		{
			ResultFlood();	
		}
	level = ID_level.second;

        CensusNodeInfo::updateHolder(GetNode()->GetId(), level);
        Simulator::Schedule (Seconds(0.1*MaxNodes+1), &CensusApplication::SendAnnounce, this);
        announce_trials=1;
  }
  Simulator::Schedule(Seconds(0.1*GetNode()->GetId()),&CensusApplication::Push,this);
}

void CensusApplication::Push()
{
	CensusPayload payload=CensusPayload();
	payload.nodeid=(int)((m_socket->GetNode())->GetId());
	payload.type=12;  //push 
	payload.senderx = GetNode()->GetObject<MobilityModel>()->GetPosition().x;
	payload.sendery = GetNode()->GetObject<MobilityModel>()->GetPosition().y;
	payload.weight=0;
	Ptr<Packet> pkt = Create<Packet> ((uint8_t*) &payload,sizeof(CensusPayload));
	m_txTrace (pkt);
	m_socket->Send (pkt);      
}

void CensusApplication::SendAnnounce()
{
 if(starter == GetNode()->GetId())
 {
      cout << "I am initial token holder " << starter << " for level " << level_starter << endl;
 }
 else
 {
      cout << "Starting token announce " << GetNode()->GetId() << endl;
 }
 if (holder==1)//Sowmya: That node has the token
 {
 
   /*if ((numtoks==1) && ((double)(CensusNodeInfo::getCountsofar()) >= 0.99*((double)(MaxNodes))))
   {
   		exit(1); //if num tokens is 1 and we have crossed 99%, we can exit
   }*/
   if (ack_mode==1) //token was received and node was in ack mode; first we cancel ack related variables
   {
   	ack_mode=0;
   	ack_receiver=-1;
   	ack_send_pending=0;
   }
   
   if (holder_nearby==1)
   {
   	NS_LOG_INFO("holder nearby, announcement postponed");
	cout << "  holder nearby, announcement postponed " << endl;
   	holder_nearby=0;
   	Simulator::Schedule (Seconds(announce_wait_time), &CensusApplication::AnnounceWait, this);
   }
   else
   {
	CensusPayload payload=CensusPayload();
	payload.nodeid=(int)((m_socket->GetNode())->GetId());
	payload.type=0; //announce
	payload.level=level;
	payload.senderx = GetNode()->GetObject<MobilityModel>()->GetPosition().x;
	payload.sendery = GetNode()->GetObject<MobilityModel>()->GetPosition().y;
	Ptr<Packet> pkt = Create<Packet> ((uint8_t*) &payload,sizeof(CensusPayload));

  	m_txTrace (pkt);
 	m_socket->Send (pkt);
  	m_lastStartTime = Simulator::Now ();
  	
  	CensusNodeInfo::updateSA(GetNode()->GetId(), level);
  	num_token_requestors=0;
  	//announce_sent=1;
  	Simulator::Schedule (Seconds(token_request_gather_time), &CensusApplication::SendToken, this);
  	ev2 = Simulator::Schedule (Seconds(announce_wait_time), &CensusApplication::AnnounceWait, this);
    }
 }
 
}

double CensusApplication::getLevel()
{
	double level=1.0/weight;
 	if (level!=1.0)
 	{
 		level = log2(level);
 	}
 	return level;
}

void CensusApplication::AnnounceWait()
{

  if (holder==1)
  {
	cout << "Announce Trials : " << announce_trials << " Max Announce Trials " << MaxAnnounceTrials << " Node ID = " << ((m_socket->GetNode())->GetId()) << endl;
	if ((announce_trials <= MaxAnnounceTrials))
	{
		if (holder_nearby==1)
   		{
   			//NS_LOG_INFO("holder nearby, announcement postponed");
			cout << "holder nearby, announcement postponed " << (int)((m_socket->GetNode())->GetId()) << endl;
   			holder_nearby=0;
   			ev2 = Simulator::Schedule (Seconds(announce_wait_time), &CensusApplication::AnnounceWait, this); //give a chance for token to move out
   		}
   		else
   		{
			CensusPayload payload=CensusPayload();
			payload.nodeid=(int)((m_socket->GetNode())->GetId());
			payload.type=0; //announce
			payload.level = level;
			payload.senderx = GetNode()->GetObject<MobilityModel>()->GetPosition().x;
			payload.sendery = GetNode()->GetObject<MobilityModel>()->GetPosition().y;
			cout << "Node ID = " << payload.nodeid << endl;
			Ptr<Packet> pkt = Create<Packet> ((uint8_t*) &payload,sizeof(CensusPayload));

		  	m_txTrace (pkt);
		 	m_socket->Send (pkt);
		  	m_lastStartTime = Simulator::Now ();
  	
		  	
		  	CensusNodeInfo::updateSA(GetNode()->GetId(), level);
		  	num_token_requestors=0;
		  	//announce_sent=1;
		  	Simulator::Schedule (Seconds(token_request_gather_time), &CensusApplication::SendToken, this);
		  	ev2 = Simulator::Schedule (Seconds(announce_wait_time), &CensusApplication::AnnounceWait, this); //let diffusion happen for a while
    		}
		announce_trials++;
	}
	else
	{
		ResultFlood();
		//NS_LOG_INFO("disconnected for more than 150 seconds");
		CensusNodeInfo::updateTerminated(GetNode()->GetId());		
	}
  }

}

void CensusApplication::receivingMsg(Ptr<Socket> socket)
{
	Ptr<Packet> pkt;
	pkt = socket->Recv();

	CensusPayload payload;

	//NS_LOG_INFO("received Packet by" << GetNode()->GetId());
	int NumOfNodesInfo = (pkt->GetSize() / sizeof(CensusPayload));
	for (int i=0;i<NumOfNodesInfo;i++)
	{
		pkt->CreateFragment(i*sizeof(CensusPayload),sizeof(CensusPayload))->CopyData((uint8_t *)&payload,sizeof(CensusPayload));
		int msgtype=payload.type;
		int sender = payload.nodeid;
		int receiver=payload.receiver;
                int temp_num_visited= payload.num_visited;
                double senderx = payload.senderx;
                double sendery = payload.sendery;
		int lev_pay = payload.level;
                double myx = GetNode()->GetObject<MobilityModel>()->GetPosition().x;
                double myy = GetNode()->GetObject<MobilityModel>()->GetPosition().y;
                int myid = (int)GetNode()->GetId();

  cout << "Node ID " << GetNode()->GetId() << " X = " << GetNode()->GetObject<MobilityModel>()->GetPosition().x << " Y = " << GetNode()->GetObject<MobilityModel>()->GetPosition().y << endl;
                cout << "msgtype " << msgtype << " sender " << sender << " receiver " << receiver << " temp_num_visited " << temp_num_visited << " senderx " << senderx << " sendery " << sendery << " my id " << myid <<  " myx " << myx << " myy " << myy << " Level payload " << lev_pay << endl;

		if (msgtype==0) //announce
		{
			if ((holder==0) && (requestpending==0))
			{

                                //here we want to change the timer depending on the node status (how many times it has been visited)
			        //criteria is if unvisited, it should reply first.  Send node with num_visited == 1, num_visited == 2 and so on.
                                ///i change request_send_Max from 0.13 to 0.2 so I have 200 msec.  2016/10/11
			        double min = 0;
                                double max = 0;
                                switch(num_visited){
				    case 0:  min = request_send_min; max = 0.045; break;
                                    case 1:  min = 0.05; max = 0.095; break;
                                    case 2:  min = 0.05; max = 0.095; break;
                                    case 3:  min = 0.1; max = 0.145; break;
                                    case 4:  min = 0.15; max = 0.2;
                                }

                                cout << "My num_visited is " << num_visited << " my min is: " << min << " my max is " << max << endl;
            
                        
				//evreq = Simulator::Schedule (Seconds(UniformVariable(request_send_min, request_send_max).GetValue()), &CensusApplication::SendRequest, this);
				level = payload.level;
	                        evreq = Simulator::Schedule (Seconds(UniformVariable(min, max).GetValue()), &CensusApplication::SendRequest, this);
				requestpending=1;
				announcer=sender;
				//NS_LOG_INFO("announcement from " << sender << " heard by " << myid << " weight " << weight << " at " << Simulator::Now().GetMicroSeconds());	
				
			}
			
			
			
			/* i am a holder and i just heard a holder of a different token*/
			if ((reached==1)&&(holder==1))
			{
				cout << "My id "<< GetNode()->GetId() << " holder heard from "<< sender << endl;
				//holder_nearby=1;
				//NS_LOG_INFO("My id "<< GetNode()->GetId() << " holder heard from "<< sender);
			}
					
				
		}
		
		if (msgtype==1) //request
		{
			if ((holder==1)&&(myid == receiver)) // i am the holder and the request is for my token
			{
				requestor_id[num_token_requestors]=sender;
				requestor_weight[num_token_requestors]=payload.weight;
                                //debuggin
                                cout << "Node " << GetNode()->GetId() << "requested token with num_visited " << payload.num_visited << endl;
                                requestor_num_visited[num_token_requestors]=payload.num_visited;
				std::pair<int, int> ID_level = CensusNodeInfo::getCellId(senderx, sendery, lev_pay);
				requestor_cellId[num_token_requestors] = ID_level.first;
				requestor_level[num_token_requestors] = ID_level.second; 
				num_token_requestors++;	
			}
			
			if ((holder==1)&&(myid!=receiver)) // I am overhearing request message for other token holder.
			{
				cout << "My id "<< GetNode()->GetId() << " request for another holder heard from "<< receiver << endl;
				//holder_nearby=1;
				//NS_LOG_INFO("My id "<< GetNode()->GetId() << " request for another holder heard from "<< receiver);
			}
			
			
			if (announcer==receiver && requestpending==1 && requestsent==0 && payload.weight>=weight) // I overheard request for the same token I am trying to request....
			{

                                //if requestor is vsited and i am also visited, i cancel my request if i am visited more times

                                // My current logic only suppresses token request from visited node.
                                //We need to let unvisited node suppresses token request if they heard other unvisited node requesting token.
                                //if(payload.weight==0 && weight == 0 && num_visited >= temp_num_visited) {

                                if(num_visited >= temp_num_visited){
				    Simulator::Cancel(evreq);
				    if(requestpending !=0){
                                         cout << "I am canceling token request as I overhead token request from node with lesser num_visted:" << num_visited << "tem_num_visited:" << temp_num_visited << endl;
                                         CensusNodeInfo::cancelRequest();
                                    }
				    requestpending=0;
				    announcer=-1;
				//NS_LOG_INFO("request cancelled ");
				}
			}
			
		}
		
		
		if (msgtype==2) //token reply
		{
			//NS_LOG_INFO("Token from " << sender << " heard by " << myid << " request sent " << requestsent);	
			
			if ((holder==1)&&(receiver!=myid))
			{
				cout << "My id "<< GetNode()->GetId() << " transfer from another holder "<< sender << " in progress" << endl;
				//holder_nearby=1;
				//NS_LOG_INFO("My id "<< GetNode()->GetId() << " transfer from another holder "<< sender << " in progress");
			}
			
			
			if (requestsent==1 && announcer==sender)
			{
				if ((receiver==myid))
				{
                                       
                                        num_visited++;
                                        CensusNodeInfo::update_visited_num(GetNode()->GetId());
                                        //debuging
                                        cout << "Node " << myid << " got token and updating num_visited " << num_visited << endl;

					holder=1;
					reached=1;
					ack_send_pending=1;
					ack_receiver=sender;
					ack_mode=1;
//Sowm:					int cellID = CensusNodeInfo::getCellId(myx,myy);
				        //CensusNodeInfo::updateCount_perCell(cellID, GetNode()->GetId());
				        /*if(payload.level == levelCount)
					{
						cout << "Level = " << payload.level << " MYX = " << myx << " MYY = " << myy << endl;
				                CensusNodeInfo::updateCount(GetNode()->GetId());
					}
					else*/

						cout << "********%%%%%%%%%%**********" << endl;
						cout << "MYX = " << myx << " MYY = " << myy << " LEVEL = " << payload.level << endl;
						std::pair<int, int> ID_level = CensusNodeInfo::getCellId(myx, myy, payload.level);
						cout << "ID_level " << ID_level.first << " " << ID_level.second << endl;
						bool ret = CensusNodeInfo::updateCount_perCell(GetNode()->GetId(), ID_level);
						if (ret == true)
						{
							ResultFlood();
						}
					CensusNodeInfo::updateWeight(GetNode()->GetId(),weight);
					weight=0;
					CensusNodeInfo::updateHolder(GetNode()->GetId(), ID_level.second);
					//NS_LOG_INFO(" Node " <<myid << " setting sendannounce at " << Simulator::Now().GetMilliSeconds()); 
  					Simulator::Schedule (Seconds(UniformVariable(token_send_time+0.01, token_send_time+0.05).GetValue()), &CensusApplication::SendAnnounce, this);
  					announce_trials=1;
  					requestsent=0;
  					requestpending=0;
  					announcer=-1;
  				
  					//NS_LOG_INFO("Token received by "<<myid<< " " << GetNode()->GetObject<MobilityModel>()->GetPosition().x << " " << GetNode()->GetObject<MobilityModel>()->GetPosition().y);
  					
  					Simulator::Schedule (Seconds(UniformVariable(0.001,0.0025).GetValue()), &CensusApplication::SendTokenAck, this);
				}
				else
				{
					//receiver is not my id; token went to someone else
					if (rqw_set==1)
					{
						//cancel requestwait
						Simulator::Cancel(evrqw);
						rqw_set=0;
					}
					requestpending=0;
					requestsent=0;
					announcer=-1;
					
				}
			}
			
			if (ack_mode==1 && sender==ack_receiver && receiver==myid && ack_send_pending==0)
			{
				//ack was not rceeived by token sender; token is being resent
				ack_send_pending=1;
				Simulator::Schedule (Seconds(UniformVariable(0.001,0.0015).GetValue()), &CensusApplication::SendTokenAck, this);
				//NS_LOG_INFO("Token repeat received by "<<myid<< " " << GetNode()->GetObject<MobilityModel>()->GetPosition().x << " " << GetNode()->GetObject<MobilityModel>()->GetPosition().y);
				
			}
		
		}
		
		if (msgtype==4) //token ack
		{
			if (safety_mode==1 && sender==safety_receiver && holder==1 && receiver==myid)
			{
				holder=0;
				safety_mode=0;
				safety_receiver=-1;
				CensusNodeInfo::removeHolder(GetNode()->GetId());
				//announce_sent=0;
			}
			
			if (holder==1)
			{
				cout << "Heard another holder in the vicinity" << endl;
				//holder_nearby=1; //heard another holder in the vicinity
			}
		
		}
		
		
		if (msgtype==6)
		{
		
			NS_LOG_INFO("Node " << myid << " received message from " <<sender);
		}
		if(msgtype == 12){ //push 
                    //add information to set
                    cout << "----------------------------Got push packet from node " << sender << " ----------------------------" << endl;
                    pushed_information.insert(sender);
                    set<int>::iterator it = pushed_information.begin();             
                    cout << "Node " << myid << " has " << pushed_information.size() << " information.  ";
                    for(unsigned int index=0; index < pushed_information.size() ; index++){
                        it = pushed_information.begin(); 
                        advance(it,index);
                        int id = *it;
                        cout << id << " ";
                    }
                    cout << endl;
                    CensusNodeInfo::Add_Information(myid,sender);
                    
                }

                if(msgtype == 20)
                {//Result Flood
                        struct broadcastLimit blim;

                        double x = GetNode()->GetObject<MobilityModel>()->GetPosition().x;
                        double y = GetNode()->GetObject<MobilityModel>()->GetPosition().y;

			std::pair<int, int> ID_level = CensusNodeInfo::getCellId(senderx, sendery, lev_pay);
			cout << " Cell ID = " << ID_level.first << endl;
			CensusNodeInfo::getBroadcastRange(ID_level.first);

                        CensusNodeInfo::getBroadcastLimit(&blim);
                        cout << blim.xmin << " " << blim.ymin << " " << blim.xmax << " " << blim.ymax << endl;
                        if((flood_result == true) && (x >= blim.xmin && y >= blim.ymin) && (x < blim.xmax && y < blim.ymax))
                        {
                                cout << "2 sender " << sender << " my id " << myid << endl;
                                flood_result = false;
                                Simulator::Schedule (Seconds(UniformVariable(0.25, 0.5).GetValue()), &CensusApplication::SendResultFlood, this);
                        }
                        else if(flood_result == false)
                        {
                                cout << "\nFlood Result = FALSE " << endl;
                        }
                        else if (x < blim.xmin || y < blim.ymin ) 
                        {
                                cout << "\nX and Y are below Minimum Value" << endl;
                        }
                        else if (x >= blim.xmax || y >= blim.ymax)
                        {
                                cout << "\nX and Y are above Maximum Value" << endl;
                        }
                }
	}
}


void CensusApplication::SendToken()
{
  cout << "Node ID " << GetNode()->GetId() << " X = " << GetNode()->GetObject<MobilityModel>()->GetPosition().x << " Y = " << GetNode()->GetObject<MobilityModel>()->GetPosition().y << endl;

if (num_token_requestors>0)
{
	cout << "num_token_requestors" << num_token_requestors << endl;
	CensusPayload payload=CensusPayload();
	payload.nodeid=(int)((m_socket->GetNode())->GetId());
	payload.type=2; //token reply
	payload.level = level;

	double senderx = GetNode()->GetObject<MobilityModel>()->GetPosition().x;
	double sendery = GetNode()->GetObject<MobilityModel>()->GetPosition().y;

	std::pair<int, int> ID_level_Sender = CensusNodeInfo::getCellId(senderx, sendery, level);
	int sender_cellId = ID_level_Sender.first;
	int sender_level = ID_level_Sender.second;

	int curr_receiver=-1;
        int curr_num_visited = -1;
        int curr_cellID = -1;
        int curr_level = -1;

        //store first requestor's as current smallest num_visited
        curr_num_visited = requestor_num_visited[0];
        curr_receiver = requestor_id[0];
        curr_cellID = requestor_cellId[0];
        curr_level = requestor_level[0];
	cout << "NOdeID " << payload.nodeid << " Level " << level << endl;
	
        cout << "set num_visited to " << curr_num_visited << " requestor is " << curr_receiver << endl;
/*        for (int i=1; i<num_token_requestors; i++)
	{
		if (requestor_num_visited[i] < curr_num_visited)
		{
			curr_receiver=requestor_id[i];
			//curr_best_weight=requestor_weight[i];
                        curr_num_visited=requestor_num_visited[i];
                        cout << "updating num_visited to " << curr_num_visited << " requestor is " << curr_receiver << endl;
		}
	}*///Sowmya Commented

        bool flag = false;

        for (int i=0; i<num_token_requestors; i++)
        {       
		cout << "Sender Level & ID " << sender_level << "-" << sender_cellId << endl;
		cout << "Requestor Level & ID " << requestor_level[i] << "-" << requestor_cellId[i] << endl;
		if(requestor_level[i] == sender_level && requestor_cellId[i] == sender_cellId)
                {
			if(requestor_num_visited[i] < curr_num_visited)
			{
                       		curr_receiver=requestor_id[i];
                       		curr_num_visited=requestor_num_visited[i];
				flag = true;
				cout << "******** Receiver updated and breaking the loop ********" << endl;
				break;
			}
			curr_receiver=requestor_id[i];
                        curr_num_visited=requestor_num_visited[i];
			cout << "******** Receiver updated ********" << endl;
			flag = true;
		}
        }
	cout << "Current Receiver = " << curr_receiver << endl;

        //now all requestor's num_visited is evaluated and curr_num_visited should be the smallest count.
	if (curr_receiver!=-1 && flag == true)
	{
		payload.receiver=curr_receiver;
		//cellID_Passes[currCellId] ++;
		//map<int, int>::iterator it2 = cellID_Passes.begin();
		//cout << "CellID_passes->first " << it2->first << " CellID_passes->second " << it2->second << endl;
		Ptr<Packet> pkt = Create<Packet> ((uint8_t*) &payload,sizeof(CensusPayload));
	
		m_txTrace (pkt);
	 	m_socket->Send (pkt);
	  	m_lastStartTime = Simulator::Now ();
  	
  		
  		//NS_LOG_INFO("Main Token sent from "<< GetNode()->GetId() <<" " << GetNode()->GetObject<MobilityModel>()->GetPosition().x << " " << GetNode()->GetObject<MobilityModel>()->GetPosition().y << " to "<< curr_receiver << " with weight "<< curr_best_weight);
  		
  		announce_trials=0;
  		Simulator::Cancel(ev2); //cancel announce wait event
  		
  					
  		safety_mode=1; //in safety mode, u resend token to reduce chance of token loss
  		safety_sends=0;
  		safety_receiver=curr_receiver;
  		Simulator::Schedule (Seconds((0.003)), &CensusApplication::SendSafety, this);
	}
	else
	{
		//announce_sent=0;
	}

        if(flag == false)
        {
                NumTokenRequests[GetNode()->GetId()]++;
                int MaxReqTrial = CensusNodeInfo::getMaxTokenPasses(0);

                map<int, int>::iterator it = NumTokenRequests.begin();
                cout << "MaxReqTrial " << MaxReqTrial << " Token Request Values " << endl;
                for(; it != NumTokenRequests.end();++it)
                {
                        cout << it->first << " " << it->second <<endl;
                }
                it = NumTokenRequests.begin();
                for(; it != NumTokenRequests.end();++it)
                {
                        if((int)GetNode()->GetId() == it->first)
                        {
                                if(it->second <= MaxReqTrial)
                                {
                                        cout << "New Send Announce " << payload.nodeid << endl;
                                        holder=1;
                                        reached=1;
                                        //ack_send_pending=1;
                                        //ack_receiver=sender;
                                        //ack_mode=1;
//Sowm:                                        int cellID = CensusNodeInfo::getCellId(x,y);
                                        //CensusNodeInfo::updateCount_perCell(cellID, GetNode()->GetId());
                                        /*if(payload.level == levelCount)
                                        {
                                                CensusNodeInfo::updateCount(GetNode()->GetId());
                                        }
                                        else*/
						int x = m_socket->GetNode()->GetObject<MobilityModel>()->GetPosition().x;
						int y = m_socket->GetNode()->GetObject<MobilityModel>()->GetPosition().y;
                                                std::pair<int, int> ID_level = CensusNodeInfo::getCellId(x, y, payload.level);
                                                cout << "ID_level " << ID_level.first << " " << ID_level.second << endl;

							bool ret = CensusNodeInfo::updateCount_perCell(GetNode()->GetId(), ID_level);
							if (ret == true)
							{
								ResultFlood();
							}
                                        //CensusNodeInfo::updateCount(GetNode()->GetId());
                                        CensusNodeInfo::updateWeight(GetNode()->GetId(),weight);
                                        weight=0;
                        //                CensusNodeInfo::updateHolder(GetNode()->GetId(), ID_level.second);

                                        //Send Announce
                                        Simulator::Schedule (Seconds(UniformVariable(0.05, 0.1).GetValue()), &CensusApplication::SendAnnounce, this);
                                        announce_trials=1;
                                        requestsent=0;
                                        requestpending=0;
                                        announcer=-1;

                                }
                                else
                                {
                                        cout << "NumTokenRequests cleared for " << GetNode()->GetId() << endl;
//Sowm:                                        int cellId = CensusNodeInfo::getCellId(GetNode()->GetObject<MobilityModel>()->GetPosition().x, GetNode()->GetObject<MobilityModel>()->GetPosition().y);
                                        //CensusNodeInfo::getBroadcastRange(cellId);
					// Flood the result
                                        Simulator::Schedule (Seconds(UniformVariable(0.1, 0.2).GetValue()), &CensusApplication::SendResultFlood, this);
                                        //Delete the map entry for that Node and Pass the token to default.
                                        NumTokenRequests.erase(GetNode()->GetId());
					//cellID_Passes.erase(currCellId);
                                }
                        }
                }

        }
}
}

void CensusApplication::ResultFlood()
{
  cout << "Node ID " << GetNode()->GetId() << " X = " << GetNode()->GetObject<MobilityModel>()->GetPosition().x << " Y = " << GetNode()->GetObject<MobilityModel>()->GetPosition().y << endl;
	Simulator::Schedule (Seconds(UniformVariable(0.1, 0.2).GetValue()), &CensusApplication::SendResultFlood, this);	
}

void CensusApplication::SendResultFlood()
{
  cout << "Node ID " << GetNode()->GetId() << " X = " << GetNode()->GetObject<MobilityModel>()->GetPosition().x << " Y = " << GetNode()->GetObject<MobilityModel>()->GetPosition().y << endl;
        cout << "#####RESULT FLOOD#######" << endl;
        CensusPayload payload=CensusPayload();
        payload.nodeid=(int)((m_socket->GetNode())->GetId());
        payload.type=20; //Result Flood
	payload.level=level;
        payload.senderx = m_socket->GetNode()->GetObject<MobilityModel>()->GetPosition().x;
        payload.sendery = m_socket->GetNode()->GetObject<MobilityModel>()->GetPosition().y;
        Ptr<Packet> pkt = Create<Packet> ((uint8_t*) &payload,sizeof(CensusPayload));

        m_txTrace (pkt);
        m_socket->Send (pkt);
        m_lastStartTime = Simulator::Now();
}

void CensusApplication::SendTokenAck()
{

  cout << "Node ID " << GetNode()->GetId() << " X = " << GetNode()->GetObject<MobilityModel>()->GetPosition().x << " Y = " << GetNode()->GetObject<MobilityModel>()->GetPosition().y << endl;
	if (ack_send_pending==1)
	{
		CensusPayload payload=CensusPayload();
		payload.nodeid=(int)((m_socket->GetNode())->GetId());
		payload.type=4; //token ack
		payload.receiver=ack_receiver;
		Ptr<Packet> pkt = Create<Packet> ((uint8_t*) &payload,sizeof(CensusPayload));

		m_txTrace (pkt);
 		m_socket->Send (pkt);
  		m_lastStartTime = Simulator::Now ();
  		//NS_LOG_INFO("Token ack sent from "<< GetNode()->GetId() << " to "<< ack_receiver );
  		
  		ack_send_pending=0;
  	}
  		

}

void CensusApplication::SendSafety()
{

    if (safety_mode==1)
    {
		
  		safety_sends++;
  		if (safety_sends<33)
  		{
  		
  			CensusPayload payload=CensusPayload();
			payload.nodeid=(int)((m_socket->GetNode())->GetId());
			payload.type=2; //token reply
			payload.receiver=safety_receiver;
			Ptr<Packet> pkt = Create<Packet> ((uint8_t*) &payload,sizeof(CensusPayload));

			m_txTrace (pkt);
 			m_socket->Send (pkt);
	  		m_lastStartTime = Simulator::Now ();
  			//NS_LOG_INFO("Token resent from "<< GetNode()->GetId() <<" " << GetNode()->GetObject<MobilityModel>()->GetPosition().x << " " << GetNode()->GetObject<MobilityModel>()->GetPosition().y << " to "<<safety_receiver );
  			Simulator::Schedule (Seconds((0.003)), &CensusApplication::SendSafety, this);
  		}
  		else
  		{
  		
  			//this is the point where the current holder continues to announce token 
  			// if the ack was lost, a duplicate token will be created
  			// hence at this point a check point should be done, i.e. current token value should be exfiltrated and token count should be reset
  			// that way there will be no duplicate counting
  			if (CensusNodeInfo::checkHolder(safety_receiver)==0) 
  			// we are checking through the back channel if the token was lost; this is possible only because we have global state access; in real deployment this is not possible and extra tokens may be created; however, we incorporate this check here so that we ensure that for results in the paper, the number of tokens are maintained.
  			{
  				Simulator::Schedule (Seconds(UniformVariable(token_send_time+0.01, token_send_time+0.05).GetValue()), &CensusApplication::SendAnnounce, this);
  			}
  			else
  			{
  				holder=0;
  				CensusNodeInfo::removeHolder(GetNode()->GetId());
  			}
  			//holder=0;
			safety_mode=0;
			safety_receiver=-1;
			//CensusNodeInfo::removeHolder(GetNode()->GetId());
  			
  			//NS_LOG_INFO("Token not successfully transferred from " << GetNode()->GetId() << " to " << safety_receiver);
  			//NS_LOG_INFO("A duplicate token may have been created; Hence taking token check-point to avoid double counting ");
  			//NS_LOG_INFO("A token may be lost. hence checkpointing to avoid data loss ");
  			CensusNodeInfo::updateCheckpoint(GetNode()->GetId());
  			
  			//announce_sent=0;
  		
  		
  		}
  	}

}


void CensusApplication::SendRequest()
{

	if (requestpending==1)  //if request is till pending and has not been cancelled by a received token or another request
	{   
		CensusPayload payload=CensusPayload();
		payload.nodeid=(int)((m_socket->GetNode())->GetId());
		payload.type=1; //request
		payload.receiver=announcer;
		payload.weight=weight;
		payload.senderx = GetNode()->GetObject<MobilityModel>()->GetPosition().x;
		payload.sendery = GetNode()->GetObject<MobilityModel>()->GetPosition().y;
		payload.level = level;
                //debugging
                cout << "Sending request" << endl;
                cout << "node id " << payload.nodeid <<  "num_visited: " << num_visited << endl;
                payload.num_visited = num_visited;
		Ptr<Packet> pkt = Create<Packet> ((uint8_t*) &payload,sizeof(CensusPayload));
		requestsent=1;

	  	m_txTrace (pkt);
 		m_socket->Send (pkt);
  		m_lastStartTime = Simulator::Now ();
  		CensusNodeInfo::updateRequests(GetNode()->GetId(), level);
  		
  		//NS_LOG_INFO("Request for token from " << announcer << " sent by " << GetNode()->GetId() << " with weight " << weight);
  		
  		evrqw = Simulator::Schedule (Seconds(0.27), &CensusApplication::RequestWait, this);
  		rqw_set=1;
  	}
 
}

void CensusApplication::RequestWait()  //no reply received
{
	if (requestpending==1)
	{
	requestsent=0;
	requestpending=0; //disable requesting for tokens until timeout
	//NS_LOG_INFO("Request for token from " << announcer << " cancelled by " << GetNode()->GetId());
	announcer=-1; //announcer is void now
	}
	rqw_set=0;
}

void CensusApplication::StopApplication () // Called at time specified by Stop
{
  //NS_LOG_FUNCTION_NOARGS ();

  CancelEvents ();
  if(m_socket != 0)
    {
      m_socket->Close ();
    }
  else
    {
      NS_LOG_WARN ("VanetApplication found null socket to close in StopApplication");
    }
}

void CensusApplication::CancelEvents ()
{
  //NS_LOG_FUNCTION_NOARGS ();

  if (m_sendEvent.IsRunning ())
    { // Cancel the pending send packet event
      Time delta (Simulator::Now () - m_lastStartTime);
    }
  Simulator::Cancel (m_periodicSending);
}

void CensusApplication::ConnectionSucceeded (Ptr<Socket>)
{
  //NS_LOG_FUNCTION_NOARGS ();
  cout << "CensusApplication, Connection Succeeded" << endl;
  //m_connected = true;
  //ScheduleStartEvent ();
}

void CensusApplication::ConnectionFailed (Ptr<Socket>)
{
  //NS_LOG_FUNCTION_NOARGS ();
  cout << "CensusApplication, Connection Failed" << endl;
}

} // Namespace ns3

