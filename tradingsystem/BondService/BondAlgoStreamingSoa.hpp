// BondAlgoStreamingSoa.hpp
// 
// Author: Yuchen Liu
// 
// Define bond algo streaming architecture, including 
// bond algo streaming service for the determination of the price to be published, and
// bond algo streaming service listener for the data inflow from bond pricing service

#ifndef BondAlgoStreamingSoa_hpp
#define BondAlgoStreamingSoa_hpp

#include "pricingservice.hpp"
#include "streamingservice.hpp"
#include "products.hpp"
#include "soa.hpp"
#include <unordered_map>

// Algo Stream with a two-way price stream
// Type T is the product type
template<typename T>
class AlgoStream
{
private:
	PriceStream<T> stream;

public:
	AlgoStream(const PriceStream<T>& _stream); // ctor
	AlgoStream() {} // empty default ctor

	// Get the price stream
	const PriceStream<T>& GetStream() const;

};

// Bond algo-streaming service to determine the two-way prices
// key on the product identifier
// value on an AlgoStream object
class BondAlgoStreamingService : public Service<string, AlgoStream<Bond>>
{
protected:
	std::vector<ServiceListener<AlgoStream<Bond>>*> listeners;
	std::unordered_map<string, AlgoStream<Bond>> algostreamMap; // key on product identifier
	long counter = 0; // counter to determine the quantity of the price stream
public:
	BondAlgoStreamingService() {} // empty ctor

	// Get data on our service given a key
	virtual AlgoStream<Bond> & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(AlgoStream<Bond> &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<AlgoStream<Bond>> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<AlgoStream<Bond>>* >& GetListeners() const;

	// Generate the price stream and update it to the stored data
	virtual void AddStream(const Price<Bond>& price);
};

// Bond algo-streaming service listener
// registered into the bond pricing service to process the price data
class BondAlgoStreamingListener : public ServiceListener<Price<Bond>>
{
protected:
	BondAlgoStreamingService* bondAlgoStreamingService;

public:
	BondAlgoStreamingListener(BondAlgoStreamingService* _bondAlgoStreamingService); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(Price<Bond> &data);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(Price<Bond> &data);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(Price<Bond> &data);
};

template <typename T>
AlgoStream<T>::AlgoStream(const PriceStream<T>& _stream) : stream(_stream)
{
}

template <typename T>
const PriceStream<T>& AlgoStream<T>::GetStream() const
{
	return stream;
}

AlgoStream<Bond> & BondAlgoStreamingService::GetData(string key)
{
	return algostreamMap[key];
}

void BondAlgoStreamingService::OnMessage(AlgoStream<Bond> &data)
{ // No OnMessage() defined for the intermediate service 
}

void BondAlgoStreamingService::AddListener(ServiceListener<AlgoStream<Bond>> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<AlgoStream<Bond>>* >& BondAlgoStreamingService::GetListeners() const
{
	return listeners;
}

void BondAlgoStreamingService::AddStream(const Price<Bond>& price)
{
	// mid price and bid-offer spread from price object
	double mid = price.GetMid();
	double spread = price.GetBidOfferSpread();

	// create bid/offer price stream order with hard-coded visible and hidden quantity 
	long visibleQuantity;
	if (counter % 2 == 0) visibleQuantity = 1000000;
	else if (counter % 2 == 1) visibleQuantity = 2000000;
	long hiddenQuantity = 2 * visibleQuantity;
	PriceStreamOrder bidOrder(mid - spread / 2, visibleQuantity, hiddenQuantity, BID);
	PriceStreamOrder offerOrder(mid + spread / 2, visibleQuantity, hiddenQuantity, OFFER);

	// Generate a price stream based on the data
	PriceStream<Bond> stream(price.GetProduct(), bidOrder, offerOrder);

	// Add an algo stream related to the price stream to the stored data
	string productId = price.GetProduct().GetProductId();
	AlgoStream<Bond> algostream(stream);
	if (algostreamMap.find(productId) == algostreamMap.end()) // if not found this one then create one
		algostreamMap.insert(std::make_pair(productId, algostream));
	else
		algostreamMap[productId] = algostream;
	counter++;

	// Call the listeners (update)
	for (auto listener : listeners)
		listener->ProcessUpdate(algostream);
}

BondAlgoStreamingListener::BondAlgoStreamingListener(BondAlgoStreamingService* _bondAlgoStreamingService) :
	bondAlgoStreamingService(_bondAlgoStreamingService)
{
}

void BondAlgoStreamingListener::ProcessAdd(Price<Bond> &data)
{
	// Add a algo stream based on the data
	bondAlgoStreamingService->AddStream(data);
}

void BondAlgoStreamingListener::ProcessRemove(Price<Bond> &data)
{ // not defined for this service
}

void BondAlgoStreamingListener::ProcessUpdate(Price<Bond> &data)
{ // not defined for this service
}


#endif // !BondAlgoStreamingSoa_hpp
