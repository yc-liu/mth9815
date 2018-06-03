// BondInquirySoa.hpp
// 
// Author: Yuchen Liu
// 
// Define bond inquiry architecture, including 
// bond inquiry service for the determination of the order to be executed, 
// bond inquiry connector for the inquiry interaction with the client, and
// bond inquiry service listener for the data processing

#ifndef BondInquirySoa_hpp
#define BondInquirySoa_hpp

#include "inquiryservice.hpp"
#include "products.hpp"
#include "productservice.hpp"
#include "soa.hpp"
#include "boost/date_time/gregorian/gregorian.hpp" // date operation
#include "boost/algorithm/string.hpp" // string algorithm
#include "utilityfunction.hpp"
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>

// Bond inquiry service
class BondInquiryService : public InquiryService<Bond>
{
private:
	std::vector<ServiceListener<Inquiry<Bond>>*> listeners;
	Connector<Inquiry<Bond>>* bondInquiryConnector;
	std::unordered_map<string, Inquiry<Bond>> inquiryMap; // key on inquiry identifier

public:
	BondInquiryService() {}; // empty ctor

	// Set the inner connector (specific for publish and subscribe connectors)
	void SetConnector(Connector<Inquiry<Bond>>* _bondInquiryConnector);

	// Get data on our service given a key
	virtual Inquiry<Bond> & GetData(string key);

	// The callback that a Connector should invoke for any new or updated data
	virtual void OnMessage(Inquiry<Bond> &data);

	// Add a listener to the Service for callbacks on add, remove, and update events
	// for data to the Service.
	virtual void AddListener(ServiceListener<Inquiry<Bond>> *listener);

	// Get all listeners on the Service.
	virtual const vector< ServiceListener<Inquiry<Bond>>* >& GetListeners() const;

	// Send a quote back to the client
	virtual void SendQuote(const string &inquiryId, double price);

	// Reject an inquiry from the client
	virtual void RejectInquiry(const string &inquiryId);

};


// corresponding subscribe and publish connector
class BondInquiryConnector : public Connector<Inquiry<Bond>>
{
protected:
	BondInquiryService* bondInquiryService;

public:
	BondInquiryConnector(string path, BondInquiryService* _bondInquiryService, 
		BondProductService* _bondProductService); // ctor

	// Publish data to the Connector
	virtual void Publish(Inquiry <Bond> &data);

};

// corresponding service listener
class BondInquiryListener : public ServiceListener<Inquiry<Bond>>
{
protected:
	BondInquiryService* bondInquiryService;

public:
	BondInquiryListener(BondInquiryService* _bondInquiryService); // Ctor

	// Listener callback to process an add event to the Service
	virtual void ProcessAdd(Inquiry<Bond> &data);

	// Listener callback to process a remove event to the Service
	virtual void ProcessRemove(Inquiry<Bond> &data);

	// Listener callback to process an update event to the Service
	virtual void ProcessUpdate(Inquiry<Bond> &data);
};

void BondInquiryService::SetConnector(Connector<Inquiry<Bond>>* _bondInquiryConnector)
{
	bondInquiryConnector = _bondInquiryConnector;
}

Inquiry<Bond> & BondInquiryService::GetData(string key)
{
	return inquiryMap[key];
}

void BondInquiryService::OnMessage(Inquiry<Bond> &data)
{
	// update the inquiry map
	string orderId = data.GetInquiryId();
	if (inquiryMap.find(orderId) == inquiryMap.end()) // if not found this one then create one
		inquiryMap.insert(std::make_pair(orderId, data));
	else
		inquiryMap[orderId] = data;

	// call the listeners
	for (auto listener : listeners)
		listener->ProcessUpdate(data);
}

void BondInquiryService::AddListener(ServiceListener<Inquiry<Bond>> *listener)
{
	listeners.push_back(listener);
}

const vector< ServiceListener<Inquiry<Bond>>* >& BondInquiryService::GetListeners() const
{
	return listeners;
}

void BondInquiryService::SendQuote(const string &inquiryId, double price)
{
	// send back the quote to the connector
	Inquiry<Bond> inquiry = inquiryMap[inquiryId]; // retrieve the corresponding inquiry
	Inquiry<Bond> newInquiry(inquiryId, inquiry.GetProduct(), inquiry.GetSide(), inquiry.GetQuantity(),
		price, inquiry.GetState()); // create a new inquiry with the new price
	bondInquiryConnector->Publish(newInquiry);
}

void BondInquiryService::RejectInquiry(const string &inquiryId)
{
	Inquiry<Bond> inquiry = inquiryMap[inquiryId]; // retrieve the corresponding inquiry
	Inquiry<Bond> newInquiry(inquiryId, inquiry.GetProduct(), inquiry.GetSide(), inquiry.GetQuantity(),
		inquiry.GetPrice(), REJECTED); // transition the inquiry to the REJECTED state
	bondInquiryConnector->Publish(newInquiry); // send back the quote to the connector
}

BondInquiryConnector::BondInquiryConnector(string path, BondInquiryService* _bondInquiryService,
	BondProductService* _bondProductService):
	bondInquiryService(_bondInquiryService)
{
	bondInquiryService->SetConnector(this);

	fstream file(path, std::ios::in);
	string line;
	std::stringstream ss;
	char separator = ','; // comma seperator

	if (file.is_open())
	{
		std::cout << "Inquiry: Begin to read data...\n";
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

			// make the corresponding inquiry object
			// inquiry Id
			string inquiryId = tempData[0];
			// bond Id
			BondIdType type = (boost::algorithm::to_upper_copy(tempData[1]) == "CUSIP") ? CUSIP : ISIN;
			string bondId = tempData[2];
			// the bond product
			Bond bond = _bondProductService->GetData(bondId); // bond id, bond id type, ticker, coupon, maturity
			// inquiry side
			Side side = (boost::algorithm::to_upper_copy(tempData[3]) == "BUY") ? BUY : SELL;
			// inquiry quantity
			long quantity = StringtoType<long>(ss, tempData[4]);
			// inquiry price
			double price = StringtoPrice<double>(ss, tempData[5]);
			// inquiry state
			boost::algorithm::to_upper(tempData[6]);
			InquiryState state = RECEIVED; // default as RECEIVED
			if (tempData[6] == "QUOTED") state = QUOTED;
			else if (tempData[6] == "DONE") state = DONE;
			else if (tempData[6] == "REJECTED") state = REJECTED;
			else if (tempData[6] == "CUSTOMER_REJECTED") state = CUSTOMER_REJECTED;

			// inquiry object
			Inquiry<Bond> inquiry(inquiryId, bond, side, quantity, price, state);

			bondInquiryService->OnMessage(inquiry);
		}
		std::cout << "Inquiry: finished!\n";
	}
	else
	{
		std::cout << "Oh no! Cannot open the file! Maybe the path is not right?\n";
	}
	
}

// Publish data to the Connector
void BondInquiryConnector::Publish(Inquiry <Bond> &data)
{
	if (data.GetState() == REJECTED) // if the inquiry rejected by the service
		bondInquiryService->OnMessage(data);
	else // if not rejected by the service
	{
		// transition the inquiry to the QUOTED state
		Inquiry<Bond> quotedInquiry(data.GetInquiryId(), data.GetProduct(), data.GetSide(), data.GetQuantity(),
			data.GetPrice(), QUOTED);

		// send it back to the service
		bondInquiryService->OnMessage(quotedInquiry);

		// transition the inquiry to the DONE state (or the customer can reject it)
		Inquiry<Bond> doneInquiry(data.GetInquiryId(), data.GetProduct(), data.GetSide(), data.GetQuantity(),
			data.GetPrice(), DONE);

		// send it back to the service
		bondInquiryService->OnMessage(doneInquiry);
	}

}

BondInquiryListener::BondInquiryListener(BondInquiryService* _bondInquiryService) :
	bondInquiryService(_bondInquiryService)
{
}

void BondInquiryListener::ProcessAdd(Inquiry<Bond> &data)
{ // not defined for this service
}

void BondInquiryListener::ProcessRemove(Inquiry<Bond> &data)
{ // not defined for this service
}

void BondInquiryListener::ProcessUpdate(Inquiry<Bond> &data)
{
	if (data.GetState() == RECEIVED) // if inquiry received, send a quote of 100.0 (or you can reject it)
		bondInquiryService->SendQuote(data.GetInquiryId(), 100.0); 
}

#endif // !BondInquirySoa_hpp
