// BondGUIService.hpp
// 
// Author: Yuchen Liu
// 
// Define bond GUI architecture, including 
// bond GUI service for modeling GUI, 
// bond GUI connector for publishing data, and
// bond GUI service listener for data inflow from bond pricing service

#ifndef BondGUIService_hpp
#define BondGUIService_hpp

#include "pricingservice.hpp"
#include "GUIService.hpp"
#include "products.hpp"
#include "soa.hpp"
#include "utilityfunction.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include <vector>
#include <unordered_map>
#include <chrono> // model the throttles
#include <fstream>
#include <sstream>
#include <iostream>

// Bond GUI Service
class BondGUIService : public GUIService<Bond>
{
protected:
	std::vector<ServiceListener<Price<Bond>>*> listeners;
	Connector<Price<Bond>>* bondGuiConnector; // publish connector
	std::unordered_map<string, Price<Bond>> priceMap; // key on product identifier

	// throttles modeling
	std::chrono::milliseconds interval;

public:
	BondGUIService(int _interval, Connector<Price<Bond>>* _bondGuiConnector); // ctor

	// Get data on our service given a key
	virtual Price<Bond> & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(Price<Bond> &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<Price<Bond>> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<Price<Bond>>* >& GetListeners() const;

	// Add a price to the service
	virtual void AddPrice(const Price<Bond> &price);

	// Get the interval in the throttle
	const std::chrono::milliseconds& GetTimeInterval() const;
};

// corresponding publish connector
class BondGUIConnector : public Connector<Price<Bond>>
{
protected:
	fstream file;
public:
	BondGUIConnector(string _path); // ctor

	// Publish data to the Connector
	virtual void Publish(Price <Bond> &data);
};

// corresponding service listener
class BondGUIListener : public ServiceListener<Price<Bond>>
{
protected:
	BondGUIService* bondGuiService;

	// model throttle
	std::chrono::system_clock::time_point start; 
	std::chrono::milliseconds interval;
	int counter = 0; // only update 100 times

public:
	BondGUIListener(BondGUIService* _bondGuiService); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(Price<Bond> &data);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(Price<Bond> &data);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(Price<Bond> &data);
};

BondGUIService::BondGUIService(int _interval, Connector<Price<Bond>>* _bondGuiConnector): 
	interval(_interval), bondGuiConnector(_bondGuiConnector)
{
}

Price<Bond> & BondGUIService::GetData(string key)
{
	return priceMap[key];
}

void BondGUIService::OnMessage(Price<Bond> &data)
{ // No OnMessage() defined for the intermediate service 
}

void BondGUIService::AddListener(ServiceListener<Price<Bond>> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<Price<Bond>>* >& BondGUIService::GetListeners() const
{
	return listeners;
}

void BondGUIService::AddPrice(const Price<Bond> &price)
{
	// push the data into the service
	string productId = price.GetProduct().GetProductId();
	if (priceMap.find(productId) == priceMap.end()) // if not found this one then create one
		priceMap.insert(std::make_pair(productId, price));
	else
		priceMap[productId] = price;

	// publish it
	Price<Bond> temp(price);
	bondGuiConnector->Publish(temp);

}

const std::chrono::milliseconds& BondGUIService::GetTimeInterval() const
{
	return interval;
}

BondGUIConnector::BondGUIConnector(string _path) : 
	file(_path, std::ios::out | std::ios::trunc)
{
	// set the header of the output file
	file << "Time,BondIDType,BondID,Price\n";
}

void BondGUIConnector::Publish(Price <Bond> &data)
{
	std::stringstream ss;

	if (file.is_open())
	{
		// make the ingredent of the outout
		auto time = boost::posix_time::microsec_clock::local_time(); // current time
		std::string date = DatetoUsString(time.date());
		std::string timeofDay = boost::posix_time::to_simple_string(time.time_of_day());
		timeofDay.erase(timeofDay.end() - 3, timeofDay.end());
		Bond bond = data.GetProduct(); // get the product
		std::string Idtype = (bond.GetBondIdType() == CUSIP) ? "CUSIP" : "ISIN"; // get the bond id type
		std::string priceStr = PricetoString(data.GetMid());
		// make the output
		file << date << " " << timeofDay << "," << Idtype << "," 
			<< bond.GetProductId() << ","<< priceStr << "\n";
	}
	else
	{
		std::cout << "Oh no! Cannot open the file! Maybe the path is not right?\n";
	}
}

BondGUIListener::BondGUIListener(BondGUIService* _bondGuiService):
	bondGuiService(_bondGuiService)
{
	interval = bondGuiService->GetTimeInterval();
	start = std::chrono::system_clock::now(); // start the clock
}

void BondGUIListener::ProcessAdd(Price<Bond> &data)
{
	// throttle control
	if (std::chrono::system_clock::now() - start >= interval && counter < 100)
	{
		bondGuiService->AddPrice(data);
		start = std::chrono::system_clock::now(); // restart the clock
		counter++;
	}
}

void BondGUIListener::ProcessRemove(Price<Bond> &data)
{ // not defined for this service
}

void BondGUIListener::ProcessUpdate(Price<Bond> &data)
{ // not defined for this service
}


#endif // !BondGUIService_hpp
