// utilityfunction.hpp
// 
// Author: Yuchen Liu
// 
// Some useful utility function regarding string manipulation

#ifndef utilityfunction_hpp
#define utilityfunction_hpp

#include <string>
#include <vector>
#include <sstream>
#include "boost/algorithm/string.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"

// convert string into the specified type
template <typename T>
T StringtoType(std::stringstream& ss, std::string str)
{
	T output;
	ss << str;
	ss >> output;
	ss.clear(); // release the buffer
	return output;
}

// convert the price string to the price
// Input format is like 99-xyz where xy is from 00 to 31 and z is from 0 to 7 (z='+' if z=4)
// calculation algorithm is (in this case) 99 + xy/32 + z/256
template <typename T>
T StringtoPrice(std::stringstream& ss, std::string priceStr)
{
	int integer, decimal1, decimal2; // the integer part, xy part and z part

	// split the input string
	std::vector<std::string> stringVec;
	boost::algorithm::split(stringVec, priceStr, boost::algorithm::is_any_of("-"));

	// extract the three parts
	integer = StringtoType<int>(ss, stringVec[0]); // the integer part
	decimal1 = StringtoType<int>(ss, stringVec[1].substr(0, 2));
	if (stringVec[1].substr(2, 1) == "+")
		decimal2 = 4;
	else
		decimal2 = StringtoType<int>(ss, stringVec[1].substr(2, 1));

	// generate the price
	double price = integer + decimal1 / 32.0 + decimal2 / 256.0;

	return price;
}

// convert price to the price string
// Output format is like 99-xyz where xy is from 00 to 31 and z is from 0 to 7 (z='+' if z=4)
template <typename T>
std::string PricetoString(T price)
{
	// the integer part, xy part and z part
	int integer = std::floor(price);
	int	decimal1 = std::floor((price - integer) * 32);
	int decimal2 = (price - integer) * 256 - decimal1 * 8;

	// make the output
	std::string output = std::to_string(integer) + "-";
	if (decimal1 < 10) output += "0";
	output += std::to_string(decimal1);
	if (decimal2 == 4) output += "+";
	else output += std::to_string(decimal2);

	//ss << std::to_string(integer) << "-";
	//ss << std::setfill('0') << std::setw(2) << std::to_string(decimal1);
	//if (decimal2 == 4) ss << "+";
	//else ss << std::to_string(decimal2);

	//std::string output = ss.str();
	//ss.str(""); // release the buffer

	return output;
	
}

// convert boost::gregorian::date to us date string
template <typename Date>
std::string DatetoUsString(Date d)
{
	std::string output;
	if (d.month().as_number() < 10) output += "0";
	output = output + std::to_string(d.month().as_number()) + "/";
	if (d.day() < 10) output += "0";
	output = output + std::to_string(d.day()) + "/" + std::to_string( d.year());
	//ss << std::setfill('0') << std::setw(2) << d.month().as_number() << "/" << d.day() << "/" << d.year();
	//std::string output = ss.str();
	//ss.str(""); // release the buffer

	return output;
}


#endif // !utilityfunction_hpp

