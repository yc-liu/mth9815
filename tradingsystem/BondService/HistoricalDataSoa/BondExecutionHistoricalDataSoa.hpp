// BondExecutionHistoricalDataSoa.hpp
// 
// Author: Yuchen Liu
// 
// Define bond execution historical data architecture, including 
// bond execution historical data service for maintaining the execution order data, and
// bond execution historical data service connector for publishing data, and 
// bond execution historical data service listener for data inflow from bond execution service

#ifndef BondExecutionHistoricalDataSoa_hpp
#define BondExecutionHistoricalDataSoa_hpp

#include "historicaldataservice.hpp"
#include "BondService/BondExecutionSoa.hpp"
#include "products.hpp"
#include "soa.hpp"
#include "utilityfunction.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>


// Bond historical data service for position data
class BondExecutionHistoricalDataService : public HistoricalDataService<ExecutionOrder<Bond>>
{
protected:
	std::vector<ServiceListener<ExecutionOrder<Bond>>*> listeners;
	Connector<ExecutionOrder<Bond>>* bondExecutionHistoricalDataConnector;
	std::unordered_map<string, ExecutionOrder<Bond>> orderMap; // key on product indentifier

public:
	BondExecutionHistoricalDataService(Connector<ExecutionOrder<Bond>>* _bondExecutionHistoricalDataConnector); // ctor

	// Get data on our service given a key
	virtual ExecutionOrder<Bond> & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(ExecutionOrder<Bond> &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<ExecutionOrder<Bond>> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<ExecutionOrder<Bond>>* >& GetListeners() const;

	// Persist data to a store
	virtual void PersistData(string persistKey, const ExecutionOrder<Bond>& data);
};

// corresponding publish connector
class BondExecutionHistoricalDataConnector : public Connector<ExecutionOrder<Bond>>
{
protected:
	fstream file;
public:
	BondExecutionHistoricalDataConnector(string _path); // ctor

	// Publish data to the Connector
	virtual void Publish(ExecutionOrder <Bond> &data);

};

// corresponding service listener
class BondExecutionHistoricalDataListener : public ServiceListener<ExecutionOrder<Bond>>
{
protected:
	BondExecutionHistoricalDataService* bondExecutionHistoricalDataService;

public:
	BondExecutionHistoricalDataListener(BondExecutionHistoricalDataService* _bondExecutionHistoricalDataService); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(ExecutionOrder<Bond> &data);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(ExecutionOrder<Bond> &data);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(ExecutionOrder<Bond> &data);
};

BondExecutionHistoricalDataService::BondExecutionHistoricalDataService(
	Connector<ExecutionOrder<Bond>>* _bondExecutionHistoricalDataConnector) :
	bondExecutionHistoricalDataConnector(_bondExecutionHistoricalDataConnector)
{
}

ExecutionOrder<Bond> & BondExecutionHistoricalDataService::GetData(string key)
{
	return orderMap[key];
}

void BondExecutionHistoricalDataService::OnMessage(ExecutionOrder<Bond> &data)
{ // No OnMessage() defined for the intermediate service 
}

void BondExecutionHistoricalDataService::AddListener(ServiceListener<ExecutionOrder<Bond>> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<ExecutionOrder<Bond>>* >& BondExecutionHistoricalDataService::GetListeners() const
{
	return listeners;
}

void BondExecutionHistoricalDataService::PersistData(string persistKey, const ExecutionOrder<Bond>& data)
{
	// push data into the map
	if (orderMap.find(persistKey) == orderMap.end()) // if not found this one then create one
		orderMap.insert(std::make_pair(persistKey, data));
	else
		orderMap[persistKey] = data;

	// publish the data
	ExecutionOrder<Bond> temp(data);
	bondExecutionHistoricalDataConnector->Publish(temp);

}

BondExecutionHistoricalDataConnector::BondExecutionHistoricalDataConnector(string _path):
	file(_path, std::ios::out | std::ios::trunc)
{
	// set the header of the output file
	file << "Time,OrderType,OrderID,BondIDType,BondID,Side,VisibleQuantity,HiddenQuantity,"
		<< "Price,IsChildOrder,ParentOrderId\n";
}

void BondExecutionHistoricalDataConnector::Publish(ExecutionOrder <Bond> &data)
{
	std::stringstream ss;

	if (file.is_open())
	{
		// make the ingredent of the outout
		auto time = boost::posix_time::microsec_clock::local_time(); // current time
		std::string date = DatetoUsString(time.date());
		std::string timeofDay = boost::posix_time::to_simple_string(time.time_of_day());
		timeofDay.erase(timeofDay.end() - 3, timeofDay.end());
		OrderType type = data.GetOrderType(); // get the state
		std::string typeStr; 
		if (type == FOK) typeStr = "FOK";
		else if (type == IOC) typeStr = "IOC";
		else if (type == MARKET) typeStr = "MARKET";
		else if (type == LIMIT) typeStr = "LIMIT";
		else if (type == STOP) typeStr = "STOP";
		Bond bond = data.GetProduct(); // get the product
		std::string Idtype = (bond.GetBondIdType() == CUSIP) ? "CUSIP" : "ISIN"; // get the bond id type
		std::string side = (data.GetSide() == BID) ? "BID" : "OFFER";
		std::string priceStr = PricetoString(data.GetPrice()); // get price
		std::string isChildOrder = (data.IsChildOrder() == true) ? "TRUE" : "FALSE";

		// make the output
		file << date << " " << timeofDay << "," << typeStr << "," << data.GetOrderId() << "," 
			<< Idtype << "," << bond.GetProductId() << ","
			<< side << "," << std::to_string(data.GetVisibleQuantity()) << ","
			<< std::to_string(data.GetHiddenQuantity()) << "," << priceStr << ","
			<< isChildOrder << "," << data.GetParentOrderId() << "\n";
	}
	else
	{
		std::cout << "Oh no! Cannot open the file! Maybe the path is not right?\n";
	}
}

BondExecutionHistoricalDataListener::BondExecutionHistoricalDataListener(
	BondExecutionHistoricalDataService* _bondExecutionHistoricalDataService):
	bondExecutionHistoricalDataService(_bondExecutionHistoricalDataService)
{
}

void BondExecutionHistoricalDataListener::ProcessAdd(ExecutionOrder<Bond> &data)
{
	string key = data.GetProduct().GetProductId();
	bondExecutionHistoricalDataService->PersistData(key, data);
}

void BondExecutionHistoricalDataListener::ProcessRemove(ExecutionOrder<Bond> &data)
{ // not defined for this service
}

void BondExecutionHistoricalDataListener::ProcessUpdate(ExecutionOrder<Bond> &data)
{ // not defined for this service
}

#endif // !BondExecutionHistoricalDataSoa_hpp
