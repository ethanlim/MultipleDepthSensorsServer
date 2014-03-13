#include "MultipleKinectsPlatformServer.h"

namespace MultipleKinectsPlatformServer{

	Core::Core(/*Server Address*/string address,/*Server Port*/string port){
	  try
	  {
		/* Communicate with a centralised time server */
		unsigned short randomTimeServerId = rand()%5;
		string timeServer = std::to_string(randomTimeServerId)+".asia.pool.ntp.org";
		this->ReportStatus("Sync with Time Server - "+ timeServer);
		NTPClient timeClient(timeServer);
		long timeFromServer = timeClient.RequestDatetime_UNIX();
		this->_time = new Timer(timeFromServer);
		this->_time = new Timer(0);
		this->_time->Start();


		this->ReportStatus("Create Key Objects");
		/* Initialise the Client Machine List */
		this->_clientList = new MultipleKinectsPlatformServer::ClientsList(this->_time);
		/* Create the jobs queue that process each incoming data from client machines */
		this->_jobQueue = new MultipleKinectsPlatformServer::JobsQueue();
		/* Create algorithm that merge the scenes */
		this->_minorityViewport = new MinorityViewport(this->_time,this->_clientList);

		this->ReportStatus("Server Starting");
		//Initialise the Server with the number of threads
		string docRoot = "C:\\Users\\ethanlim\\Documents\\Projects\\School\\MultipleKinectsPlatformServer\\Web";
		std::size_t num_threads = boost::lexical_cast<std::size_t>(30);
		this->_server = new http::server::server(address, 
												port, 
												docRoot, 
												this->_jobQueue, 
												num_threads,
												this->_clientList,
												this->_minorityViewport);

		this->ReportStatus("Server Started");
	  }
	  catch (std::exception& e)
	  {
		std::cerr << "exception: " << e.what() << "\n";
	  }
	}
	
	void Core::BeginListen(){
		try{
			this->_server->run();			
		}
		catch(std::exception& e){
			std::cerr << "exception: " << e.what() << "\n";
		}
	}

	void Core::ProcessJobs(){

		Json::Value root;   
		Json::Reader reader;
		string rawJSON;
		string timeStamp;

		while(true){

			Job recvJob;
			JobThreadMutex.lock();
			recvJob =  this->_jobQueue->get();
			JobThreadMutex.unlock();
			
			rawJSON = recvJob.GetJobJSON();
			timeStamp = recvJob.GetTimeStamp();
			
			if (!rawJSON.empty()&&!timeStamp.empty()&&reader.parse(rawJSON,root))
			{
				for(unsigned short skeletons=0;skeletons<root.size();skeletons++){
					MultipleKinectsPlatformServer::Skeleton newSkeleton(root.get(skeletons,NULL),atol(timeStamp.c_str()));
					this->_minorityViewport->LoadSkeleton(newSkeleton);
				}
			}
		}
	}

	void Core::ReportStatus(string msg){
		cout << "Core : " << msg << endl;
	}
}

int main(int argc, char **argv)
{
	srand(time(NULL));

	MultipleKinectsPlatformServer::Core *platform;

	if(argc==1){
		throw exception("Program arguments are empty");
	}

	try{
		platform = new MultipleKinectsPlatformServer::Core(argv[1],argv[2]);

		if(platform!=NULL){
			// Start Server on a separate thread
			thread server_thread(&MultipleKinectsPlatformServer::Core::BeginListen,platform);

			// Process Job on a separate thread
			/*
			thread job_thread1(&MultipleKinectsPlatformServer::Core::ProcessJobs,platform);
			thread job_thread2(&MultipleKinectsPlatformServer::Core::ProcessJobs,platform);
			thread job_thread3(&MultipleKinectsPlatformServer::Core::ProcessJobs,platform);
			thread job_thread4(&MultipleKinectsPlatformServer::Core::ProcessJobs,platform);
			job_thread1.join();
			job_thread2.join();
			job_thread3.join();
			job_thread4.join();
			*/

			server_thread.join();
		}
	}catch(exception &error){
		throw error;
	}

	return 0;
}
