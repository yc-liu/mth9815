------------------- This is a Readme file -------------------

Project Description:
* Final project in the course MTH 9815
* Instructor: Breman Thuraisingham 
* Model the infrastructure of a trading system by Service-oriented Architecture (SOA) using C++.
* Emulate 3 service lines in the system: the market making service line, the trading execution service line, and the inquiry service line. 
* Project structure description
	* .\BondService: the bond implementation header files on different services
	* .\Data: the generation files for the input data, and the address for the input data and the output data
	* .\StopWatch.hpp: an utility class to model the time elapsion
	* .\utilityfunction.hpp: utility functions to model the conversion from/to string
	* .\main.cpp: the execution file
	* .\CMakeLists.txt: the c-make file
	* other hpp files: the SOA structure and the base product class
	* .\Readme.txt: Read-me file
	

Project Setup:
* Need cmake 3.9 and boost 1.6
* cd to the root directory, make sure it contains CMakeLists.txt
* Write the following commands in the Unix terminal:
cmake .
make
./tradingsystem
* When running, the main program will first set up the data paths and some prerequisites, then it will generate the input data, and finally run the three service lines sequentially and get the corresponding output data

Modifications to the original service codes:
* executionservice.hpp: 
	* add an empty default ctor in the ExecutionOrder<T> class
	* change the type of visibleQuantity and hiddenQuantity in the ExecutionOrder<T> class from double to long, as well as the corresponding ctor and getters
	* add a GetSide() function in the ExecutionOrder<T> class to get the inner Side data member
	* add 'virtual' keyword to the ExecuteOrder() function in the ExecutionService<T> class
* historicaldataservice.hpp:
	* add 'virtual' keyword to the PersistData() function in the HistoricalDataService<T> class			  
* inquiryservice.hpp: 			
	* add an empty default ctor in the Inquiry<T> class
	* add 'virtual' keyword to the SendQuote() and RejectInquiry() function in the InquiryService<T> class
* marketdataservice.hpp:
	* add an empty default ctor in the OrderBook<T> class
	* add a GetBestBidOffer() function in the OrderBook<T> class to get the best bid-offer order pair within this orderbook
* positionservice.hpp:
	* add an empty default ctor in the Position<T> class
	* change the type of positions data member in the Position<T> class from map to unordered_map
	* change the type of the value parameter in the positions data member in the Position<T> class from long to long long
	* change the return type of the GetPosition() and GetAggregatePosition() functions in the Position<T> class from long to long long
	* add a AddNewPosition() function in the Position<T> class to update the positions in a specific book
	* add a HasBook() function in the Position<T> class to determine whether the inner positions have a specific book
	* implement the GetAggregatePosition() function in the Position<T> class
* pricingservice.hpp:
	* add an empty default ctor in the Price<T> class
* productservice.hpp:
	* declare and implement the virtual functions inherited from Service<K,V> base class
* riskservice.hpp
	* add an empty default ctor in the PV01<T> class and the BucketedSector<T> class
	* implement the GetProduct(), GetPV01() and GetQuantity() functions in the PV01<T> class
	* change the type of quantity in the PV01<T> class from long to long long, as well as the corresponding ctor and getter
	* add 'virtual' keyword to the AddPosition() and GetBucketedRisk() functions in the RiskService<T> class
* streamingservice.hpp:
	* add an empty default ctor in the PriceStreamOrder<T> class and the PriceStream<T> class
	* implement the GetSide() function in the PriceStreamOrder<T> class
	* add 'virtual' keyword to the PublishPrice() function in the StreamingService<T> class
* tradebookingservice.hpp:
	* add an empty default ctor in the Trade<T> class
	* add 'virtual' keyword to the BookTrade() function in the TradeBookingService<T> class
	
Contact Information:
* Arthor: Yuchen Liu
* Email: liuyc0618@gmail.com

-------------------------------------------------------------

