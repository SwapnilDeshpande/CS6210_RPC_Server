#include "FetchURL.h"
#include <transport/TSocket.h>
#include <transport/TBufferTransports.h>
#include <protocol/TBinaryProtocol.h>
#include <iostream>
#include <fstream>
#include <string>
#include <time.h>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using boost::shared_ptr;
using namespace std;

using namespace ::CS6210;


int main(int argc, char **argv) {
  // Enter IP address of the proxy server here
  shared_ptr<TSocket> socket(new TSocket("localhost", 9090));
  shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

  string body;
  struct timeval startTime, endTime;
  double totalTime=0;
  string line;
  ifstream myfile ("ExperimentData/random_contiguous.txt");
  FetchURLClient client(protocol);
  transport->open();
  int count=0;
  if (myfile.is_open())
  {
    while ( getline (myfile,line) )
    { 
       count++;
       gettimeofday(&startTime, NULL);
       client.fetch(body, line);
       gettimeofday(&endTime, NULL);
       totalTime += ((endTime.tv_sec - startTime.tv_sec) + ((endTime.tv_usec - startTime.tv_usec)/1000000.0));
       //cout << "Time taken for URL " << line<< " to fetch body is " << totalTime*1000<<" milliseconds.\n";
    }
    if(count==0)
	cout<<"No input URL in file url.txt"<<endl;
    else
	cout<< "Average time to fetch page of the URLS is : "<< (totalTime/count*1.0)<<" seconds"<<endl;
    myfile.close();
  }
  else cout << "Unable to open file";
	  transport->close();
  return 0;
}
