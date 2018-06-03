// BondMarketDataGenerator.hpp
// 
// Author: Yuchen Liu
// 
// Simulate the market data (orderbook) with some instructions

#ifndef BondMarketDataGenerator_hpp
#define BondMarketDataGenerator_hpp

#include "productservice.hpp"
#include "products.hpp"
#include "utilityfunction.hpp"
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

// generate the market data (orderbook) and write it to the file specified by path
void bond_market_data_generator(std::string path, BondProductService* bondProductService, std::string ticker)
{
	std::fstream file(path, std::ios::out | std::ios::trunc); // open the file
	std::vector<Bond> bondVec = bondProductService->GetBonds(ticker);

	if (file.is_open())
	{
		std::cout << "Market data: Simulating the market data...\n";
		// header
		file << "BondIDType,BondID,Price,Spread1,Spread2,Spread3,Spread4,Spread5," 
			<< "Size1,Size2,Size3,Size4,Size5\n";

		int n = bondVec.size(); // # of bonds
		int bondIndex;
		// each product has 1000000 data
		for (long i = 0; i < n * 1000000; i++)
		{
			bondIndex = i % n;
			Bond bond = bondVec[bondIndex];
			// bond id type
			std::string Idtype = (bond.GetBondIdType() == CUSIP) ? "CUSIP" : "ISIN";
			// price (ocsillate from 99 to 101 to 99 with 1/256 as increments/decrements for each product)
			int temp = (i / n) % 1024;
			double price = 99.0 + ((temp < 512) ? temp / 256.0 : (1024 - temp) / 256.0);
			std::string priceStr = PricetoString(price);
			// top spread (alternate between 1/128 and 1/64 for each product)
			int temp2 = (i / n) % 6;
			double spread1 = 1 / 128.0 + ((temp2 < 3) ? temp2 / 128.0 : (6 - temp2) / 128.0);
			std::string spreadStr1 = PricetoString(spread1);
			// following spread (by increments of 1/128)
			double spread2 = spread1 + 1 / 128.0;
			double spread3 = spread1 + 2 / 128.0;
			double spread4 = spread1 + 3 / 128.0;
			double spread5 = spread1 + 4 / 128.0;
			std::string spreadStr2 = PricetoString(spread2);
			std::string spreadStr3 = PricetoString(spread3);
			std::string spreadStr4 = PricetoString(spread4);
			std::string spreadStr5 = PricetoString(spread5);
			// make the output
			file << Idtype << "," << bond.GetProductId()  << ","<< priceStr << "," 
				<< spreadStr1 << "," << spreadStr2 << "," << spreadStr3 << "," << spreadStr4 << "," 
				<< spreadStr5 << "," << std::to_string(10000000) << "," << std::to_string(20000000) << "," 
				<< std::to_string(30000000) << "," << std::to_string(40000000) << "," << std::to_string(50000000) 
				<< "\n";

		}
		std::cout << "Market data: Simulation finished!\n";
	}
	else
	{
		std::cout << "Oh no! Cannot open the file! Maybe the path is not right?\n";
	}

}


#endif // !BondMarketDataGenerator_hpp
