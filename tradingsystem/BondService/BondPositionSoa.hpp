// BondPositionSoa.hpp
// 
// Author: Yuchen Liu
// 
// Define bond position architecture, including 
// bond position service for updating the position of bonds, and
// bond position service listener for the data inflow from bond trade book service

#ifndef BondPositionSoa_hpp
#define BondPositionSoa_hpp

#include "productservice.hpp"
#include "positionservice.hpp"
#include "tradebookingservice.hpp"
#include "products.hpp"
#include "soa.hpp"
#include <unordered_map>

// Bond position service
class BondPositionService : public PositionService<Bond>
{
protected:
	std::vector<ServiceListener<Position<Bond>>*> listeners;
	std::unordered_map<string, Position<Bond>> positionMap; // key on product identifier

public:
	BondPositionService(BondProductService* bondProductService, std::string ticker); 

	// Get data on our service given a key
	virtual Position<Bond> & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(Position<Bond> &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<Position<Bond>> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<Position<Bond>>* >& GetListeners() const;

	// Add a trade to the service
	virtual void AddTrade(const Trade<Bond> &trade);

};

// corresponding service listener
class BondPositionListener : public ServiceListener<Trade<Bond>>
{
protected:
	BondPositionService* bondPositionService;

public:
	BondPositionListener(BondPositionService* _bondPositionService); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(Trade<Bond> &data);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(Trade<Bond> &data);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(Trade<Bond> &data);
};

BondPositionService::BondPositionService(BondProductService* bondProductService, std::string ticker)
{
	std::vector<Bond> products = bondProductService->GetBonds(ticker);

	// initialize the position map
	for (auto iter = products.begin(); iter != products.end(); iter++)
	{
		Position<Bond> position(*iter);
		positionMap.insert(std::make_pair(iter->GetProductId(), position));
	}
}

Position<Bond> & BondPositionService::GetData(string key)
{
	return positionMap[key];
}

void BondPositionService::OnMessage(Position<Bond> &data)
{ // No OnMessage() defined for the intermediate service 
}

void BondPositionService::AddListener(ServiceListener<Position<Bond>> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<Position<Bond>>* >& BondPositionService::GetListeners() const
{
	return listeners;
}

void BondPositionService::AddTrade(const Trade<Bond> &trade)
{
	// Update the position based on this trade
	string productId = trade.GetProduct().GetProductId();
	string book = trade.GetBook();
	int sign = (trade.GetSide() == BUY) ? 1 : -1;
	long quantity = sign * trade.GetQuantity();
	Position<Bond> position = positionMap[productId];
	position.AddNewPosition(book, quantity);
	positionMap[productId] = position;

	// Send this position to the listeners
	for (auto listener : listeners)
		listener->ProcessUpdate(position);
	

}

BondPositionListener::BondPositionListener(BondPositionService* _bondPositionService) :
	bondPositionService(_bondPositionService)
{
}

void BondPositionListener::ProcessAdd(Trade<Bond> &data)
{ // not defined for this service
}

void BondPositionListener::ProcessRemove(Trade<Bond> &data)
{ // not defined for this service
}

void BondPositionListener::ProcessUpdate(Trade<Bond> &data)
{
	bondPositionService->AddTrade(data);
}


#endif // !BondPositionSoa_hpp
