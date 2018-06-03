// BondStreamingHistoricalDataSoa.hpp
// 
// Author: Yuchen Liu
// 
// Define bond streaming historical data architecture, including 
// bond streaming historical data service for maintaining the price streaming data, and
// bond streaming historical data service connector for publishing data, and 
// bond streaming historical data service listener for data inflow from bond streaming service

#ifndef BondStreamingHistoricalDataSoa_hpp
#define BondStreamingHistoricalDataSoa_hpp

#include "historicaldataservice.hpp"
#include "BondService/BondStreamingSoa.hpp"
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
class BondStreamingHistoricalDataService : public HistoricalDataService<PriceStream<Bond>>
{
protected:
	std::vector<ServiceListener<PriceStream<Bond>>*> listeners;
	Connector<PriceStream<Bond>>* bondStreamingHistoricalDataConnector;
	std::unordered_map<string, PriceStream<Bond>> streamMap; // key on product indentifier

public:
	BondStreamingHistoricalDataService(Connector<PriceStream<Bond>>* _bondStreamingHistoricalDataConnector); // ctor

	// Get data on our service given a key
	virtual PriceStream<Bond> & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(PriceStream<Bond> &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<PriceStream<Bond>> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<PriceStream<Bond>>* >& GetListeners() const;

	// Persist data to a store
	virtual void PersistData(string persistKey, const PriceStream<Bond>& data);
};

// corresponding publish connector
class BondStreamingHistoricalDataConnector : public Connector<PriceStream<Bond>>
{
protected:
	fstream file;
public:
	BondStreamingHistoricalDataConnector(string path); // ctor

	// Publish data to the Connector
	virtual void Publish(PriceStream <Bond> &data);

};

// corresponding service listener
class BondStreamingHistoricalDataListener : public ServiceListener<PriceStream<Bond>>
{
protected:
	BondStreamingHistoricalDataService* bondStreamingHistoricalDataService;

public:
	BondStreamingHistoricalDataListener(BondStreamingHistoricalDataService* _bondStreamingHistoricalDataService); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(PriceStream<Bond> &data);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(PriceStream<Bond> &data);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(PriceStream<Bond> &data);
};

BondStreamingHistoricalDataService::BondStreamingHistoricalDataService(
	Connector<PriceStream<Bond>>* _bondStreamingHistoricalDataConnector) :
	bondStreamingHistoricalDataConnector(_bondStreamingHistoricalDataConnector)
{
}

PriceStream<Bond> & BondStreamingHistoricalDataService::GetData(string key)
{
	return streamMap[key];
}

void BondStreamingHistoricalDataService::OnMessage(PriceStream<Bond> &data)
{ // No OnMessage() defined for the intermediate service 
}

void BondStreamingHistoricalDataService::AddListener(ServiceListener<PriceStream<Bond>> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<PriceStream<Bond>>* >& BondStreamingHistoricalDataService::GetListeners() const
{
	return listeners;
}

void BondStreamingHistoricalDataService::PersistData(string persistKey, const PriceStream<Bond>& data)
{
	// push data into the map
	if (streamMap.find(persistKey) == streamMap.end()) // if not found this one then create one
		streamMap.insert(std::make_pair(persistKey, data));
	else
		streamMap[persistKey] = data;

	// publish the data
	PriceStream<Bond> temp(data);
	bondStreamingHistoricalDataConnector->Publish(temp);

}

BondStreamingHistoricalDataConnector::BondStreamingHistoricalDataConnector(string _path): 
	file(_path, std::ios::out | std::ios::trunc)
{
	// set the header of the output file
	file << "Time,BondIDType,BondID,BidPrice,BidVisibleQuantity,BidHiddenQuantity," 
		<< "OfferPrice,OfferVisibleQuantity,OfferHiddenQuantity\n";
}

void BondStreamingHistoricalDataConnector::Publish(PriceStream <Bond> &data)
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
		// make the output
		file << date << " " << timeofDay << "," << Idtype << "," << bond.GetProductId() << "," 
			<< std::to_string(data.GetBidOrder().GetPrice()) << ","
			<< std::to_string(data.GetBidOrder().GetVisibleQuantity()) << "," 
			<< std::to_string(data.GetBidOrder().GetHiddenQuantity()) << ","
			<< std::to_string(data.GetOfferOrder().GetPrice()) << "," 
			<< std::to_string(data.GetBidOrder().GetVisibleQuantity()) << ","
			<< std::to_string(data.GetBidOrder().GetHiddenQuantity()) << "\n";

	}
	else
	{
		std::cout << "Oh no! Cannot open the file! Maybe the path is not right?\n";
	}
}

BondStreamingHistoricalDataListener::BondStreamingHistoricalDataListener(
	BondStreamingHistoricalDataService* _bondStreamingHistoricalDataService) :
	bondStreamingHistoricalDataService(_bondStreamingHistoricalDataService)
{
}

void BondStreamingHistoricalDataListener::ProcessAdd(PriceStream<Bond> &data)
{
	string key = data.GetProduct().GetProductId();
	bondStreamingHistoricalDataService->PersistData(key, data);
}

void BondStreamingHistoricalDataListener::ProcessRemove(PriceStream<Bond> &data)
{ // not defined for this service
}

void BondStreamingHistoricalDataListener::ProcessUpdate(PriceStream<Bond> &data)
{ // not defined for this service
}

#endif // !BondStreamingHistoricalDataSoa_hpp
