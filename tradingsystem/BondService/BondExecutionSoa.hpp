// BondExecutionSoa.hpp
// 
// Author: Yuchen Liu
// 
// Define bond execution architecture, including 
// bond execution service for executing the order, and
// bond execution service listener for the data inflow from bond algo execution service

#ifndef BondExecutionSoa_hpp
#define BondExecutionSoa_hpp

#include "executionservice.hpp"
#include "BondService/BondAlgoExecutionSoa.hpp"
#include "products.hpp"
#include "soa.hpp"
#include <unordered_map>
#include <random> // to determine the market where the order is executed

// Bond execution service
class BondExecutionService : public ExecutionService<Bond>
{
protected:
	std::vector<ServiceListener<ExecutionOrder<Bond>>*> listeners;
	std::unordered_map<string, ExecutionOrder<Bond>> orderMap; // key on product identifier

public:
	BondExecutionService() {} // empty ctor

	// Get data on our service given a key
	virtual ExecutionOrder<Bond> & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(ExecutionOrder<Bond> &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<ExecutionOrder<Bond>> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<ExecutionOrder<Bond>>* >& GetListeners() const;

	// Execute an order on a market
	void ExecuteOrder(const ExecutionOrder<Bond>& order, Market market);
};


// Bond streaming service listener
class BondExecutionListener : public ServiceListener<AlgoExecution<Bond>>
{
protected:
	BondExecutionService* bondExecutionService;

public:
	BondExecutionListener(BondExecutionService* _bondExecutionService); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(AlgoExecution<Bond> &data);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(AlgoExecution<Bond> &data);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(AlgoExecution<Bond> &data);
};


ExecutionOrder<Bond> & BondExecutionService::GetData(string key)
{
	return orderMap[key];
}

void BondExecutionService::OnMessage(ExecutionOrder<Bond> &data)
{ // No OnMessage() defined for the intermediate service 
}

void BondExecutionService::AddListener(ServiceListener<ExecutionOrder<Bond>> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<ExecutionOrder<Bond>>* >& BondExecutionService::GetListeners() const
{
	return listeners;
}

void BondExecutionService::ExecuteOrder(const ExecutionOrder<Bond>& order, Market market)
{
	// execute the order (push data to the map)
	string productId = order.GetProduct().GetProductId();
	if (orderMap.find(productId) == orderMap.end()) // if not found this one then create one
		orderMap.insert(std::make_pair(productId, order));
	else
		orderMap[productId] = order;

	// call the listeners
	ExecutionOrder<Bond> temp(order);
	for (auto listener : listeners)
		listener->ProcessAdd(temp);
}

BondExecutionListener::BondExecutionListener(BondExecutionService* _bondExecutionService):
	bondExecutionService(_bondExecutionService)
{
}

void BondExecutionListener::ProcessAdd(AlgoExecution<Bond> &data)
{ // not defined for this service
}

void BondExecutionListener::ProcessRemove(AlgoExecution<Bond> &data)
{ // not defined for this service
}

void BondExecutionListener::ProcessUpdate(AlgoExecution<Bond> &data)
{
	// determine the exchange where the order is executed
	int val = rand() % 3;
	Market exchange = CME;
	switch (val)
	{
	case 0: exchange = BROKERTEC; break;
	case 1: exchange = ESPEED; break;
	default: break;
	}

	// call the order execution
	ExecutionOrder<Bond> order = data.GetOrder();
	bondExecutionService->ExecuteOrder(order, exchange); 
	
}

#endif // !BondExecutionSoa_hpp
