// BondInquiryDataGenerator.hpp
// 
// Author: Yuchen Liu
// 
// Simulate the inquiry data with some instructions

#ifndef BondInquiryDataGenerator_hpp
#define BondInquiryDataGenerator_hpp

#include "productservice.hpp"
#include "products.hpp"
#include "utilityfunction.hpp"
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <random>

// generate the inquiry data and write it to the file specified by path
void bond_inquiry_generator(std::string path, BondProductService* bondProductService, std::string ticker)
{
	std::fstream file(path, std::ios::out | std::ios::trunc); // open the file
	std::stringstream ss;
	std::vector<Bond> bondVec = bondProductService->GetBonds(ticker);

	// random engine to be used
	std::random_device rd;
	std::mt19937 eng(rd());
	std::uniform_real_distribution<> dis(0.0, 1.0);

	if (file.is_open())
	{
		std::cout << "Inquiry: Simulating the inquiry data...\n";
		// header
		file << "InquiryID,BondIDType,BondID,Side,Quantity,Price,State\n";

		int n = bondVec.size(); // # of bonds
		int bondIndex; 
		// each product has 10 data
		for (long i = 0; i < n * 10; i++)
		{
			bondIndex = i % n;
			Bond bond = bondVec[bondIndex];
			// inquiry Id (e.g. INQ2024T005)
			ss << "INQ" << std::to_string(bond.GetMaturityDate().year()) << bond.GetTicker()
				<< std::setfill('0') << std::setw(3) << std::to_string(i + 1);
			std::string inquiryId = ss.str();
			ss.str(""); // release the buffer
			// bond id type
			std::string Idtype = (bond.GetBondIdType() == CUSIP) ? "CUSIP" : "ISIN"; 
			// side (uniform-randomly decide)
			std::string side = (dis(eng) < 0.5) ? "BUY" : "SELL";
			// quantity (uniform-randomly decide)
			long quantity = 1000000 * std::ceil(dis(eng) * 6);
			// price (uniform-randomly decide)
			double price = 99.0 + std::ceil(dis(eng) * 512) / 256.0; // between 99 and 101
			std::string priceStr = PricetoString(price);
			// state(always "receive")
			std::string stateStr = "RECEIVED";

			// make the output
			file << inquiryId << "," << Idtype << "," << bond.GetProductId() << "," << side << ","
				<< quantity << "," << priceStr << "," << stateStr << "\n";

			ss.str(""); // release the buffer
		}
		std::cout << "Inquiry: Simulation finished!\n";
	}
	else
	{
		std::cout << "Oh no! Cannot open the file! Maybe the path is not right?\n";
	}
	
}


#endif // !BondInquiryDataGenerator_hpp
