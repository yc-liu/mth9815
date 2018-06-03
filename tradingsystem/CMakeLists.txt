cmake_minimum_required(VERSION 3.9)
project(tradingsystem)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        BondService/HistoricalDataSoa/BondExecutionHistoricalDataSoa.hpp
        BondService/HistoricalDataSoa/BondInquiryHistoricalDataSoa.hpp
        BondService/HistoricalDataSoa/BondPositionHistoricalDataSoa.hpp
        BondService/HistoricalDataSoa/BondRiskHistoricalDataSoa.hpp
        BondService/HistoricalDataSoa/BondStreamingHistoricalDataSoa.hpp
        BondService/BondAlgoExecutionSoa.hpp
        BondService/BondAlgoStreamingSoa.hpp
        BondService/BondExecutionSoa.hpp
        BondService/BondGUIService.hpp
        BondService/BondInquirySoa.hpp
        BondService/BondMarketDataSoa.hpp
        BondService/BondPositionSoa.hpp
        BondService/BondPricingSoa.hpp
        BondService/BondRiskSoa.hpp
        BondService/BondStreamingSoa.hpp
        BondService/BondTradeBookingSoa.hpp
        Data/BondInquiryDataGenerator.hpp
        Data/BondMarketDataGenerator.hpp
        Data/BondPriceDataGenerator.hpp
        Data/BondTradeDataGenerator.hpp
        executionservice.hpp
        GUIService.hpp
        historicaldataservice.hpp
        inquiryservice.hpp
        main.cpp
        marketdataservice.hpp
        positionservice.hpp
        pricingservice.hpp
        products.hpp
        productservice.hpp
        riskservice.hpp
        soa.hpp
        StopWatch.hpp
        streamingservice.hpp
        tradebookingservice.hpp
        utilityfunction.hpp)

add_executable(tradingsystem ${SOURCE_FILES})

include_directories(/Users/wentingyang/yangwt/Baruch_MFE_YQSL/tmp/tradingsystem)
