// GUIService.hpp
// 
// Author: Yuchen Liu
// 
// Define the data types and Service for GUI

#ifndef GUIService_hpp
#define GUIService_hpp

#include "pricingservice.hpp"
#include "soa.hpp"

// GUI Service managing the graphical user interface
// Keyed on product identifier.
// Type T is the product type.
template <typename T>
class GUIService : public Service<string, Price<T>>
{

public:
	// Add a price to the service
	virtual void AddPrice(const Price<T> &price) = 0;

};


#endif // !GUIService_hpp
