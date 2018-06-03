// BondInquiryHistoricalDataSoa.hpp
// 
// Author: Yuchen Liu
// 
// Define bond inquiry historical data architecture, including 
// bond inquiry historical data service for maintaining the inquiry data, and
// bond inquiry historical data service connector for publishing data, and 
// bond inquiry historical data service listener for data inflow from bond inquiry service

#ifndef BondInquiryHistoricalDataSoa_hpp
#define BondInquiryHistoricalDataSoa_hpp

#include "historicaldataservice.hpp"
#include "BondService/BondInquirySoa.hpp"
#include "products.hpp"
#include "utilityfunction.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "soa.hpp"
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>


// Bond historical data service for position data
class BondInquiryHistoricalDataService : public HistoricalDataService<Inquiry<Bond>>
{
protected:
	std::vector<ServiceListener<Inquiry<Bond>>*> listeners;
	Connector<Inquiry<Bond>>* bondInquiryHistoricalDataConnector;
	std::unordered_map<string, Inquiry<Bond>> inquiryMap; // key on inquiry indentifier

public:
	BondInquiryHistoricalDataService(Connector<Inquiry<Bond>>* _bondInquiryHistoricalDataConnector); // Ctor

	// Get data on our service given a key
	virtual Inquiry<Bond> & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(Inquiry<Bond> &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<Inquiry<Bond>> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<Inquiry<Bond>>* >& GetListeners() const;

	// Persist data to a store
	virtual void PersistData(string persistKey, const Inquiry<Bond>& data);
};

// corresponding publish connector
class BondInquiryHistoricalDataConnector : public Connector<Inquiry<Bond>>
{
protected:
	fstream file;
public:
	BondInquiryHistoricalDataConnector(string _path); // ctor

	// Publish data to the Connector
	virtual void Publish(Inquiry <Bond> &data);

};

// corresponding service listener
class BondInquiryHistoricalDataListener : public ServiceListener<Inquiry<Bond>>
{
protected:
	BondInquiryHistoricalDataService* bondInquiryHistoricalDataService;

public:
	BondInquiryHistoricalDataListener(BondInquiryHistoricalDataService* _bondInquiryHistoricalDataService); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(Inquiry<Bond> &data);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(Inquiry<Bond> &data);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(Inquiry<Bond> &data);
};

BondInquiryHistoricalDataService::BondInquiryHistoricalDataService(Connector<Inquiry<Bond>>* 
	_bondInquiryHistoricalDataConnector): 
	bondInquiryHistoricalDataConnector(_bondInquiryHistoricalDataConnector)
{
}


Inquiry<Bond> & BondInquiryHistoricalDataService::GetData(string key)
{
	return inquiryMap[key];
}

void BondInquiryHistoricalDataService::OnMessage(Inquiry<Bond> &data)
{ // No OnMessage() defined for the intermediate service 
}

void BondInquiryHistoricalDataService::AddListener(ServiceListener<Inquiry<Bond>> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<Inquiry<Bond>>* >& BondInquiryHistoricalDataService::GetListeners() const
{
	return listeners;
}

void BondInquiryHistoricalDataService::PersistData(string persistKey, const Inquiry<Bond>& data)
{
	// push data into the map
	if (inquiryMap.find(persistKey) == inquiryMap.end()) // if not found this one then create one
		inquiryMap.insert(std::make_pair(persistKey, data));
	else
		inquiryMap[persistKey] = data;

	// publish the data
	Inquiry<Bond> temp(data);
	bondInquiryHistoricalDataConnector->Publish(temp);

}

BondInquiryHistoricalDataConnector::BondInquiryHistoricalDataConnector(string _path) : 
	file(_path, std::ios::out | std::ios::trunc)
{
	// set the header of the output file
	file << "Time,InquiryID,BondIDType,BondID,Side,Quantity,Price,State\n";
}

void BondInquiryHistoricalDataConnector::Publish(Inquiry <Bond> &data)
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
		std::string side = (data.GetSide() == BUY) ? "BUY" : "SELL";
		std::string priceStr = PricetoString(data.GetPrice()); // get price
		InquiryState state = data.GetState(); // get the state
		std::string stateStr = "RECEIVED";
		if (state == QUOTED) stateStr = "QUOTED";
		else if (state == DONE) stateStr = "DONE";
		else if (state == REJECTED) stateStr = "REJECTED";
		else if (state == CUSTOMER_REJECTED) stateStr = "CUSTOMER_REJECTED";

		// make the output
		file << date << " " << timeofDay << "," << data.GetInquiryId() << "," << Idtype << "," 
			<< bond.GetProductId() << "," << bond.GetTicker() << ","
			<< std::to_string(data.GetQuantity()) << "," << priceStr << "," << stateStr << "\n";
	}
	else
	{
		std::cout << "Oh no! Cannot open the file! Maybe the path is not right?\n";
	}
}

BondInquiryHistoricalDataListener::BondInquiryHistoricalDataListener(BondInquiryHistoricalDataService*
	_bondInquiryHistoricalDataService) :
	bondInquiryHistoricalDataService(_bondInquiryHistoricalDataService)
{
}

void BondInquiryHistoricalDataListener::ProcessAdd(Inquiry<Bond> &data)
{ // not defined for this service
}

void BondInquiryHistoricalDataListener::ProcessRemove(Inquiry<Bond> &data)
{ // not defined for this service
}

void BondInquiryHistoricalDataListener::ProcessUpdate(Inquiry<Bond> &data)
{ 
	string key = data.GetProduct().GetProductId();
	bondInquiryHistoricalDataService->PersistData(key, data);
}

#endif // !BondInquiryHistoricalDataSoa_hpp
