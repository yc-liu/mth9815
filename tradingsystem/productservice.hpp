/**
 * productservice.hpp defines Bond and IRSwap ProductServices
 *
 * Revised by Yuchen Liu on 11/19/2017
 * Add class to model service for future products
 * Add utility methods on the BondProductService and IRSwapProductService to
 * search for all instances of a Bond/IR Swap for a particular attribute
 */

#ifndef productservice_hpp
#define productservice_hpp

#include <iostream>
#include <map>
#include "products.hpp"
#include "soa.hpp"

 /**
 * Bond Product Service to own reference data over a set of bond securities.
 * Key is the productId string, value is a Bond.
 */
class BondProductService : public Service<string, Bond>
{

public:
	// BondProductService ctor
	BondProductService();

	// Return the bond data for a particular bond product identifier
	virtual Bond& GetData(string productId);

	// Add a bond to the service (convenience method)
	void Add(Bond &bond);

	// Get all Bonds with the specified ticker
	vector<Bond> GetBonds(string& _ticker);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(Bond &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<Bond> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<Bond>* >& GetListeners() const;

private:
	map<string, Bond> bondMap; // cache of bond products
	std::vector<ServiceListener<Bond>*> listeners;

};

/**
* Interest Rate Swap Product Service to own reference data over a set of IR Swap products
* Key is the productId string, value is a IRSwap.
*/
class IRSwapProductService : public Service<string, IRSwap>
{
public:
	// IRSwapProductService ctor
	IRSwapProductService();

	// Return the IR Swap data for a particular bond product identifier
	IRSwap& GetData(string productId);

	// Add a bond to the service (convenience method)
	void Add(IRSwap &swap);

	// Get all Swaps with the specified fixed leg day count convention
	vector<IRSwap> GetSwaps(DayCountConvention _fixedLegDayCountConvention);

	// Get all Swaps with the specified fixed leg payment frequency
	vector<IRSwap> GetSwaps(PaymentFrequency _fixedLegPaymentFrequency);

	// Get all Swaps with the specified floating index
	vector<IRSwap> GetSwaps(FloatingIndex _floatingIndex);

	// Get all Swaps with a term in years greater than the specified value
	vector<IRSwap> GetSwapsGreaterThan(int _termYears);

	// Get all Swaps with a term in years less than the specified value
	vector<IRSwap> GetSwapsLessThan(int _termYears);

	// Get all Swaps with the specified swap type
	vector<IRSwap> GetSwaps(SwapType _swapType);

	// Get all Swaps with the specified swap leg type
	vector<IRSwap> GetSwaps(SwapLegType _swapLegType);

private:
	map<string, IRSwap> swapMap; // cache of IR Swap products

};


BondProductService::BondProductService()
{
	bondMap = map<string, Bond>();
}

Bond& BondProductService::GetData(string productId)
{
	return bondMap[productId];
}

void BondProductService::Add(Bond &bond)
{
	bondMap.insert(pair<string, Bond>(bond.GetProductId(), bond));
}

vector<Bond> BondProductService::GetBonds(string& _ticker)
{
	vector<Bond> result;
	for (auto iter = bondMap.begin(); iter != bondMap.end(); iter++)
	{
		if (iter->second.GetTicker() == _ticker)
			result.push_back(iter->second);
	}
	return result;
}

void BondProductService::OnMessage(Bond &data)
{
}

void BondProductService::AddListener(ServiceListener<Bond> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<Bond>* >& BondProductService::GetListeners() const
{
	return listeners;
}

IRSwapProductService::IRSwapProductService()
{
	swapMap = map<string, IRSwap>();
}

IRSwap& IRSwapProductService::GetData(string productId)
{
	return swapMap[productId];
}

void IRSwapProductService::Add(IRSwap &swap)
{
	swapMap.insert(pair<string, IRSwap>(swap.GetProductId(), swap));
}

vector<IRSwap> IRSwapProductService::GetSwaps(DayCountConvention _fixedLegDayCountConvention)
{
	vector<IRSwap> result;
	for (auto iter = swapMap.begin(); iter != swapMap.end(); iter++)
	{
		if (iter->second.GetFixedLegDayCountConvention() == _fixedLegDayCountConvention)
			result.push_back(iter->second);
	}
	return result;
}

vector<IRSwap> IRSwapProductService::GetSwaps(PaymentFrequency _fixedLegPaymentFrequency)
{
	vector<IRSwap> result;
	for (auto iter = swapMap.begin(); iter != swapMap.end(); iter++)
	{
		if (iter->second.GetFixedLegPaymentFrequency() == _fixedLegPaymentFrequency)
			result.push_back(iter->second);
	}
	return result;
}

vector<IRSwap> IRSwapProductService::GetSwaps(FloatingIndex _floatingIndex)
{
	vector<IRSwap> result;
	for (auto iter = swapMap.begin(); iter != swapMap.end(); iter++)
	{
		if (iter->second.GetFloatingIndex() == _floatingIndex)
			result.push_back(iter->second);
	}
	return result;
}

vector<IRSwap> IRSwapProductService::GetSwapsGreaterThan(int _termYears)
{
	vector<IRSwap> result;
	for (auto iter = swapMap.begin(); iter != swapMap.end(); iter++)
	{
		if (iter->second.GetTermYears() >= _termYears) // greater or equal than
			result.push_back(iter->second);
	}
	return result;
}

vector<IRSwap> IRSwapProductService::GetSwapsLessThan(int _termYears)
{
	vector<IRSwap> result;
	for (auto iter = swapMap.begin(); iter != swapMap.end(); iter++)
	{
		if (iter->second.GetTermYears() < _termYears) // strictly less than
			result.push_back(iter->second);
	}
	return result;
}

vector<IRSwap> IRSwapProductService::GetSwaps(SwapType _swapType)
{
	vector<IRSwap> result;
	for (auto iter = swapMap.begin(); iter != swapMap.end(); iter++)
	{
		if (iter->second.GetSwapType() == _swapType)
			result.push_back(iter->second);
	}
	return result;
}

vector<IRSwap> IRSwapProductService::GetSwaps(SwapLegType _swapLegType)
{
	vector<IRSwap> result;
	for (auto iter = swapMap.begin(); iter != swapMap.end(); iter++)
	{
		if (iter->second.GetSwapLegType() == _swapLegType)
			result.push_back(iter->second);
	}
	return result;
}



#endif // !productservice_hpp


