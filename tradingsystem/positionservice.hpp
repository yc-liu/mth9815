/**
 * positionservice.hpp
 * Defines the data types and Service for positions.
 *
 * @author Breman Thuraisingham
 */
#ifndef POSITION_SERVICE_HPP
#define POSITION_SERVICE_HPP

#include <string>
#include <map>
#include <unordered_map>
#include "soa.hpp"
#include "tradebookingservice.hpp"

using namespace std;

/**
 * Position class in a particular book.
 * Type T is the product type.
 */
template<typename T>
class Position
{

public:

  // ctor for a position
  Position(const T &_product);
  Position() {} 

  // Get the product
  const T& GetProduct() const;

  // Get the position quantity
  long long GetPosition(string &book);  

  // Get the aggregate position
  long long GetAggregatePosition(); 

  // Add the positions to a specified book
  void AddNewPosition(string &book, long quantity);

  // whether the position has a specified book
  bool HasBook(string book);

private:
  T product;
  unordered_map<string,long long> positions; 

};

/**
 * Position Service to manage positions across multiple books and secruties.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class PositionService : public Service<string,Position <T> >
{

public:

  // Add a trade to the service
  virtual void AddTrade(const Trade<T> &trade) = 0;

};

template<typename T>
Position<T>::Position(const T &_product) :
  product(_product)
{
}

template<typename T>
const T& Position<T>::GetProduct() const
{
  return product;
}

template<typename T>
long long Position<T>::GetPosition(string &book)
{
  return positions[book];
}

template<typename T>
long long Position<T>::GetAggregatePosition()
{
  
	long long sum = 0;
	for (auto element : positions)
		sum += element.second;
	return sum;
}

template<typename T>
void Position<T>::AddNewPosition(string &book, long quantity)
{
	positions[book] += quantity;
}

template<typename T>
bool Position<T>::HasBook(string book)
{
	return (positions.find(book) != positions.end());
}


#endif
