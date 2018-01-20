#ifndef VANET_APPLICATION_H
#define VANET_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable.h"
#include "ns3/traced-callback.h"
#include "census-payload.h"
#include "ns3/census-globals.h"
#include <set>

namespace ns3 {

class Address;
class RandomVariable;
class Socket;


class CensusApplication : public Application 
{
public:
  static TypeId GetTypeId (void);

  CensusApplication ();

  virtual ~CensusApplication();

  /**
   * \param maxBytes the total number of bytes to send
   *
   * Set the total number of bytes to send. Once these bytes are sent, no packet 
   * is sent again, even in on state. The value zero means that there is no 
   * limit.
   */
  void SetMaxBytes (uint32_t maxBytes);
  
  int GetTurnNumber();
  //int TurnNumber;

  /**
   * \return pointer to associated socket
   */
  Ptr<Socket> GetSocket (void) const;

  struct broadcastLimit
  {
        double xmin;
        double ymin;
        double xmax;
        double ymax;
  };

protected:
  virtual void DoDispose (void);


private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  //helpers
  void CancelEvents ();

/*  void Construct (Ptr<Node> n,
                  std::string tid,
                  const RandomVariable& newtxtime,
                  uint32_t size);
*/


  // Event handlers
  void SendAnnounce();
  void AnnounceWait();
  void receivingMsg(Ptr<Socket> socket);
  //void receivingToken(Ptr<Socket> socket);
  void ResultFlood();
  void SendRequest();
  void SendSafety();
  void SendToken();
  void SendTokenAck();
  void RequestWait();
  void SendResultFlood();
  void ResetWeight();
  double getLevel();
  void generateTokens(int level);
  void Push();
  Ptr<Socket>     m_socket;       // Associated socket
  //bool            m_connected;    // True if connected
  RandomVariable  m_newTxTime;    // rng for New Transmission Time
  DataRate        m_cbrRate;      // Rate that data is generated
  uint32_t        m_pktSize;      // Size of packets
  Time            m_lastStartTime; // Time last packet sent
  uint32_t        m_maxBytes;     // Limit total number of bytes sent
  //uint32_t        m_totBytes;     // Total bytes sent so far
  EventId         m_periodicSending;     // Event id for periodic position sending
  EventId         m_sendEvent;    // Eventid of pending "send packet" event
  bool            m_sending;      // True if currently in sending state
  TypeId          m_tid;
  TracedCallback<Ptr<const Packet> > m_txTrace;

  //double othersTimeStamp[MaxNodes];
  //int othersRecvCount[MaxNodes];
  double weight;
  int seqnum;
  int MaxNodes;
  int numtoks;
  int levelCount;
  int TurnNumber;
  int holder;
  int announce_trials;
  int reached;
  int requestpending;
  int requestsent;
  int announcer;
  int safety_mode;
  int safety_receiver;
  int safety_sends;
  int holder_nearby;
  int num_token_requestors;
  int requestor_id[max_token_requestors];
  int requestor_cellId[max_token_requestors];
  int requestor_level[max_token_requestors];
  double requestor_weight[max_token_requestors];
  int requestor_num_visited[max_token_requestors];
  int num_split_requestors;
  int rqw_set;
  int ack_send_pending;
  int ack_mode;
  int ack_receiver;
  double sr_min;
  double sr_max;
  int num_visited; //number of time this node holds token exclusively.
  std::set<int> pushed_information;
  unsigned int starter;
  unsigned int level_starter;

  std::map <int, int> NumTokenRequests; // <Node,NumberOfRequest>  
  std::map <int, int> cellID_Passes; // <cellID,Passes>  
  bool flood_result;
  int level;

  Time waitInterval;
  EventId ev;
  EventId ev2;
  EventId ev3;
  EventId evrb;
  EventId evreq;
  EventId evrqw;
  //TypeId a_tid;
  //Ptr<Socket> a_socket;
  //Ptr<Socket> r_socket;      
  void ConnectionSucceeded(Ptr<Socket> socket);
  void ConnectionFailed(Ptr<Socket> socket);

  //void ForwardPackets(VanetPayload vp);
};

} // namespace ns3

#endif /* VANET_APPLICATION_H */
