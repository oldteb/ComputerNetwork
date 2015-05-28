This is a C source code for simulating both GBN protocol and selective repeat protocol in rdt.
However, we finally only get GBN run.

The structure of our code: 
Application layer:
client.c client.h Server.c

DataLink Layer/physical layer:
DLL.c dllgen.h (for go-back-N protocol)
SR.c slgen.h (for selective repeat protocol)

How to compile:
you type make in your command line. Then it just help you compile C code.

How to run:
Run server: type ./server
Run client: type ./client [server's IP address] [port number = 8888]

We have several commands.
HELP:Return the help from server.
FILELIST:Client send this command, server will return you all files listed in server.
DOWNLOAD: client could download one file from the server after using this command.
UPLOAD:Client could also upload his/her own files to server. After server receives that file, it will updata its file list.


Tas covered:
1. Full implementation of the Application Layer (Client And Server) – Yes 
This application layer has several commands just like what we stated above

2. Correct implementation of Go-back-N protocol – Yes 
This implementation could be seen in DLL.c source code, and also in our performance graph, you can see the specific performance in real 
GBN transmission with different packet loss and packet corruption rate.

3. Correct implementation of Selective Repeat protocol – Partially 
Due to the time, we do not have the time to finish this part. But we design this .c and .h code file to implement.

4. Correct implementation of queuing, checksum, and the inter-layer interfaces – Yes 
We have a queue.h and queue.c in our folder. So we use queue implementation to help us simulate rdt transmission. And we have client/server file for application layer
,with DLL.c/DLL.h file to implement both link layer  and physical layer.

5. Full implementation of Physical Layer with data corruption and packet drop – Yes
We could enable user to input packet loss/packet corruption rate. 

6. Makefile + README = 5% - Yes

7. Graph file = 5% 	- Yes (only GBN)
In this file we show the performance of our GBN protocol.


