#include <fstream>
#include "ns3/core-module.h"
#include "census-nodeinfo.h"
#include "census-application.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/log.h"
#include "ns3/mobility-module.h"
#include "ns3/gnuplot.h"
#include <set>
#include <time.h>
NS_LOG_COMPONENT_DEFINE ("CensusNodeInfo"); 

namespace ns3 {
     static NodeContainer* ncp;
     static int MaxNodes;
     static Ipv4InterfaceContainer* naddress;
	 static int counted[MN];   
         static int duplicatecount[MN];
	 static int terminated_tokens;
	 static int holder[MN];
//	 static int tokentransfers;
	 static int totalsplits;
	 static int totalsplitrebroadcasts;
//	 static int totalrequests;
	 static int totaldiffusees;
//	 static int token_announcements;
	 static double countsofar;
	 static int totalcps;
	 static int numtoks;
	 static ofstream filestr1;
	 static ofstream filestr2;
         static ofstream coordinate;
         static ofstream duplicate;
	 static int timeaxis[snapcounts];
	 static double percentage_counted[snapcounts];
	 static int total_transfers[snapcounts];
	 static int total_diffusion[snapcounts];
	 static int visited_num[MN];
	 bool flag50, flag75,flag90, flag100;
	 map <int, bool> terminate_flag;
	 map <int, int> counted_cell;
	 map <int, int> tokentransfers; // level --> count
	 map <int, int> token_announcements; // level --> count
	 map <int, int> totalrequests; // level --> count
         vector<set<int> > whole_world;
	 static int cancel_request;

	 static double xBound;
         static double yBound;
         static double levelCount;
         const double range = 150;
         static int continuous_number;
         static int result_count_l2;
         static int result_count_l1;
         static int result_count_l0;
	 time_t start;
	 double seconds_since_start;
         struct cellInfo
         {
                int level;
                int cellId;
                double x1;
                double x2;
                double y1;
                double y2;
         };

        struct broadcastLimit
        {
                double xmin;
                double ymin;
                double xmax;
                double ymax;
        };


	map <int, cellInfo> cellinfoMap;
        map <int, int> CensusNodeInfo::MaxTokenPassesPerLevel;
        map <int, int> CensusNodeInfo::levelIncrement;
	map <int, int> CensusNodeInfo::CountOfCellsPerlevel;
	map <int, int> CensusNodeInfo::token_cellID;// Cell ID Vs Token/Node Number
        struct broadcastLimit broadcastLimit;
	pair <int, int> cellID_level;

void CensusNodeInfo::getNodecontainer(NodeContainer* nc, int mn, int runid, int ntoks)
{
        whole_world.resize(mn);
	ncp=nc;
	MaxNodes=mn;	
	numtoks = ntoks;
	if (ntoks==0)
	{
		std::ostringstream sstreamb;
		std::ostringstream sstreamc;
    	    sstreamb << "src/census/Data_Nodes_" << mn << "_Sqrt_tokens_Run_" << runid << ".txt";
    	std::string FileAsString = sstreamb.str();
    	sstreamc << "src/census/Data_Nodes_" << mn << "_Sqrt_tokens_Run_" << runid << "token_covered" << ".txt";
    	std::string FileAsString2 = sstreamc.str();
		//filestr1.open ("src/census/trial.txt");
		filestr1.open(FileAsString.c_str());
		filestr2.open(FileAsString2.c_str());
	}	
	else
	{
		std::ostringstream sstreamb;
		std::ostringstream sstreamc;
    	sstreamb << "src/census/Data_Nodes_" << mn << "_Log_Tokens_" << ntoks << "_Run_" << runid << ".txt";
    	std::string FileAsString = sstreamb.str();
	sstreamc << "src/census/Data_Nodes_" << mn << "_Sqrt_tokens_Run_" << runid << "token_covered" << ".txt";
    	std::string FileAsString2 = sstreamc.str();
		//filestr1.open ("src/census/trial.txt");
		filestr1.open(FileAsString.c_str());
		filestr2.open(FileAsString2.c_str());
	}
	//create file to store coordinate information
	std::ostringstream sstreamb;
        std::ostringstream sstreamc;
        sstreamb << "src/census/Data_Nodes_" << mn << "_coordinate_info_Run_" << runid << ".txt";
        std::string coordinateFileAsString = sstreamb.str();
        coordinate.open(coordinateFileAsString.c_str());

        std::ostringstream sstreamgg;
        sstreamgg << "src/census/Data_Nodes_" << MaxNodes << "_duplicate_" << runid << ".txt";
        std::string FileAsStringq = sstreamgg.str();
        duplicate.open(FileAsStringq.c_str());

	for (int i=0; i<snapcounts; i++)
	{
		timeaxis[i]=0;
		percentage_counted[i]=0;
		total_transfers[i]=0;
		total_diffusion[i]=0;
	}
 	for (int i=0; i<MaxNodes; i++)
 	{
 	        counted[i]=0;
                duplicatecount[i] =0;
                holder[i]=0;
 		
 	}
 	terminated_tokens=0;
 	//tokentransfers=0;
 	totalsplits=0;
 	totaldiffusees=0;
 	//token_announcements=0;
 	totalsplitrebroadcasts=0;
 	//totalrequests=0;
 	totalcps=0;
 	countsofar=0.0;
 	for(unsigned int index =0; index < MN; index++){
            visited_num[index] =0;
        }
        flag50 = true;
        flag75 = true;
        flag90= true;
        flag100 = true;

        for(unsigned int index = 0; index <continuous_number; index++)
        {
                terminate_flag[index] = true;
        }
}

int CensusNodeInfo::getMaxNodes()
{
	return MaxNodes;
}

int CensusNodeInfo::getCountsofar()
{
	int tc=0;
	for (int i=0; i< MaxNodes; i++)
	{
		if (counted[i]==1)
		{
			tc++;
		}	
	}
	return tc;
}

int CensusNodeInfo::checkHolder(int i)
{
	if (holder[i]==1) 
		return 1;
	else
		return 0;
}

int CensusNodeInfo::getToks()
{
	return numtoks;
}

void CensusNodeInfo::getAddresscontainer(Ipv4InterfaceContainer* ni)
{
	naddress=ni;
}

void CensusNodeInfo::updateWeight(int id, double wt)
{
	countsofar+=wt;
}

bool CensusNodeInfo::updateCount_perCell(int nodeid, pair <int, int> ID_level)
{
	bool ret_val = false;
	cout << "Update Count per Cell" << endl;
	cout << ID_level.first << " " << ID_level.second << endl;
	cout << "Node ID = " << nodeid << endl;
	counted_cell[ID_level.first]++;

	set<int>::iterator it = whole_world[nodeid].begin();
        for(unsigned int index=0; index < whole_world[nodeid].size(); index++)
	{
            it = whole_world[nodeid].begin();
            std::advance(it,index);
            int node = *it;
            cout << node << " ";
            counted_cell[ID_level.first]++;
        }
        cout << endl;

        map<int, int>::iterator it1 = counted_cell.begin();

        for(; it1!=counted_cell.end();++it1)
        {
                cout << it1->first << " " << it1->second << endl;
        }
	
        for(int i = 1; i < continuous_number; i++)
	{
		//cout << "Maximum Nodes = " << MaxNodes << endl;
		cout << "Counted Cell[" <<i << "] " << counted_cell[i]  << endl;
		if(levelCount == 3)
		{
			cout << "############################# i = " << i << endl;
			if((i==1||i == 22|| i == 43 || i == 64))
			{
				if(counted_cell[i] >= 1250)
				{
					counted_cell[i] = 0;
					result_count_l2++;
					cout << i  << " 11 Aggregation Done for level " << ID_level.second << endl;
					cout << "11 Result Count = " << result_count_l2 << endl;
				}
			}
			else if((i == 2 ||i == 7 ||i == 12 ||i == 17 ||i == 23 ||i == 28 ||i == 33 ||i == 38 ||i == 44 ||i == 49 ||i == 54 ||i == 59 ||i == 65 ||i == 70 ||i == 75 ||i == 80))
			{
				if(counted_cell[i] >= 313)
				{
					counted_cell[i] = 0;
					result_count_l1++;
					cout << i  << " 12 Aggregation Done for level " << ID_level.second << endl;
					cout << "12 Result Count = " << result_count_l1 << endl;
				}
			}
			else if (counted_cell[i] >= 79)
			{
				counted_cell[i] = 0;
				result_count_l0++;
				cout << i  << " 13 Aggregation Done for level " << ID_level.second << endl;
				cout << "13 Result Count = " << result_count_l0 << endl;
			}
		}
		else if(levelCount == 2)
		{
			if((i==1||i == 6|| i == 11 || i == 16))
			{
				if(counted_cell[i] >= 875)
				{
					counted_cell[i] = 0;
					result_count_l1++;
					cout << i  << " Loop 2 Aggregation Done for level " << ID_level.second << endl;
					cout << "Loop2 Result Count = " << result_count_l1 << endl;
				}
			}
			else if (counted_cell[i] >= 219)
			{
				counted_cell[i] = 0;
				result_count_l0++;
				cout << i  << " 22 Aggregation Done for level " << ID_level.second << endl;
				cout << "22 Result Count = " << result_count_l0 << endl;
			}
		} 
		else if (levelCount == 1)
		{
			if(counted_cell[i] >= 50)
			{
				counted_cell[i] = 0;
                                result_count_l0++;
                                cout << i  << " 31 Aggregation Done for level " << ID_level.second << endl;
                                cout << "31 Result Count = " << result_count_l0 << endl;
			}
		}
	}
	return ret_val;
}

void CensusNodeInfo::updateCount(int id)
{
        cout << "----------------- Update Count-------------------" << endl;
	//to get idea of how many duplicated information we obtained, I am updating code to increment counted value;
        //indicate that the token receiver is visited here...
        counted[id]=1;
        //duplicatecount[id]++;
	cout << "This node: " << id << " has ";

        //As each node has multiple information, we need to update all the information each node has.  
        set<int>::iterator it = whole_world[id].begin();
        for(unsigned int index=0; index < whole_world[id].size(); index++){
            it = whole_world[id].begin();
            std::advance(it,index);
            int node = *it;
            cout << node << " "; 
            counted[node] = 1;
            duplicatecount[node]++;   //increment number of times, the information is collected for node, "node".
        }
        cout << endl;
}

void CensusNodeInfo::updateRequests(int id, int level)
{
	totalrequests[level]++;
}

void CensusNodeInfo::updateCheckpoint(int id)
{
	totalcps++;
}

void CensusNodeInfo::cancelRequest(){
        cancel_request++;
}

void CensusNodeInfo::updateHolder(int id, int level)
{
	if (holder[id]==1) 
	{
		NS_LOG_INFO("somthing wrong ");
	}
	holder[id]=1;
	tokentransfers[level]++;
}

void CensusNodeInfo::updateSplits(int id)
{
	
	totalsplits++;
}

void CensusNodeInfo::updateSplitRebroadcasts(int id)
{
	
	totalsplitrebroadcasts++;
}

void CensusNodeInfo::Add_Information(int myid, int sender)
{
    cout << "----------------- Add Information-------------------" << endl;
    whole_world[myid].insert(sender);
    cout << "whole_world[" << myid << "] has " << whole_world[myid].size() << " nodes:   ";
    set<int>::iterator it = whole_world[myid].begin();
    for(unsigned int index =0; index < whole_world[myid].size(); index++){
        it = whole_world[myid].begin();
        std::advance(it,index);
        int id = *it;      
        cout << id << " ";
    }
    cout << endl;
}  

void CensusNodeInfo::updateDiffusees(int id)
{
	
	totaldiffusees++;
}

void CensusNodeInfo::removeHolder(int id)
{
	
	holder[id]=0;
}

void CensusNodeInfo::updateTerminated(int id)
{
	terminated_tokens++;
}

void CensusNodeInfo::update_visited_num(int id)
{
    visited_num[id]++;
}

void CensusNodeInfo::updateSA(int id, int level)
{
	token_announcements[level]++;
}

double CensusNodeInfo::getXlocation(int id)
{
	//NS_LOG_INFO("inside getx location");
	return ncp->Get(id)->GetObject<MobilityModel>()->GetPosition().x;  
}

Ipv4Address CensusNodeInfo::getAddress(int id)
{
	return naddress->GetAddress(id,0);
}

void CensusNodeInfo::getXYNwBound(double x_bound, double y_bound)
{
	start = time(0);
        xBound = x_bound;
        yBound = y_bound;
}

int getMaxLevel(double x, double y)
{
	double x_temp = x;
	double y_temp = y;
	while (x_temp > range && y_temp > 2* range)
	{
		if(levelCount == 0)
			CensusNodeInfo::levelIncrement[levelCount] = 0;
		else
			CensusNodeInfo::levelIncrement[levelCount] = x_temp;

		levelCount++;
		x_temp = x_temp/2;
		y_temp = y_temp/2;

		if(levelCount-1 == 0)
			CensusNodeInfo::levelIncrement[levelCount] = 0;
		else if(levelCount != 0)
			CensusNodeInfo::levelIncrement[levelCount] = x_temp;
	}
	cout << "\n Level Count : " << levelCount << endl;

        map<int, int>::iterator it = CensusNodeInfo::levelIncrement.begin();

        for(; it!=CensusNodeInfo::levelIncrement.end();++it)
        {
                cout << it->first << " " << it->second << endl;
        }

	return levelCount;
}

void divide (double x1, double y1, double x2, double y2, int level)
{
	cellInfo cellInfo;
	cellInfo.level = level;
	cellInfo.cellId = continuous_number;
	cellInfo.x1 = x1;
	cellInfo.y1 = y1;
	cellInfo.x2 = x2;
	cellInfo.y2 = y2;
	cellinfoMap[continuous_number] = cellInfo;
	continuous_number++;
	level--;

	if(level >= 0)
	{
		double x3 = (x1+x2)/2;
		double y3 = (y1+y2)/2;
		divide(x1,y1,x3,y3,level);
		divide(x3,y1,x2,y3,level);
		divide(x3,y3,x2,y2,level);
		divide(x1,y3,x3,y2,level);
	}
	return;
}

void calcMaxTokenPassesPerLevel()
{
        int MaxPasses;

	cout << "Level Count = " << levelCount << endl;
        for(int i = levelCount; i >=0; i--)
        {
                int count = 0;
                map<int, cellInfo>::iterator it = cellinfoMap.begin();
                for(; it != cellinfoMap.end();++it)
                {
                        if(i == it->second.level)
                                count++;
                }
                //MaxPasses = MaxNodes/count;
		cout << "Max Nodes = " << CensusNodeInfo::getMaxNodes() << " count : " << count << endl;
                MaxPasses = CensusNodeInfo::getMaxNodes()/count;
		cout << "Max Passes = " << MaxPasses << endl;
                CensusNodeInfo::MaxTokenPassesPerLevel[i] = MaxPasses;
        }
        map<int, int>::iterator it = CensusNodeInfo::MaxTokenPassesPerLevel.begin();

	cout << "#####################" << endl;
        for(; it!=CensusNodeInfo::MaxTokenPassesPerLevel.end();++it)
        {
                cout << it->first << " " << it->second << endl;
        }
}

int CensusNodeInfo::getMaxTokenPasses(int level)
{
	map<int, int>::iterator it = MaxTokenPassesPerLevel.begin();
	for(; it != MaxTokenPassesPerLevel.end();++it)
	{
		if(level == it->first)
		{
			return it->second;
		}
	}
}

std::pair <int, int> CensusNodeInfo::getCellId(double x, double y, int level)
{
	cout << "Sowmya " << x << " " << y << " " << level << endl;
	map<int, cellInfo>::iterator it = cellinfoMap.begin();
	for(; it != cellinfoMap.end();++it)
	{
		if(it->second.level == level)
		{
			if(x >= it->second.x1 && y >= it->second.y1 && x < it->second.x2 && y < it->second.y2)
			{
				std::pair <int, int> cellID_level;
				cellID_level.first = it->second.cellId;
				cellID_level.second = it->second.level;
				cout << "NodeInfo = " << cellID_level.first << " " << cellID_level.second << endl;
				return cellID_level;
			}
		}
	}
}

// checkForTokenGeneration : Transverses through the Level, determines where the node is present for the particular cell. 
// And coins it to be token and returns it's value to Census Application. 
std::pair<int,int> CensusNodeInfo::checkForTokenGeneration(double x, double y)
{
//	start = time(0);
	std::pair <int, int> cellID_token;
	cellID_token.first = -1;
        cellID_token.second = -1;
        cout << "Count of Number of levels possible " << levelCount << endl;	
	for(int i = levelCount; i >= 0; i--)
	{
		map<int, cellInfo>::iterator it = cellinfoMap.begin();
		for(; it != cellinfoMap.end();++it)
		{
			cout << " Level = " << it->second.level << " i = " << i << endl;
			if(it->second.level == i)
			{
				if(x >= it->second.x1 && y >= it->second.y1 && x < it->second.x2 && y < it->second.y2)
                        	{
					if(token_cellID.at(it->second.cellId) == 0)
					{
	                            		double res1 = sqrt(pow(x - it->second.x1, 2) + pow(y - it->second.y1, 2));
        	                        	double res2 = sqrt(pow(x - it->second.x2, 2) + pow(y - it->second.y2, 2));
	                	                double res = sqrt(pow(it->second.x1 - it->second.x2, 2) + pow(it->second.y1 - it->second.y2, 2));
                	        	        if((res1 + res2 >= res+10 && res1+res2 < res+25) || (res1+res2 <= res-10 && res1+res2 > res-25))
						{
			// CellID : true Only to track the token so that 2 tokens aren't coined for the same cell in the same level.
							token_cellID[it->second.cellId] = 1; 
							cellID_token.first = it->second.cellId;
							cellID_token.second = i;
							cout << "NodeInfo : " << cellID_token.first  << " " << cellID_token.second << endl;
							return cellID_token;
						}
					}	
				}
			}
		}
	}
	return cellID_token;
}

void CensusNodeInfo::divideNetwork()
{
	int level = getMaxLevel(xBound, yBound);
	divide(0,0,xBound,yBound,level);
	map<int, cellInfo>::iterator it = cellinfoMap.begin();
	cout << "Check point" << endl;
	for(; it != cellinfoMap.end();++it)
	{
		cout << it->second.level << "\t" << it->second.cellId << "\t" << it->second.x1 << "\t" << it->second.y1 << "\t" << it->second.x2 << "\t" << it->second.y2 << endl;
	}

	calcMaxTokenPassesPerLevel();
	for(int i = level; i >= 0; i--)
	{
		it = cellinfoMap.begin();
		for(; it != cellinfoMap.end();++it)
        	{
			if(i >= 0 && i == it->second.level)
			    CountOfCellsPerlevel[i]++;
        	}
	}

        /*map<int, int>::iterator it1 = CountOfCellsPerlevel.begin();
        for(; it1 != CountOfCellsPerlevel.end();++it1)
        {
                cout << it1->first<< "\t" << it1->second  << endl;
        }*/
        initialiseTokenMap();

	return;
}

void CensusNodeInfo::initialiseTokenMap()
{
	for(int i = 0; i < continuous_number; i++)
	{
		token_cellID [i] = 0;
	}

	map<int, int>::iterator it1 = token_cellID.begin();
        for(; it1 != token_cellID.end();++it1)
        {
                cout << it1->first<< "\t" << it1->second  << endl;
        }
}

void CensusNodeInfo::getBroadcastRange(int cellID)
{
        cout << "cellID = " << cellID << endl;
        cout << "Level Increment[0] = " << levelIncrement[0] << endl;
        map<int, cellInfo>::iterator it = cellinfoMap.begin();
        for(; it != cellinfoMap.end();++it)
        {
                if(it->second.level == 0)
                {
                        if(it->second.cellId == cellID)
                        {
                                broadcastLimit.xmin = it->second.x1 - levelIncrement[2];
                                broadcastLimit.ymin = it->second.y1 - levelIncrement[2];
                                broadcastLimit.xmax = it->second.x2 + levelIncrement[2];
                                broadcastLimit.ymax = it->second.y2 + levelIncrement[2];

                                if(broadcastLimit.xmin < 0)
                                {
                                        broadcastLimit.xmin = it->second.x1;
                                }
                                else if (broadcastLimit.ymin < 0)
                                {
                                        broadcastLimit.ymin = it->second.y1;
                                }
                                else if(broadcastLimit.xmax > xBound)
                                {
                                        broadcastLimit.xmax = it->second.x2;
                                }
                                else if(broadcastLimit.ymax > yBound)
                                {
                                        broadcastLimit.ymax = it->second.y2;
                                }
                                cout << "x1 = " << it->second.x1 << " it->second.y1 " << it->second.y1 << " it->second.x2 " << it->second.x2 << " it->second.y2 " << it->second.y2 << endl;
                                cout << "Min x = " << broadcastLimit.xmin << " Min y = " << broadcastLimit.ymin << " Max x = " << broadcastLimit.xmax << " Max y = " << broadcastLimit.ymax << endl;
                                break;
                        }
                }
        }
}

void CensusNodeInfo::getBroadcastLimit(struct CensusApplication::broadcastLimit* broadcastLim)
{
        broadcastLim->xmin = broadcastLimit.xmin;
        broadcastLim->ymin = broadcastLimit.ymin;
        broadcastLim->xmax = broadcastLimit.xmax;
        broadcastLim->ymax = broadcastLimit.ymax;

        cout << "BC Lim " << broadcastLim->xmin << " " << broadcastLim->ymin << " " << broadcastLim->xmax << " " << broadcastLim->ymax << endl;
}

int CensusNodeInfo::getMaxLevelCount()
{
	return levelCount;
}

int CensusNodeInfo::getReport(int snapid)
{
	//open file to store counted node numbers

	int cont=1;
	int totalcounted=0;
	int totaltokens=0;

	for (int i=0; i< MaxNodes; i++)
	{

	    coordinate << snapid << "," << ncp->Get(i)->GetObject<MobilityModel>()->GetPosition().x << "," <<ncp->Get(i)->GetObject<MobilityModel>()->GetPosition().y;

		
		if (counted[i]==1)
		{
			totalcounted++;
		        filestr2 << i << ","; 
		 coordinate << "," << 1 << endl;
		}	
                else
		    coordinate << endl;

		if (holder[i]==1)
		{
			totaltokens++;
		}
	}
	filestr2 << "\n";

	totalcounted = counted_cell[0];
	/*for(int i = 1; i < continuous_number; i++)
        {
                cout << "Maximum Nodes = " << MaxNodes << endl;
                cout << "Counted Cell[" <<i << "] " << counted_cell[i]  << endl;
                if(counted_cell[i] >= 0.25*((double)(MaxNodes)))
                {
                        counted_cell[i] = 0;
                        result_count++;
                        cout << "Aggregation Done for level " << i << endl;
                        cout << "Result Count = " << result_count << endl;
                }
        }*/

	timeaxis[snapid]=snapid*2;
	percentage_counted[snapid] = (double)(((double)totalcounted*100)/(double)MaxNodes);
	//total_transfers[snapid]=tokentransfers;
	total_diffusion[snapid]=totalsplits;
	//filestr1 << timeaxis[snapid] << " " << percentage_counted[snapid] << " " << total_transfers[snapid] << " " << (tokentransfers+token_announcements+totalrequests) << " " << totalsplits << " " << totalsplitrebroadcasts << " " << totaltokens << " " << totalcps  << " " << token_announcements << " " << totalrequests << "\n";

	if((double)(totalcounted) >= 0.5*((double)(MaxNodes)) && (flag50== true))
        {
            int one =0; int two =0; int three =0; int four =0; int five =0; int six =0; int seven =0; int other=0;
            for(int index=0; index <MN; index++)
            {
                switch(visited_num[index])
                {
                case 0: break;
                case 1: one++;break;
                case 2: two++;break;
                case 3: three++;break;
                case 4: four++;break;
                case 5: five++;break;
                case 6: six++;break;
                case 7: seven++;break;
                default: other++;
                }
            }
            cout << "At 50% coverage" << one << " " << two << " " << three << " " << four<< " "  << five << " " << six << " " << seven << " " << other << endl;
            flag50 = false;
       }

       if((double)(totalcounted) >= 0.75*((double)(MaxNodes)) && (flag75== true))
       {
            int one =0; int two =0; int three =0; int four =0; int five =0; int six =0; int seven =0; int other=0;
            
            for(int index=0; index <MN; index++)
            {
                switch(visited_num[index])
                {
                case 0: break;
                case 1: one++;break;
                case 2: two++;break;
                case 3: three++;break;
                case 4: four++;break;
                case 5: five++;break;
                case 6: six++;break;
                case 7: seven++;break;
                default: other++;
                }
            }
            cout << "At 75% coverage" << one << " " << two << " " << three << " " << four<< " "  << five << " " << six << " " << seven << " " << other << endl;
            flag75=false;
       }

       if((double)(totalcounted) >= 0.9*((double)(MaxNodes)) && (flag90== true))
       {
            int one =0; int two =0; int three =0; int four =0; int five =0; int six =0; int seven =0; int other=0;
            
            for(int index=0; index <MN; index++)
            {
                switch(visited_num[index])
                {
                case 0: break;
                case 1: one++;break;
                case 2: two++;break;
                case 3: three++;break;
                case 4: four++;break;
                case 5: five++;break;
                case 6: six++;break;
                case 7: seven++;break;
                default: other++;
                }
            }
            cout << "At 90% coverage" << one << " " << two << " " << three << " " << four<< " "  << five << " " << six << " " << seven << " " << other << endl;
            flag90=false;
       }
      
       if((double)(totalcounted) >= 1*((double)(MaxNodes)))
       {
            int one =0; int two =0; int three =0; int four =0; int five =0; int six =0; int seven =0; int other=0;
            for(int index=0; index <MN; index++)
            {
                switch(visited_num[index])
                {
                case 0: break;
                case 1: one++;break;
                case 2: two++;break;
                case 3: three++;break;
                case 4: four++;break;
                case 5: five++;break;
                case 6: six++;break;
                case 7: seven++;break;
                default: other++;
                }
            }
            cout << "At 100% coverage" << one << " " << two << " " << three << " " << four<< " "  << five << " " << six << " " << seven << " " << other << endl;
            //to find out duplicated count, print out information

	    for(int i = 0; i < MaxNodes; i++)
            {
                duplicate << i << "," << duplicatecount[i] << "," << visited_num[i]  << "\n";
                //cout << i <<"," << 	counted[i] << endl;
            }	
           
       }
	if ((double)(totalcounted) >= 0.9*((double)(MaxNodes)))
	{
   
        //Debugggin.... Here I want to know what kind of impact node speed has. So find out number of information each node obtained during push.
            int temp = 0;
            for(int id =0; id < 100 ; id++)
            {      
              cout << "whole_world[" << id << "] has " << whole_world[id].size() << " nodes:   " << endl;
               temp += whole_world[id].size();
             /*for(unsigned int index =0; index < whole_world[id].size(); index++){
                  it = whole_world[id].begin();
                   std::advance(it,index);
                   int id = *it;      
                   cout << id << " ";
              }*/
            }           
cout << "RESULT COUNT 2 = " << result_count_l2 << endl;
        cout << "RESULT COUNT 1 = " << result_count_l1 << endl;
        cout << "RESULT COUNT 0 = " << result_count_l0 << endl;
            cout << "Average: " << (double)temp / 100.0 << endl;

        map<int, int>::iterator it = token_announcements.begin();
        for(; it!=token_announcements.end();++it)
        {
                cout << "Level = " << it->first << " Count of token announced = " << it->second << endl;
        }

        map<int, int>::iterator it2 = totalrequests.begin();
        for(; it2!=totalrequests.end();++it2)
        {
                cout << "Level = " << it2->first << " Count of token requests = " << it2->second << endl;
        }

        map<int, int>::iterator it1 = tokentransfers.begin();
        for(; it1!=tokentransfers.end();++it1)
        {
                cout << "Level = " << it1->first << " Count of token Transferred = " << it1->second << endl;
        }

	seconds_since_start = difftime( time(0), start);
	cout << "Simulation Time= " << seconds_since_start << endl;

		cont=0;
		exit(1); //if finished with 99% just exit
	}
	return cont;
}



void CensusNodeInfo::fileClose()
{
	filestr1.close();
	filestr2.close();
	coordinate.close();
       duplicate.close();
	//filestr2.close();
	//plotGraph1();
	//plotGraph2();
}

void CensusNodeInfo::plotGraph1()
{

	// Graph 1
	
		string fileNameWithNoExtension = "Nodes-2000-tokens-12-run-1-convergence";
    	string graphicsFileName        = "src/census/" + fileNameWithNoExtension + ".png";
        string plotFileName            = fileNameWithNoExtension + ".plt";
        string plotTitle               = "Nodes-2000-tokens-12-convergence";
        string dataTitle               = "Counting progress";

        // Instantiate the plot and set its title.
        Gnuplot plot(graphicsFileName);
        plot.SetTitle (plotTitle);

        // Make the graphics file, which the plot file will create when it
        // is used with Gnuplot, be a PNG file.
        plot.SetTerminal ("png");

        // Set the labels for each axis.
        plot.SetLegend ("Time", "Percentage");

        // Set the range for the x axis.
        std::ostringstream sstream1;
        sstream1 << (snapcounts * snapinterval);
        std::string varAsString = sstream1.str();
        NS_LOG_INFO(varAsString);
        plot.AppendExtra ("set xrange [0:" + varAsString+ "]");

        // Instantiate the dataset, set its title, and make the points be
        // plotted along with connecting lines.
        Gnuplot2dDataset dataset;
        dataset.SetTitle (dataTitle);
        dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);

		for (int x=0; x<snapcounts; x++)
		{
			dataset.Add ((x)*snapinterval, percentage_counted[x]);
		}
		
		plot.AddDataset (dataset);
		
		// Open the plot file.
        ofstream plotFile (plotFileName.c_str());

        // Write the plot file.
        plot.GenerateOutput (plotFile);

        // Close the plot file.
        plotFile.close ();

}

void CensusNodeInfo::plotGraph2()
{

	// Graph 2
	
		//string fileNameWithNoExtension = "Nodes-4000-tokens-20-messages";
		string fileNameWithNoExtension = "Nodes-2000-tokens-12-run-1-messages";
    	string graphicsFileName        = "src/census/" + fileNameWithNoExtension + ".png";
        string plotFileName            = fileNameWithNoExtension + ".plt";
        string plotTitle               = "Nodes-2000-tokens-12";
        string dataTitle               = "Token transfers";
        string dataTitle2               = "Diffusions";

        // Instantiate the plot and set its title.
        Gnuplot plot(graphicsFileName);
        plot.SetTitle (plotTitle);

        // Make the graphics file, which the plot file will create when it
        // is used with Gnuplot, be a PNG file.
        plot.SetTerminal ("png");

        // Set the labels for each axis.
        plot.SetLegend ("Time", "Messages");

        // Set the range for the x axis.
        std::ostringstream sstream1;
        sstream1 << (snapcounts * snapinterval);
        std::string varAsString = sstream1.str();
        NS_LOG_INFO(varAsString);
        plot.AppendExtra ("set xrange [0:" + varAsString+ "]");

        // Instantiate the dataset, set its title, and make the points be
        // plotted along with connecting lines.
        Gnuplot2dDataset dataset;
        dataset.SetTitle (dataTitle);
        dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);
        
        Gnuplot2dDataset dataset2;
        dataset2.SetTitle (dataTitle2);
        dataset2.SetStyle (Gnuplot2dDataset::LINES_POINTS);

		for (int x=0; x<snapcounts; x++)
		{
			dataset.Add ((x)*snapinterval, total_transfers[x]);
			dataset2.Add ((x)*snapinterval, total_diffusion[x]);
		}
		
		plot.AddDataset (dataset);
		plot.AddDataset (dataset2);
		
		// Open the plot file.
        ofstream plotFile (plotFileName.c_str());

        // Write the plot file.
        plot.GenerateOutput (plotFile);

        // Close the plot file.
        plotFile.close ();

}



} //namespace ns3
