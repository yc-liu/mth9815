// BondStreamingSoa.hpp
// 
// Author: Yuchen Liu
// 
// Define bond streaming architecture, including 
// bond streaming service for publishing the price of the bond, and
// bond streaming service listener for the data inflow from bond algo-streaming service

#ifndef BondStreamingSoa_hpp
#define BondStreamingSoa_hpp

#include "streamingservice.hpp"
#include "BondService/BondAlgoStreamingSoa.hpp"
#include "products.hpp"
#include "soa.hpp"
#include <unordered_map>

// Bond streaming service
class BondStreamingService : public StreamingService<Bond>
{
protected:
	std::vector<ServiceListener<PriceStream<Bond>>*> listeners;
	std::unordered_map<string, PriceStream<Bond>> streamMap; // key on product identifier
public:
	BondStreamingService() {} // empty ctor

	// Get data on our service given a key
	virtual PriceStream<Bond> & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(PriceStream<Bond> &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<PriceStream<Bond>> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<PriceStream<Bond>>* >& GetListeners() const;

	// Publish two-way prices
	void PublishPrice(const PriceStream<Bond>& priceStream);
};

// Bond streaming service listener
class BondStreamingListener : public ServiceListener<AlgoStream<Bond>>
{
protected:
	BondStreamingService* bondStreamingService;

public:
	BondStreamingListener(BondStreamingService* _bondStreamingService); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(AlgoStream<Bond> &data);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(AlgoStream<Bond> &data);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(AlgoStream<Bond> &data);
};

PriceStream<Bond> & BondStreamingService::GetData(string key)
{
	return streamMap[key];
}

void BondStreamingService::OnMessage(PriceStream<Bond> &data)
{ // No OnMessage() defined for the intermediate service 
}

void BondStreamingService::AddListener(ServiceListener<PriceStream<Bond>> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<PriceStream<Bond>>* >& BondStreamingService::GetListeners() const
{
	return listeners;
}

void BondStreamingService::PublishPrice(const PriceStream<Bond>& priceStream)
{
	// push data to the map
	string productId = priceStream.GetProduct().GetProductId();
	if (streamMap.find(productId) == streamMap.end()) // if not found this one then create one
		streamMap.insert(std::make_pair(productId, priceStream));
	else
		streamMap[productId] = priceStream;

	// call the listeners
	PriceStream<Bond> temp(priceStream);
	for (auto listener : listeners)
		listener->ProcessAdd(temp);
}

BondStreamingListener::BondStreamingListener(BondStreamingService* _bondStreamingService):
	bondStreamingService(_bondStreamingService)
{
}

void BondStreamingListener::ProcessAdd(AlgoStream<Bond> &data)
{ // not defined for this service
}

void BondStreamingListener::ProcessRemove(AlgoStream<Bond> &data)
{ // not defined for this service
}

void BondStreamingListener::ProcessUpdate(AlgoStream<Bond> &data)
{
	// publish the price stream
	PriceStream<Bond> stream = data.GetStream();
	bondStreamingService->PublishPrice(stream);

}


#endif // !BondStreamingSoa_hpp
