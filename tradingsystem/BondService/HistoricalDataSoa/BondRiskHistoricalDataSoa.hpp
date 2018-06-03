// BondRiskHistoricalDataSoa.hpp
// 
// Author: Yuchen Liu
// 
// Define bond risk historical data architecture, including 
// bond risk historical data service for maintaining the pv01 data, and
// bond risk historical data service connector for publishing data, and 
// bond risk historical data service listener for data inflow from bond risk service

#ifndef BondRiskHistoricalDataSoa_hpp
#define BondRiskHistoricalDataSoa_hpp

#include "historicaldataservice.hpp"
#include "BondService/BondRiskSoa.hpp"
#include "products.hpp"
#include "soa.hpp"
#include "utilityfunction.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <fstream>

class BondRiskHistoricalDataConnector;// : public Connector<PV01<Bond>>;

// Bond historical data service for position data
class BondRiskHistoricalDataService : public HistoricalDataService<PV01<Bond>>
{
protected:
	std::vector<ServiceListener<PV01<Bond>>*> listeners;
	BondRiskHistoricalDataConnector* bondRiskHistoricalDataConnector;
	std::unordered_map<string, PV01<Bond>> pv01Map; // key on product indentifier
	std::unordered_map<string, PV01<BucketedSector<Bond>>> bucketpv01Map; // key on sector name

public:
	BondRiskHistoricalDataService(BondRiskHistoricalDataConnector* _bondRiskHistoricalDataConnector); // ctor

	// Get data on our service given a key : for a single bond
	virtual PV01<Bond> & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(PV01<Bond> &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<PV01<Bond>> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<PV01<Bond>>* >& GetListeners() const;

	// Persist data to a store: for a sigle bond
	virtual void PersistData(string persistKey, const PV01<Bond>& data);

	// Persist data to a store: for bucket sector
	virtual void PersistData(string persistKey, const PV01<BucketedSector<Bond>>& data);
};

// corresponding publish connector
class BondRiskHistoricalDataConnector : public Connector<PV01<Bond>>
{
protected:
	fstream file;

public:
	BondRiskHistoricalDataConnector(string _path); // ctor

	// Publish data to the Connector: for single bond
	virtual void Publish(PV01 <Bond> &data);

	// Publish data to the Connector: for bucketed sector
	virtual void Publish(PV01 <BucketedSector<Bond>> &data);

};

// corresponding service listener
class BondRiskHistoricalDataListener : public ServiceListener<PV01<Bond>>
{
protected:
	BondProductService* bondProductService;
	BondRiskHistoricalDataService* bondRiskHistoricalDataService;
	BondRiskService* bondRiskService;
	std::vector<BucketedSector<Bond>> buckets; // for buckets classification

public:
	BondRiskHistoricalDataListener(BondProductService* _bondProductService, 
		BondRiskHistoricalDataService* _bondRiskHistoricalDataService,BondRiskService* _bondRiskService, 
		std::unordered_map<std::string, std::vector<std::string>>& _bucketMap); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(PV01<Bond> &data);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(PV01<Bond> &data);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(PV01<Bond> &data);
};

BondRiskHistoricalDataService::BondRiskHistoricalDataService(BondRiskHistoricalDataConnector*
	_bondRiskHistoricalDataConnector) : bondRiskHistoricalDataConnector(_bondRiskHistoricalDataConnector)
{
}

PV01<Bond> & BondRiskHistoricalDataService::GetData(string key)
{
	return pv01Map[key];
}

void BondRiskHistoricalDataService::OnMessage(PV01<Bond> &data)
{ // No OnMessage() defined for the intermediate service 
}

void BondRiskHistoricalDataService::AddListener(ServiceListener<PV01<Bond>> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<PV01<Bond>>* >& BondRiskHistoricalDataService::GetListeners() const
{
	return listeners;
}

void BondRiskHistoricalDataService::PersistData(string persistKey, const PV01<Bond>& data)
{
	// push data into the map
	if (pv01Map.find(persistKey) == pv01Map.end()) // if not found this one then create one
		pv01Map.insert(std::make_pair(persistKey, data));
	else
		pv01Map[persistKey] = data;

	// publish the data
	PV01<Bond> temp(data);
	bondRiskHistoricalDataConnector->Publish(temp);

}

void BondRiskHistoricalDataService::PersistData(string persistKey, const PV01<BucketedSector<Bond>>& data)
{
	// push data into the map
	if (bucketpv01Map.find(persistKey) == bucketpv01Map.end()) // if not found this one then create one
		bucketpv01Map.insert(std::make_pair(persistKey, data));
	else
		bucketpv01Map[persistKey] = data;

	// publish the data
	PV01<BucketedSector<Bond>> temp(data);
	bondRiskHistoricalDataConnector->Publish(temp);
}

BondRiskHistoricalDataConnector::BondRiskHistoricalDataConnector(string _path) :
	file(_path, std::ios::out | std::ios::trunc)
{
	// set the header of the output file
	file << "Time,ProductIDType,ProductID,PV01,Quantity\n";
}

void BondRiskHistoricalDataConnector::Publish(PV01 <Bond> &data)
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
			<< std::to_string(data.GetPV01()) << "," << std::to_string(data.GetQuantity()) << "\n";
	}
	else
	{
		std::cout << "Oh no! Cannot open the file! Maybe the path is not right?\n";
	}
}

void BondRiskHistoricalDataConnector::Publish(PV01 <BucketedSector<Bond>> &data)
{
	if (file.is_open())
	{
		// make the ingredent of the outout
		auto time = boost::posix_time::microsec_clock::local_time(); // current time
		std::string date = DatetoUsString(time.date());
		std::string timeofDay = boost::posix_time::to_simple_string(time.time_of_day());
		timeofDay.erase(timeofDay.end() - 3, timeofDay.end());
		std::string Idtype = "Bucketed Sector"; // special id type

		// make the output
		file << date << " " << timeofDay << "," << Idtype << "," << data.GetProduct().GetName() << ","
			<< std::to_string(data.GetPV01()) << "," << std::to_string(data.GetQuantity()) << "\n";
	}
	else
	{
		std::cout << "Oh no! Cannot open the file! Maybe the path is not right?\n";
	}
}

BondRiskHistoricalDataListener::BondRiskHistoricalDataListener(BondProductService* _bondProductService,
	BondRiskHistoricalDataService* _bondRiskHistoricalDataService, BondRiskService* _bondRiskService,
	std::unordered_map<std::string, std::vector<std::string>>& _bucketMap) : bondProductService(_bondProductService),
	bondRiskHistoricalDataService(_bondRiskHistoricalDataService), bondRiskService(_bondRiskService)
{
	// initialize the bucketed sector
	for (auto iter = _bucketMap.begin(); iter != _bucketMap.end(); iter++)
	{
		std::vector<Bond> bonds;
		for (auto iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++)
			bonds.push_back(bondProductService->GetData(*iter2));
		buckets.push_back(BucketedSector<Bond>(bonds, iter->first));
	}
}

void BondRiskHistoricalDataListener::ProcessAdd(PV01<Bond> &data)
{ // not defined for this service
}

void BondRiskHistoricalDataListener::ProcessRemove(PV01<Bond> &data)
{ // not defined for this service
}

void BondRiskHistoricalDataListener::ProcessUpdate(PV01<Bond> &data)
{ 
	// persist data for the single product
	string key = data.GetProduct().GetProductId();
	bondRiskHistoricalDataService->PersistData(key, data);

	// find the bucketed sector the product is in
	std::vector<Bond> products;
	bool status = false; // whether the bucketed sector is found
	int index; // the index of the found bucketed sector in the buckets vector
	int n = buckets.size(); // # of bucketed sector of the buckets vector
	for (int i = 0; i< n;i++)
	{
		// find the bucket the product is in
		products = buckets[i].GetProducts();
		for (auto iter2 = products.begin(); iter2 != products.end(); iter2++)
		{
			if (iter2->GetProductId() == key) // if found
			{
				index = i;
				status = true;
				break;
			}
		}
		
		if (status) // if already found
			break;
	}

	// generate the corresponding bucketed risk update and persist it via the service
	if (status) // if found
	{
		bondRiskService->UpdateBucketedRisk(buckets[index]); // update the bucketed risk
		PV01<BucketedSector<Bond>> bucketpv01 = bondRiskService->GetBucketedRisk(buckets[index]);
		bondRiskHistoricalDataService->PersistData(buckets[index].GetName(), bucketpv01);
	}
	else
	{
		std::cout << "Unfounded bucketed sector for the underlying product of this risk update!!\n";
	}

}

#endif // !BondRiskHistoricalDataSoa_hpp
