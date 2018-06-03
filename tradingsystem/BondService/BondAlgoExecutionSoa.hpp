// BondAlgoExecutionSoa.hpp
// 
// Author: Yuchen Liu
// 
// Define bond algo execution architecture, including 
// bond algo execution service for the determination of the order to be executed, and
// bond algo execution service listener for the data inflow from bond market data service

#ifndef BondAlgoExecutionSoa_hpp
#define BondAlgoExecutionSoa_hpp

#include "marketdataservice.hpp"
#include "executionservice.hpp"
#include "products.hpp"
#include "soa.hpp"
#include <unordered_map>
#include <sstream>

// Algo Execution with an execution order object
// Type T is the product type
template <typename T>
class AlgoExecution
{
private:
	ExecutionOrder<T> order;

public:
	AlgoExecution(const ExecutionOrder<T>& _order); // ctor
	AlgoExecution() {} // empty default ctor

	// Get the execution order
	const ExecutionOrder<T>& GetOrder() const;
};


// Bond algo-execution service to determine the execution order
// key on the product identifier
// value on an AlgoExecution object
class BondAlgoExecutionService : public Service<string, AlgoExecution<Bond>>
{
protected:
	std::vector<ServiceListener<AlgoExecution<Bond>>*> listeners;
	std::unordered_map<string, AlgoExecution<Bond>> algoexecutionMap; // key on product identifier
	long counter = 0; // counter to determine the side of the algo execution

public:
	BondAlgoExecutionService() {} // empty ctor

	// Get data on our service given a key
	virtual AlgoExecution<Bond> & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(AlgoExecution<Bond> &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<AlgoExecution<Bond>> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<AlgoExecution<Bond>>* >& GetListeners() const;

	// Generate the execution order and update it to the stored data
	virtual void AddOrder(const OrderBook<Bond>& orderBook);
};

// Bond algo-execution service listener
// registered into the bond market data service to process the orderbook data
class BondAlgoExecutionListener : public ServiceListener<OrderBook<Bond>>
{
protected:
	BondAlgoExecutionService* bondAlgoExecutionService;

public:
	BondAlgoExecutionListener(BondAlgoExecutionService* _bondAlgoExecutionService); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(OrderBook<Bond> &data);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(OrderBook<Bond> &data);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(OrderBook<Bond> &data);
};

template <typename T>
AlgoExecution<T>::AlgoExecution(const ExecutionOrder<T>& _order) : order(_order)
{
}


template <typename T>
const ExecutionOrder<T>& AlgoExecution<T>::GetOrder() const
{
	return order;
}

AlgoExecution<Bond> & BondAlgoExecutionService::GetData(string key)
{
	return algoexecutionMap[key];
}

void BondAlgoExecutionService::OnMessage(AlgoExecution<Bond> &data)
{ // No OnMessage() defined for the intermediate service 
}

void BondAlgoExecutionService::AddListener(ServiceListener<AlgoExecution<Bond>> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<AlgoExecution<Bond>>* >& BondAlgoExecutionService::GetListeners() const
{
	return listeners;
}

void BondAlgoExecutionService::AddOrder(const OrderBook<Bond>& orderBook)
{
	string productId = orderBook.GetProduct().GetProductId();

	// Get the best bid and offer in the order book
	BidOffer bestBidOffer = orderBook.GetBestBidOffer();

	// Generate an execution order only if the spread is tightest
	double bestbid = bestBidOffer.GetOfferOrder().GetPrice();
	double bestoffer = bestBidOffer.GetBidOrder().GetPrice();
	if (bestoffer - bestbid <= 1.0 / 128)
	{
		// determine the attributes of the execution order
		// order ID (e.g. ORD2024T0001040)
		std::stringstream ss;
		ss << "ORD" << std::to_string(orderBook.GetProduct().GetMaturityDate().year()) << orderBook.GetProduct().GetTicker()
			<< std::setfill('0') << std::setw(7) << std::to_string(counter);
		string orderId = ss.str();
		ss.str(""); // release the buffer

		// parent order ID (null)
		string parentOrderId = "N/A";

		// ischildOrder (always false)
		bool isChildOrder = false;

		// quantity
		long allQuantity, visibleQuantity, hiddenQuantity;

		// order type is always immediate-or-cancel
		OrderType type = IOC;

		// determine the side of the execution
		PricingSide side = OFFER;
		if (counter % 2 == 1) side = BID;

		if (side == OFFER) 
		{
			// determine the quantity with visible : hidden = 1 : 4 (hard-coded)
			allQuantity = bestBidOffer.GetOfferOrder().GetQuantity();
			visibleQuantity = allQuantity / 5;
			hiddenQuantity = allQuantity - visibleQuantity;

			// generate the execution order
			ExecutionOrder<Bond> execution(orderBook.GetProduct(), side, orderId, type,
				bestoffer, visibleQuantity, hiddenQuantity, parentOrderId, isChildOrder);

			// Add an algo execution related to the execution order to the stored data
			AlgoExecution<Bond> algoexecution(execution);
			if (algoexecutionMap.find(productId) == algoexecutionMap.end()) // if not found this one then create one
				algoexecutionMap.insert(std::make_pair(productId, algoexecution));
			else
				algoexecutionMap[productId] = algoexecution;

			// Call the listeners (update)
			for (auto listener : listeners)
				listener->ProcessUpdate(algoexecution);
		}
		else if (side == BID) // and now is the bid
		{
			// determine the quantity with visible : hidden = 1 : 4 (hard-coded)
			allQuantity = bestBidOffer.GetBidOrder().GetQuantity();
			visibleQuantity = allQuantity / 5;
			hiddenQuantity = allQuantity - visibleQuantity;

			// generate the execution order
			ExecutionOrder<Bond> execution(orderBook.GetProduct(), side, orderId, type,
				bestbid, visibleQuantity, hiddenQuantity, parentOrderId, isChildOrder);

			// Add an algo execution related to the execution order to the stored data
			AlgoExecution<Bond> algoexecution(execution);
			if (algoexecutionMap.find(productId) == algoexecutionMap.end()) // if not found this one then create one
				algoexecutionMap.insert(std::make_pair(productId, algoexecution));
			else
				algoexecutionMap[productId] = algoexecution;

			// Call the listeners (update)
			for (auto listener : listeners)
				listener->ProcessUpdate(algoexecution);
		}	
		counter++;
	}

}

BondAlgoExecutionListener::BondAlgoExecutionListener(BondAlgoExecutionService* _bondAlgoExecutionService):
	bondAlgoExecutionService(_bondAlgoExecutionService)
{
}

void BondAlgoExecutionListener::ProcessAdd(OrderBook<Bond> &data)
{
	bondAlgoExecutionService->AddOrder(data);
}

void BondAlgoExecutionListener::ProcessRemove(OrderBook<Bond> &data)
{ // not defined for this service
}

void BondAlgoExecutionListener::ProcessUpdate(OrderBook<Bond> &data)
{ // not defined for this service
}

#endif // ! BondAlgoExecutionSoa_hpp
