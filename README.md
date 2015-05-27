potential-octobot
=================

An assignment in which I was tasked with creating three seperate servers. Each server uses sockets as the base, and expands upon each other using differing methods to handle clients. The servers in question are simple echo servers. The end goal was to create at least one server that could handle ten thousand concurrent connections at one time. All three met this requirement.
BasicServer waits for connections, then spawns a thread to read the client input and echo it back. 
SelectServer waits for connections, then adds the client to the select discriptor for servicing. 
EpollServer uses non-blocking file-discriptors to service the client. 

The client used for testing is also in C, and simply allows the user to specify how many threads, the size of the message, and the number of times to send it to the server. This client was used on multiple machines at a time in order to test the load limits on each server.

The design document, report, and testing data can all be requested through email to jacob.miner1@gmail.com.
