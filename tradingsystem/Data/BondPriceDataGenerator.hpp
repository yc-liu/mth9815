// BondPriceDataGenerator.hpp
// 
// Author: Yuchen Liu
// 
// Simulate the trade data with some instructions

#ifndef BondPriceDataGenerator_hpp
#define BondPriceDataGenerator_hpp

#include "productservice.hpp"
#include "products.hpp"
#include "utilityfunction.hpp"
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

// generate the price data and write it to the file specified by path
void bond_price_generator(std::string path, BondProductService* bondProductService, std::string ticker)
{
	std::fstream file(path, std::ios::out | std::ios::trunc); // open the file
	std::vector<Bond> bondVec = bondProductService->GetBonds(ticker);

	if (file.is_open())
	{
		std::cout << "Price: Simulating the price data...\n";
		// header
		file << "BondIDType,BondID,Price,Spread\n";

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
			// spread (alternate between 1/128 and 1/64 for each product)
			double spread = ((i / n) % 2 == 0) ? 1.0/64 : 1.0/128;
			std::string spreadStr = PricetoString(spread);

			// make the output
			file << Idtype << "," << bond.GetProductId() << "," 
				<< priceStr << "," << spreadStr << "\n";


		}
		std::cout << "Price: Simulation finished!\n";
	}
	else
	{
		std::cout << "Oh no! Cannot open the file! Maybe the path is not right?\n";
	}

}


#endif // !BondPriceDataGenerator_hpp
