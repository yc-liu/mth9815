// BondPositionHistoricalDataSoa.hpp
// 
// Author: Yuchen Liu
// 
// Define bond position historical data architecture, including 
// bond position historical data service for maintaining the position data, and
// bond position historical data service connector for publishing data, and 
// bond position historical data service listener for data inflow from bond position service

#ifndef BondPositionHistoricalDataSoa_hpp
#define BondPositionHistoricalDataSoa_hpp

#include "historicaldataservice.hpp"
#include "BondService/BondPositionSoa.hpp"
#include "products.hpp"
#include "soa.hpp"
#include "utilityfunction.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iostream>


// Bond historical data service for position data
class BondPositionHistoricalDataService : public HistoricalDataService<Position<Bond>>
{
protected:
	std::vector<ServiceListener<Position<Bond>>*> listeners;
	Connector<Position<Bond>>* bondPositionHistoricalDataConnector;
	std::unordered_map<string, Position<Bond>> positionMap; // key on product indentifier

public:
	BondPositionHistoricalDataService(Connector<Position<Bond>>* _bondPositionHistoricalDataConnector); // ctor

	// Get data on our service given a key
	virtual Position<Bond> & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(Position<Bond> &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<Position<Bond>> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<Position<Bond>>* >& GetListeners() const;

	// Persist data to a store
	virtual void PersistData(string persistKey, const Position<Bond>& data);
};

// corresponding publish connector
class BondPositionHistoricalDataConnector : public Connector<Position<Bond>>
{
protected:
	fstream file;
public:
	BondPositionHistoricalDataConnector(string _path); // ctor

	// Publish data to the Connector
	virtual void Publish(Position <Bond> &data);

};

// corresponding service listener
class BondPositionHistoricalDataListener : public ServiceListener<Position<Bond>>
{
protected:
	BondPositionHistoricalDataService* bondPositionHistoricalDataService;

public:
	BondPositionHistoricalDataListener(BondPositionHistoricalDataService* _bondPositionHistoricalDataService); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(Position<Bond> &data);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(Position<Bond> &data);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(Position<Bond> &data);
};

BondPositionHistoricalDataService::BondPositionHistoricalDataService(Connector<Position<Bond>>*
	_bondPositionHistoricalDataConnector) :
	bondPositionHistoricalDataConnector(_bondPositionHistoricalDataConnector)
{
}


Position<Bond> & BondPositionHistoricalDataService::GetData(string key)
{
	return positionMap[key];
}

void BondPositionHistoricalDataService::OnMessage(Position<Bond> &data)
{ // No OnMessage() defined for the intermediate service 
}

void BondPositionHistoricalDataService::AddListener(ServiceListener<Position<Bond>> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<Position<Bond>>* >& BondPositionHistoricalDataService::GetListeners() const
{
	return listeners;
}

void BondPositionHistoricalDataService::PersistData(string persistKey, const Position<Bond>& data)
{
	// push data into the map
	if (positionMap.find(persistKey) == positionMap.end()) // if not found this one then create one
		positionMap.insert(std::make_pair(persistKey, data));
	else
		positionMap[persistKey] = data;

	// publish the data
	Position<Bond> temp(data);
	bondPositionHistoricalDataConnector->Publish(temp);

}

BondPositionHistoricalDataConnector::BondPositionHistoricalDataConnector(string _path) : 
	file(_path, std::ios::out | std::ios::trunc)
{
	// set the header of the output file
	file << "Time,BondIDType,BondID,BookId,Positions\n";
}

void BondPositionHistoricalDataConnector::Publish(Position <Bond> &data)
{
	std::stringstream ss;
	// hard-coded the book id
	std::string book1 = "TRSY1";
	std::string book2 = "TRSY2";
	std::string book3 = "TRSY3";


	if (file.is_open())
	{
		// make the ingredent of the outout
		auto time = boost::posix_time::microsec_clock::local_time(); // current time
		std::string date = DatetoUsString(time.date());
		std::string timeofDay = boost::posix_time::to_simple_string(time.time_of_day());
		timeofDay.erase(timeofDay.end() - 3, timeofDay.end());
		Bond bond = data.GetProduct(); // get the product
		std::string Idtype = (bond.GetBondIdType() == CUSIP) ? "CUSIP" : "ISIN"; // get the bond id type
		// make the output
		file << date << " " << timeofDay << "," << Idtype << "," << bond.GetProductId() << ","
			<< book1 << "," << std::to_string(data.GetPosition(book1)) << "\n";
		file << date << " " << timeofDay << "," << Idtype << "," << bond.GetProductId() << ","
			<< book2 << "," << std::to_string(data.GetPosition(book2)) << "\n";
		file << date << " " << timeofDay << "," << Idtype << "," << bond.GetProductId() << ","
			<< book3 << "," << std::to_string(data.GetPosition(book3)) << "\n";
		file << date << " " << timeofDay << "," << Idtype << "," << bond.GetProductId() << ","
			<< "AGGREGATED" << "," << std::to_string(data.GetAggregatePosition()) << "\n";
	}
	else
	{
		std::cout << "Oh no! Cannot open the file! Maybe the path is not right?\n";
	}
}

BondPositionHistoricalDataListener::BondPositionHistoricalDataListener(BondPositionHistoricalDataService* 
	_bondPositionHistoricalDataService): bondPositionHistoricalDataService(_bondPositionHistoricalDataService)
{
}

void BondPositionHistoricalDataListener::ProcessAdd(Position<Bond> &data)
{ // not defined for this service
}

void BondPositionHistoricalDataListener::ProcessRemove(Position<Bond> &data)
{ // not defined for this service
}

void BondPositionHistoricalDataListener::ProcessUpdate(Position<Bond> &data)
{
	string key = data.GetProduct().GetProductId();
	bondPositionHistoricalDataService->PersistData(key, data);
}

#endif // !BondPositionHistoricalDataSoa_hpp
