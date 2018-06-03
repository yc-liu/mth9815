// BondPricingSoa.hpp
// 
// Author: Yuchen Liu
// 
// Define bond pricing architecture, including 
// bond pricing service for pricing a bond object, and
// bond pricing connector for the data inflow

#ifndef BondPricingSoa_hpp
#define BondPricingSoa_hpp

#include "pricingservice.hpp"
#include "products.hpp"
#include "soa.hpp"
#include "utilityfunction.hpp"
#include "productservice.hpp"
#include "boost/algorithm/string.hpp" // string algorithm
#include "boost/date_time/gregorian/gregorian.hpp" // date operation
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iostream>

// Bond pricing service
class BondPricingService : public PricingService<Bond>
{
protected:
	std::vector<ServiceListener<Price<Bond>>*> listeners;
	std::unordered_map<string, Price<Bond>> priceMap; // key on product identifier

public:
	BondPricingService() {} // empty ctor

	// Get data on our service given a key
	virtual Price<Bond> & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(Price<Bond> &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<Price<Bond>> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<Price<Bond>>* >& GetListeners() const;


};

// Corresponding subscribe connector
class BondPricingConnector : public Connector<Price<Bond>>
{
protected:
	Service<string,Price <Bond>>* bondPricingService;

public:
	BondPricingConnector(string path, 
		Service<string, Price <Bond>>* _bondPricingService, BondProductService* _bondProductService); // ctor

	// Publish data to the Connector
	virtual void Publish(Price <Bond> &data);

};

Price<Bond> & BondPricingService::GetData(string key)
{
	return priceMap[key]; 
}

void BondPricingService::OnMessage(Price<Bond> &data)
{
	// push the data into map
	string productId = data.GetProduct().GetProductId();
	if (priceMap.find(productId) == priceMap.end()) // if not found this one then create one
		priceMap.insert(std::make_pair(productId, data));
	else
		priceMap[productId] = data;

	// call the listeners
	for (auto listener : listeners)
		listener->ProcessAdd(data);

}

void BondPricingService::AddListener(ServiceListener<Price<Bond>> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<Price<Bond>>* >& BondPricingService::GetListeners() const
{
	return listeners;
}

BondPricingConnector::BondPricingConnector(string path,
	Service<string, Price <Bond>>* _bondPricingService, BondProductService* _bondProductService):
	bondPricingService(_bondPricingService)
{
	fstream file(path, std::ios::in);
	string line;
	std::stringstream ss;
	char separator = ','; // comma seperator

	if (file.is_open())
	{
		std::cout << "Price: Begin to read data...\n";
		getline(file, line); // discard header

		while (getline(file, line))
		{
			stringstream sin(line);

			// save all trimmed data into a vector of strings
			std::vector<string> tempData;
			string info;
			while (getline(sin, info, separator))
			{
				boost::algorithm::trim(info);
				tempData.push_back(info);
			}

			// make the corresponding trade object
			// bond Id
			BondIdType type = (boost::algorithm::to_upper_copy(tempData[0]) == "CUSIP") ? CUSIP : ISIN;
			string bondId = tempData[1];
			// the bond product
			Bond bond = _bondProductService->GetData(bondId); // bond id, bond id type, ticker, coupon, maturity
			// bond price
			double mid = StringtoPrice<double>(ss, tempData[2]);
			// bond price spread
			double spread = StringtoPrice<double>(ss, tempData[3]);

			// price object
			Price<Bond> price(bond, mid, spread);

			bondPricingService->OnMessage(price);
		}
		std::cout << "Price: finished!\n";
	}
	else
	{
		std::cout << "Oh no! Cannot open the file! Maybe the path is not right?\n";
	}
}

void BondPricingConnector::Publish(Price <Bond> &data)
{ // undefined publish() for subsribe connector
}

#endif // !BondPricingSoa_hpp
