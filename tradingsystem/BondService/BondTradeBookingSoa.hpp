// BondTradeBookingSoa.hpp
// 
// Author: Yuchen Liu
// 
// Define bond trade book architecture, including 
// bond trade book service for booking the trade of bonds,
// bond trade book connector for the data inflow from outside, and 
// bond trade book service listener for the data inflow from bond execution service

#ifndef BondTradeBookingSoa_hpp
#define BondTradeBookingSoa_hpp

#include "tradebookingservice.hpp"
#include "BondService/BondExecutionSoa.hpp"
#include "products.hpp"
#include "soa.hpp"
#include "utilityfunction.hpp"
#include "productservice.hpp"
#include "boost/algorithm/string.hpp" // string algorithm
#include "boost/date_time/gregorian/gregorian.hpp" // date operation
#include <vector>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <iostream>

// Bond trade booking service
class BondTradeBookingService : public TradeBookingService<Bond>
{
protected:
	std::vector<ServiceListener<Trade<Bond>>*> listeners;
	long counter = 0; // counter to determine the trade book of the trade coming from bond execution service
	std::unordered_map<string, Trade<Bond>> tradeMap; // key on trade identifier

public:
	BondTradeBookingService() {} // empty ctor

	// Get data on our service given a key
	virtual Trade<Bond> & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(Trade<Bond> &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<Trade<Bond>> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<Trade<Bond>>* >& GetListeners() const;

	// Book the trade
	virtual void BookTrade(const Trade<Bond> &trade);

	// Get the current value of counter
	const long GetCounter() const;

};

// corresponding subscribe connector
class BondTradeBookingConnector : public Connector<Trade<Bond>>
{
protected:
	Service<string, Trade<Bond>>* bondTradeBookingService;

public:
	BondTradeBookingConnector(string path, 
		Service<string, Trade<Bond>>* _bondTradeBookingService, BondProductService* _bondProductService); // ctor

	// Publish data to the Connector
	virtual void Publish(Trade <Bond> &data);

};

// corresponding service listener
class BondTradeBookingListener : public ServiceListener<ExecutionOrder<Bond>>
{
protected:
	BondTradeBookingService* bondTradeBookingService;

public:
	BondTradeBookingListener(BondTradeBookingService* _bondTradeBookingService); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(ExecutionOrder<Bond> &data);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(ExecutionOrder<Bond> &data);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(ExecutionOrder<Bond> &data);
};

Trade<Bond> & BondTradeBookingService::GetData(string key)
{
	return tradeMap[key]; 

}

void BondTradeBookingService::OnMessage(Trade<Bond> &data)
{
	// book the trade
	BookTrade(data);

}

void BondTradeBookingService::AddListener(ServiceListener<Trade<Bond>> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<Trade<Bond>>* >& BondTradeBookingService::GetListeners() const
{
	return listeners;
}

void BondTradeBookingService::BookTrade(const Trade<Bond> &trade)
{
	string tradeId = trade.GetTradeId();
	if (tradeMap.find(tradeId) == tradeMap.end()) // if not found this one then create one
		tradeMap.insert(std::make_pair(tradeId, trade));
	else
		tradeMap[tradeId] = trade;
	counter++; // Increment the counter

	// call the listeners
	Trade<Bond> temp(trade);
	for (auto listener : listeners)
		listener->ProcessUpdate(temp);
}

const long BondTradeBookingService::GetCounter() const
{
	return counter;
}

BondTradeBookingConnector::BondTradeBookingConnector(
	string path, Service<string, Trade<Bond>>* _bondTradeBookingService, BondProductService* _bondProductService):
	bondTradeBookingService(_bondTradeBookingService)
{
	fstream file(path, std::ios::in);
	string line;
	std::stringstream ss;
	char separator = ','; // comma seperator

	if (file.is_open())
	{
		std::cout << "Trade: Begin to read data...\n";
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
			// trade Id
			string tradeId = tempData[0];
			// bond Id
			BondIdType type = (boost::algorithm::to_upper_copy(tempData[1]) == "CUSIP") ? CUSIP : ISIN;
			string bondId = tempData[2];
			// the bond product
			Bond bond = _bondProductService->GetData(bondId); // bond id, bond id type, ticker, coupon, maturity
			// trade side
			Side side = (boost::algorithm::to_upper_copy(tempData[3]) == "BUY") ? BUY : SELL;
			// trade quantity
			long quantity = StringtoType<long>(ss, tempData[4]);
			// trade price
			double price = StringtoPrice<double>(ss, tempData[5]);
			// book id
			string bookId = tempData[6];

			// trade object
			Trade<Bond> trade(bond, tradeId, price, bookId, quantity, side);

			bondTradeBookingService->OnMessage(trade);
		}
		std::cout << "Trade: finished!\n";
	}
	else
	{
		std::cout << "Oh no! Cannot open the file! Maybe the path is not right?\n";
	}
}

void BondTradeBookingConnector::Publish(Trade <Bond> &data)
{ // undefined publish() for subsribe connector
}

BondTradeBookingListener::BondTradeBookingListener(BondTradeBookingService* _bondTradeBookingService) :
	bondTradeBookingService(_bondTradeBookingService)
{
}


void BondTradeBookingListener::ProcessAdd(ExecutionOrder<Bond> &data)
{
	// Determine the atributes of the trade
	long counter = bondTradeBookingService->GetCounter();
	Bond bond = data.GetProduct();
	// Trade ID (e.g. TRS2024T0000023)
	std::stringstream ss;
	ss << "TRS" << std::to_string(bond.GetMaturityDate().year()) << bond.GetTicker()
		<< std::setfill('0') << std::setw(7) << std::to_string(counter);
	string tradeId = ss.str();
	ss.str(""); // release the buffer
	// 'hard-coded' determine the book id
	string bookId;
	switch (counter % 3)
	{
	case 0: bookId = "TRSY1"; break;
	case 1: bookId = "TRSY2"; break;
	default: bookId = "TRSY3"; break;
	}
	// determine the side
	Side side = BUY;
	if (data.GetSide() == BID)
		side = SELL;

	// generate a trade based on the coming execution order
	Trade<Bond> trade(data.GetProduct(), tradeId, data.GetPrice(), bookId,
		data.GetHiddenQuantity() + data.GetVisibleQuantity(), side);

	// book the trade
	bondTradeBookingService->BookTrade(trade);

}

void BondTradeBookingListener::ProcessRemove(ExecutionOrder<Bond> &data)
{ // not defined for this service
}

void BondTradeBookingListener::ProcessUpdate(ExecutionOrder<Bond> &data)
{ // not defined for this service
}

#endif // !BondTradeBookingSoa_hpp
