// BondRiskSoa.hpp
// 
// Author: Yuchen Liu
// 
// Define bond risk architecture, including 
// bond risk service for modeling the risk of bond positions, and
// bond risk service listener for the data inflow from bond position service

#ifndef BondRiskSoa_hpp
#define BondRiskSoa_hpp

#include "riskservice.hpp"
#include "positionservice.hpp"
#include "productservice.hpp"
#include "products.hpp"
#include "soa.hpp"
#include <unordered_map>
#include <vector>

// Bond risk service
class BondRiskService : public RiskService<Bond>
{
protected:
	BondProductService* bondProductService;
	std::vector<ServiceListener<PV01<Bond>>*> listeners;
	std::unordered_map<string, PV01<Bond>> pv01Map; // key on product identifier
	std::unordered_map<string, PV01<BucketedSector<Bond>>> bucketpv01Map; // key on sector name

public:
	BondRiskService(BondProductService* _bondProductService, std::unordered_map<string, double>& _pv01); // ctor

	// Get data on our service given a key
	virtual PV01<Bond> & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(PV01<Bond> &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<PV01<Bond>> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<PV01<Bond>>* >& GetListeners() const;

	// Add a position that the service will risk
	virtual void AddPosition(Position<Bond> &position);

	// Update the bucketed risk for the bucket sector
	virtual void UpdateBucketedRisk(const BucketedSector<Bond> &sector);

	// Get the bucketed risk for the bucket sector
	virtual const PV01<BucketedSector<Bond>>& GetBucketedRisk(const BucketedSector<Bond> &sector) const;
};

// corresponding service listener
class BondRiskListener : public ServiceListener<Position<Bond>>
{
protected:
	BondRiskService* bondRiskService;

public:
	BondRiskListener(BondRiskService* _bondRiskService); // ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(Position<Bond> &data);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(Position<Bond> &data);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(Position<Bond> &data);
};

BondRiskService::BondRiskService(BondProductService* _bondProductService, std::unordered_map<string, double>& _pv01):
	bondProductService(_bondProductService)
{
	// initialize the pv01 map
	for (auto iter = _pv01.begin(); iter != _pv01.end(); iter++)
	{
		string productId = iter->first;
		pv01Map.insert(std::make_pair(productId, 
			PV01<Bond>(bondProductService->GetData(productId), iter->second, 0)));
	}
}

PV01<Bond> & BondRiskService::GetData(string key)
{
	return pv01Map[key];
}

void BondRiskService::OnMessage(PV01<Bond> &data)
{ // No OnMessage() defined for the intermediate service 
}

void BondRiskService::AddListener(ServiceListener<PV01<Bond>> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<PV01<Bond>>* >& BondRiskService::GetListeners() const
{
	return listeners;
}

void BondRiskService::AddPosition(Position<Bond> &position)
{
	// retrieve the corresponding pv01
	string productId = position.GetProduct().GetProductId();
	PV01<Bond> productPv = pv01Map[productId];

	// Update the pv01 object
	long long newQuantity = position.GetAggregatePosition() + productPv.GetQuantity();
	PV01<Bond> productNewPv(productPv.GetProduct(), productPv.GetPV01(), newQuantity);
	pv01Map[productId] = productNewPv;
		
	// call the listeners with the pv01 update of a single product
	for (auto listener : listeners)
		listener->ProcessUpdate(productNewPv); 
	
	
}

void BondRiskService::UpdateBucketedRisk(const BucketedSector<Bond> &sector)
{
	std::vector<Bond> products = sector.GetProducts();
	string productId;
	long long sum_quantity = 0;
	double sum_pv01 = 0.0;

	// calculate the overall pv01 and the overall quantity
	for (Bond product : products)
	{
		productId = product.GetProductId();
		sum_quantity += pv01Map.at(productId).GetQuantity();
		sum_pv01 += pv01Map.at(productId).GetPV01() * pv01Map.at(productId).GetQuantity();
	}

	double unit_pv01 = 0.0;
	if (sum_quantity != 0)
		unit_pv01 = sum_pv01 / sum_quantity;

	PV01<BucketedSector<Bond>> bucketpv01(sector, unit_pv01, sum_quantity);

	// push the data to the map
	string name = sector.GetName();
	if (bucketpv01Map.find(name) == bucketpv01Map.end()) // if not found this one then create one
		bucketpv01Map.insert(std::make_pair(name, bucketpv01));
	else
		bucketpv01Map[name] = bucketpv01;
}

const PV01<BucketedSector<Bond>>& BondRiskService::GetBucketedRisk(const BucketedSector<Bond> &sector) const
{
	 std::string name = sector.GetName();
	 return bucketpv01Map.at(name);

}

BondRiskListener::BondRiskListener(BondRiskService* _bondRiskService) :
	bondRiskService(_bondRiskService)
{
}

void BondRiskListener::ProcessAdd(Position<Bond> &data)
{ // not defined for this service
}

void BondRiskListener::ProcessRemove(Position<Bond> &data)
{ // not defined for this service
}

void BondRiskListener::ProcessUpdate(Position<Bond> &data)
{
	bondRiskService->AddPosition(data);
}

#endif // !BondRiskSoa_hpp
