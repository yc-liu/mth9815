// BondMarketDataSoa.hpp
// 
// Author: Yuchen Liu
// 
// Define bond market data architecture, including 
// bond market data service for the market data related to a bond, and
// bond market data connector for the data inflow

#ifndef BondMarketDataSoa_hpp
#define BondMarketDataSoa_hpp

#include "marketdataservice.hpp"
#include "products.hpp"
#include "soa.hpp"
#include "utilityfunction.hpp"
#include "boost/algorithm/string.hpp" // string algorithm
#include "boost/date_time/gregorian/gregorian.hpp" // date operation
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <sstream>

// Bond market data service
class BondMarketDataService : public MarketDataService<Bond>
{
protected:
	std::vector<ServiceListener<OrderBook<Bond>>*> listeners;
	std::unordered_map<string, OrderBook<Bond>> orderbookMap; // key on product identifier
public:
	BondMarketDataService() {} // empty ctor

	// Get data on our service given a key
	virtual OrderBook<Bond> & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(OrderBook<Bond> &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<OrderBook<Bond>> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<OrderBook<Bond>>* >& GetListeners() const;

	// Get the best bid/offer order
	virtual const BidOffer& GetBestBidOffer(const string &productId);

	// Aggregate the order book
	virtual const OrderBook<Bond>& AggregateDepth(const string &productId);
};

// Corresponding subscribe connector
class BondMarketDataConnector : public Connector<OrderBook<Bond>>
{
protected:
	Service<string, OrderBook<Bond>>* bondMarketDataService;

public:
	BondMarketDataConnector(string path, Service<string, OrderBook<Bond>>* _bondMarketDataService, BondProductService* _bondProductService); // ctor

	// Publish data to the Connector
	virtual void Publish(OrderBook <Bond> &data);

};

OrderBook<Bond> & BondMarketDataService::GetData(string key)
{
	return orderbookMap[key];
}

void BondMarketDataService::OnMessage(OrderBook<Bond> &data)
{
	// push the data into map
	string productId = data.GetProduct().GetProductId();
	if (orderbookMap.find(productId) == orderbookMap.end()) // if not found this one then create one
		orderbookMap.insert(std::make_pair(productId, data));
	else
		orderbookMap[productId] = data;

	// call the listeners
	for (auto listener : listeners)
		listener->ProcessAdd(data);
}

void BondMarketDataService::AddListener(ServiceListener<OrderBook<Bond>> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<OrderBook<Bond>>* >& BondMarketDataService::GetListeners() const
{
	return listeners;
}

const BidOffer& BondMarketDataService::GetBestBidOffer(const string &productId)
{
	return orderbookMap[productId].GetBestBidOffer();
}

const OrderBook<Bond>& BondMarketDataService::AggregateDepth(const string &productId)
{
	OrderBook<Bond> orderbook = orderbookMap[productId];
	std::vector<Order> bidOrders = orderbook.GetBidStack();
	std::vector<Order> offerOrders = orderbook.GetOfferStack();
	
	// use map for aggregation
	std::unordered_map<double, long> bidMap;
	std::unordered_map<double, long> offerMap;

	double price; // temp price

	// for bid orders
	for (auto iter = bidOrders.begin(); iter != bidOrders.end(); iter++)
	{
		price = iter->GetPrice();
		if (bidMap.find(price) != bidMap.end()) // if found
			bidMap[price] += iter->GetQuantity();
		else
			bidMap.insert(std::make_pair(price, iter->GetQuantity()));
	}

	// for offer orders
	for (auto iter = offerOrders.begin(); iter != offerOrders.end(); iter++)
	{
		price = iter->GetPrice();
		if (offerMap.find(price) != offerMap.end()) // if found
			offerMap[price] += iter->GetQuantity();
		else
			offerMap.insert(std::make_pair(price, iter->GetQuantity()));
	}

	std::vector<Order> newbidOrders; 
	std::vector<Order> newofferOrders; 

	// re-construct the bid and offer orders
	for (auto iter = bidMap.begin(); iter != bidMap.end(); iter++)
	{
		Order bid(iter->first, iter->second, BID);
		newbidOrders.push_back(bid);
	}
	for (auto iter = offerMap.begin(); iter != offerMap.end(); iter++)
	{
		Order offer(iter->first, iter->second, OFFER);
		newofferOrders.push_back(offer);
	}

	// re-construct the order book
	OrderBook<Bond> neworderbook(orderbook.GetProduct(), newbidOrders, newofferOrders);

	orderbookMap[productId] = neworderbook;
	return orderbookMap[productId];
}

BondMarketDataConnector::BondMarketDataConnector(
	string path, Service<string, OrderBook<Bond>>* _bondMarketDataService, BondProductService* _bondProductService):
	bondMarketDataService(_bondMarketDataService)
{
	fstream file(path, std::ios::in);
	string line;
	std::stringstream ss;
	char separator = ','; // comma seperator

	if (file.is_open())
	{
		std::cout << "Market data: Begin to read data...\n";
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
			// mid price
			double midprice = StringtoPrice<double>(ss, tempData[2]);
			// 5 bid orders and 5 offer orders
			std::vector<Order> bidOrders;
			std::vector<Order> offerOrders;
			for (int i = 1; i <= 5; i++)
			{
				Order bid(midprice - StringtoPrice<double>(ss, tempData[2 + i]), 
					StringtoType<long>(ss, tempData[7 + i]), BID);
				Order offer(midprice + StringtoPrice<double>(ss, tempData[2 + i]),
					StringtoType<long>(ss, tempData[7 + i]), OFFER);
				bidOrders.push_back(bid);
				offerOrders.push_back(offer);
			}

			// order book object
			OrderBook<Bond> orderbook(bond, bidOrders, offerOrders);

			bondMarketDataService->OnMessage(orderbook);
		}
		std::cout << "Market data: finished!\n";
	}
	else
	{
		std::cout << "Oh no! Cannot open the file! Maybe the path is not right?\n";
	}
}
								
void BondMarketDataConnector::Publish(OrderBook <Bond> &data)
{  // undefined publish() for subsribe connector
}

#endif // !BondMarketDataSoa_hpp

