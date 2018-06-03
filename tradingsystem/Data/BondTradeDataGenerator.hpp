// BondTradeDataGenerator.hpp
// 
// Author: Yuchen Liu
// 
// Simulate the trade data with some instructions

#ifndef BondTradeDataGenerator_hpp
#define BondTradeDataGenerator_hpp

#include "productservice.hpp"
#include "products.hpp"
#include "utilityfunction.hpp"
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

// generate the trade data and write it to the file specified by path
void bond_trade_generator(std::string path, BondProductService* bondProductService, std::string ticker)
{
	std::fstream file(path, std::ios::out | std::ios::trunc); // open the file
	std::stringstream ss;
	std::vector<Bond> bondVec = bondProductService->GetBonds(ticker);


	if (file.is_open())
	{
		std::cout << "Trade: Simulating the trade data...\n";
		// header
		file << "TradeID,BondIDType,BondID,Side,Quantity,Price,BookId\n";

		int n = bondVec.size(); // # of bonds
		int bondIndex;
		// each product has 10 data
		for (long i = 0; i < n * 10; i++)
		{
			bondIndex = i % n;
			Bond bond = bondVec[bondIndex];
			// trade Id (e.g. TRS2024T005)
			ss << "TRS" << std::to_string(bond.GetMaturityDate().year()) << bond.GetTicker()
				<< std::setfill('0') << std::setw(3) << std::to_string(i + 1);
			std::string tradeId = ss.str();
			ss.str(""); // release the buffer
			// bond id type
			std::string Idtype = (bond.GetBondIdType() == CUSIP) ? "CUSIP" : "ISIN";
			// side (alternate between buy and sell for each product)
			std::string side = ((i/n) % 2 == 0) ? "BUY" : "SELL";
			// quantity (alternate among 1M, 2M, 3M, 4M and 5M for each product)
			long quantity = 1000000 * ((i / n) % 5 + 1);
			// price (99 for buy, 100 for sell)
			double price = (side == "BUY")? 99.0: 100.0; 
			std::string priceStr = PricetoString(price);
			// book id (alternate between three books for each product)
			std::string bookId = "TRSY1";
			if ((i / n) % 3 == 1) bookId = "TRSY2";
			else if ((i / n) % 3 == 2) bookId = "TRSY3";

			// make the output
			file << tradeId << "," << Idtype << "," << bond.GetProductId() << "," << side << ","
				<< quantity << "," << priceStr << "," << bookId << "\n";

			ss.str(""); // release the buffer
		}
		std::cout << "Trade: Simulation finished!\n";
	}
	else
	{
		std::cout << "Oh no! Cannot open the file! Maybe the path is not right?\n";
	}

}


#endif // !BondTradeDataGenerator_hpp
