// Server

#include "FetchURL.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <cstring>
#include <time.h>
#include <queue>
#include <vector>

#define POLICY 1 // 1 = RANDOM, 2 = FIFO, 3 = LSF_MAXS

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;
using namespace std;

using namespace ::CS6210;

// Node of a priority queue used in Largest Size First algorithm containing URL and size/length of body
struct QNode{
	string URL;
	int32_t length;
};
  
//Code for creating a priority queue which is sorted in descending order based on the size of the body of the URL
class CompareCount {
	public:
	    bool operator()(QNode &q1, QNode &q2)
	    {
	       if (q1.length < q2.length) return true;	       
	       return false;
	    }
};
  

class FetchURLHandler : virtual public FetchURLIf {
 public:

  // Cache limit in bytes
  static const double CACHELIMIT = 1*1024*1024;
  URLMap MyMap; 
  
  // Map used in Random and FIFO algorithms. (key,value) = (index,URL)
  map<int32_t,string> IndexMap;  
  int32_t index;
  int32_t cachehits;

  // Priority queue used in LSF_MAXS algorithm
  priority_queue<QNode,vector<QNode>, CompareCount> queue;  
    
  FetchURLHandler() {
    index=0;
    cachehits=0;
  }

  void ping() {
    printf("ping\n");
  }
  
  static size_t CallbackFunction(void *contents, size_t size, size_t nmemb, void *userp){
  	((string*)userp)->append((char*)contents, size * nmemb);
  	return size*nmemb;
  }
  
  // Retrieve the size of cache in bytes
  int32_t getMapSize(){
  	int32_t size = 0;
  	std::map<string,string>::iterator it;
  	for (it=MyMap.URLBody.begin(); it!=MyMap.URLBody.end(); ++it){
  		size += it->second.size();
  	}
  	return size;    		
  }
  
  // Cache entry chosen at random from the cache to evict
  void random(){
  	cout << "Random policy selected \n";
  	int32_t start, end, randNum;
  	std::map<int32_t, string>::iterator it = IndexMap.begin();
  	start = it->first;
  	end = index;
  	while(1){
  		srand(time(NULL));
  		randNum = rand() % end + start; 
  		it = IndexMap.find(randNum);
  		if(it != IndexMap.end())
  			break;
  	}  	
  	string keyURL = IndexMap[randNum];
  	IndexMap.erase(randNum);
  	MyMap.URLBody.erase(keyURL);
  	cout << "URL to be evicted is : " << keyURL << endl;  	
  }

  // Oldest cache entry evicted  
  void fifo(){
  	int32_t start;  	
  	cout << "FIFO policy selected \n";
  	std::map<int32_t, string>::iterator it = IndexMap.begin();
	start = it->first;  		
	it = IndexMap.find(start);
	string keyURL = IndexMap[start];
  	IndexMap.erase(start);
  	MyMap.URLBody.erase(keyURL);
  	cout << "URL to be evicted is : " << keyURL << endl;  
  }
  
  // Cache entry whose body size is largest is chosen
  void LSF_MAXS(){  		
  	cout << "LSF_MAXS policy selected \n";
  	QNode node = queue.top();
 	cout << "URL to be evicted is : " << node.URL << "\n";
	MyMap.URLBody.erase(node.URL);
  	queue.pop();  	
  }
  

  void deleteFromCache(){
  	switch(POLICY){
  		case 1:			// Random
  			random();
  			break;
		case 2:			// FIFO
			fifo();
			break;
		case 3:	
			LSF_MAXS();	// LFU
			break;
		default:
			cout << "Invalid policy.";
  	}
  }
  
  void freeCache(double cPageSize){
/*  	cout << "Current Page size (in KB): " << cPageSize << "\n";
  	cout << "Inside freeCache()\n";
  	cout << "BEFORE: " << "\n";
  	cout << "Number of elememts in cache: " << MyMap.URLBody.size() << "\n";
  	cout << "Map size: " << (getMapSize()) << "\n";
*/
  	double totalSize = 0;
  	do{
  		deleteFromCache();
  		totalSize = (getMapSize() + cPageSize);
  	}while(totalSize > CACHELIMIT);
  	
/*  	cout << "AFTER: " << "\n";
  	cout << "Number of elememts in cache: " << MyMap.URLBody.size() << "\n"; 
*/ 	
  }

  void fetch(std::string& _return, const std::string& URL) {

    string readBuf;    
    CURL *hd;    
    cout << "Requested URL: " << URL << "\n";
    switch(POLICY){
	case 1:			
		cout<< "Cache replacement policy used: Random Policy\n";
		break;
	case 2:			
		cout<< "Cache replacement policy used: FIFO Policy\n";
		break;
	case 3:	
		cout<< "Cache replacement policy used: Largest Size First Policy\n";	
		break;
	default:
		cout << "Invalid policy used.";
    }

    map<string,string>::iterator it;
    it=MyMap.URLBody.find(URL);

    // URL not found in cache. Make http request for the URL requested.
    if(it == MyMap.URLBody.end()){						
    	    hd = curl_easy_init();	    
	    curl_easy_setopt(hd, CURLOPT_URL, URL.c_str());
	    curl_easy_setopt(hd, CURLOPT_WRITEFUNCTION, CallbackFunction);
	    curl_easy_setopt(hd, CURLOPT_WRITEDATA, &readBuf);    
	    curl_easy_perform(hd);
	    _return = readBuf;
	    cout<< "Size of url body : "<<readBuf.size()/1024*1.0 <<" KB"<<endl;
	    //Check if the page fits in the cache.
	    if ((readBuf.size()) > CACHELIMIT){					
	    	cout << "Current page size greater than cache limit.\n";
	    	cout << "Page NOT inserted in cache.\n";
	    }
	    else{
                    //Does it require some pages to be removed?
	    	    double totalSize = (getMapSize() + readBuf.size());	    
		    if(totalSize > CACHELIMIT){
				// Remove page(s) by the cache replacement policy
			    	cout << "\n-----------------------CACHE LIMIT EXCEEDED!!!-----------------\n Calling replacement algorithm." << endl ;
			    	freeCache(readBuf.size());
			    	MyMap.URLBody[URL] = readBuf;
			    	if(POLICY == 1 || POLICY == 2){
			    	 	IndexMap[index++] = URL;
			    	}
			        else if(POLICY == 3){
			  		//LSF_MAXS Replacement
			    		QNode node;
			    		node.URL=URL;
			    		string str = MyMap.URLBody[URL];
			    		cout << "URL : " << URL << "  Size: " << str.size() << "\n";
			    		node.length=str.size();
			    		queue.push(node);
			    		cout<< "Node inserted : "<<node.URL<<endl;
			        }			

			    	cout << "Size of cache: " << getMapSize() << "\n-----------------------------------\n";
			  	cout << "Number of elememts in cache: " << MyMap.URLBody.size() << "\n";
			        cout << "Cache hits: " <<cachehits <<endl;  	
		    }
		    else{		
	 		    // Put the page in the cache directly					
	    		    MyMap.URLBody[URL] = readBuf;

			    //Random or FIFO Replacement
			    if(POLICY == 1 || POLICY == 2){			
	    		    	IndexMap[index++] = URL;
 			    }
			    else if(POLICY == 3){
			  		//LSF_MAXS Replacement
			    		QNode node;
			    		node.URL=URL;
			    		string str = MyMap.URLBody[URL];
			    		cout << "URL : " << URL << "  Size: " << str.size() << "\n";
			    		node.length=str.size();
			    		queue.push(node);
			    		cout<< "Node inserted : "<<node.URL<<endl;
			    }			

			    cout << "No. of elements in cache: " << MyMap.URLBody.size() << "\n";
			    cout << "Size of cache: " << getMapSize() << "\n-----------------------------------\n";
		            cout << "Cache hits: " <<cachehits <<endl;
		    }
	    }
	    curl_easy_cleanup(hd);
    }
    else{									
	// The page is in the cache. (Cache HIT :) !!!)
    	_return = MyMap.URLBody[URL];
        cachehits++;
    	cout << "Cache HIT\n";
	cout << "No. of elements in cache: " << MyMap.URLBody.size() << "\n";
    	cout << "Size of cache: " << getMapSize() << "\n-----------------------------------\n";
        cout << "Cache hits: " <<cachehits <<endl;
    }
  }
};

int main(int argc, char **argv) {
  int port = 9090;
  shared_ptr<FetchURLHandler> handler(new FetchURLHandler());
  shared_ptr<TProcessor> processor(new FetchURLProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
  return 0;
}

