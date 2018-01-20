# Data-Aggregation-in-MANETS
# Authors : Dr Masahiro Nakagawa (Initial Version)
#           Sowmya Devaraja (Implementation for Hierarchical data aggregation)
# In this project, I have the code updated for data aggregation in Mobile Ad-hoc Networks (MANETs).
# This project was simulated on a network simulator, ns-3 for nodes (i.e moving devices) ranging from 100 to 4000 at different mobility speed for the nodes. (3m/s to 21m/s)
# 
# census-application.cc :
# In this file, the various states of the nodes have been handled.
# Push phase - Sharing it's state/node information with the neighbors
# Announce phase - Only node having the token holder enters to this phase
# Request phase - Nodes which have received the announce message responds satisfying a few criterias.
# Reply phase - Token is handed to one of the nodes in the vicinity.

# census-nodeinfo.cc :
# In this file,
# Dividing the network into cells is handled.
# Result broadcast to neighbors is handled
# Termination of data aggregation in the network.
