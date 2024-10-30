//
// Created by bordeax on 10/30/24.
//

#include "InstructionsHandlers.h"

uint8_t fetchingFunc(INSTRUCTION_MASKS dataMask,uint8_t buffer8Bit)
{
    uint8_t fetchedData = buffer8Bit & static_cast<uint8_t>(dataMask);
    fetchedData >>= shiftRight[dataMask];
    return fetchedData;
};


std::array<uint16_t,5> fetchingRegMemData(std::array<uint8_t, 6>& buffer)
{
    // Fetching W
    uint16_t wValue = fetchingFunc(INSTRUCTION_MASKS::W_6BITES,buffer[0]);
    
    // Fetching D
    uint16_t dValue = fetchingFunc(INSTRUCTION_MASKS::D, buffer[0]);
    
    //Fetching Mode
    uint16_t mod = fetchingFunc(INSTRUCTION_MASKS::MOD, buffer[1]);
    
    // Fetching Reg
    uint16_t regValue = fetchingFunc(INSTRUCTION_MASKS::REG_6BITES_OP_CODE, buffer[1]);
    
    // Fetching REG_MEM
    uint16_t regMemValue = fetchingFunc(INSTRUCTION_MASKS::REG_MEM, buffer[1]);
    return {wValue,dValue,mod,regValue,regMemValue};
}

std::string handleRegMemModValues(std::array<uint16_t,5> prefetchedValues, const std::string& instructionString, std::ifstream& bytesStream, std::array<uint8_t, 6>& buffer)
{
    auto [wValue,dValue,mod,regValue,regMemValue] = prefetchedValues;
    std::stringstream ss;
    
    if(mod == 0b11)
    {
        auto regString = (wValue) ? regNamesExtendedToStr[regValue] : regNamesToStr[regValue];
        
        auto regMemValueString = (wValue) ? regNamesExtendedToStr[regMemValue] : regNamesToStr[regMemValue];
        
        ss << instructionString << " " << regMemValueString  << "," << " " << regString << std::endl;
        return ss.str();
    }
    else if (mod == 0b01)
    {
        auto regMemValueString = (wValue) ? regNamesExtendedToStr[regValue] : regNamesToStr[regValue];
        auto calculationAddress = regNamesPlus[regMemValue];
        
        bytesStream.read(reinterpret_cast<char*>(&buffer[2]), sizeof(buffer[2]));
        uint8_t data = fetchingFunc(INSTRUCTION_MASKS::DATA, buffer[2]);
        
        std::stringstream displacementValue;
        if(data != 0u)
        {
            displacementValue << "[" << calculationAddress << " + " << std::to_string(data) << "]";
        }
        else
        {
            displacementValue << "[" << calculationAddress << "]";
        }
        
        ss << instructionString << " "
           << ((dValue == 1u) ? regMemValueString : displacementValue.str())  << ", "
           << ((dValue == 1u) ? displacementValue.str() : regMemValueString)  << std::endl;;
        
        return ss.str();
    }
    else if(mod == 0b00)
    {
        
        auto regMemValueString = (wValue) ? regNamesExtendedToStr[regValue] : regNamesToStr[regValue];
        auto calculationAddress = regNamesPlus[regMemValue];
        std::string displacementValue = "[" + calculationAddress + "]";
        ss << instructionString << " "
           << ((dValue == 1u) ? regMemValueString: displacementValue) << ", "
           << ((dValue == 1u) ? displacementValue: regMemValueString) << std::endl;
        
        return ss.str();
    }
    else if(mod == 0b10)
    {
        auto regMemValueString = (wValue) ? regNamesExtendedToStr[regMemValue] : regNamesToStr[regMemValue];
        auto calculationAddress = regNamesPlus[regMemValue];
        
        bytesStream.read(reinterpret_cast<char*>(&buffer[2]), sizeof(buffer[2]));
        bytesStream.read(reinterpret_cast<char*>(&buffer[3]), sizeof(buffer[3]));
        uint16_t data = fetchingFunc(INSTRUCTION_MASKS::DATA, buffer[2]);
        uint16_t dataHigh = fetchingFunc(INSTRUCTION_MASKS::DATA, buffer[3]);
        dataHigh <<= 8;
        data ^= dataHigh;
        
        
        std::stringstream displacementValue;
        if(data != 0u)
        {
            displacementValue << "[" << calculationAddress << " + " << std::to_string(data) << "]";
        }
        else
        {
            displacementValue << "[" << calculationAddress << "]";
        }
        
        ss << instructionString << " "
           << ((dValue == 1u)? regMemValueString : displacementValue.str())  << ","
           << ((dValue == 1u) ? displacementValue.str() : regMemValueString) << std::endl;;
        
        
        return ss.str();
    }
    
    return "";
}

std::string handleMovRegMemInstruction(std::array<uint8_t, 6>& buffer, std::ifstream& bytesStream)
{
    bytesStream.read(reinterpret_cast<char*>(&buffer[1]), sizeof(buffer[1]));
    auto fetchedValues = fetchingRegMemData(buffer);
    return handleRegMemModValues(fetchedValues,OpCodeToString(OP_CODE_VALUES::MOV_REG_MEM),bytesStream, buffer);
}

std::string handleImmediateToRegister(std::array<uint8_t, 6>& buffer, std::ifstream& bytesStream, const std::string& instructionType)
{
    uint8_t wValue = fetchingFunc(INSTRUCTION_MASKS::W_4BITES,buffer[0]);
    if(wValue == 1u)
    {
        bytesStream.read(reinterpret_cast<char*>(&buffer[2]), sizeof(buffer[2]));
    }
    uint8_t regValue = fetchingFunc(INSTRUCTION_MASKS::REG_4BITES_OP_CODE, buffer[0]);
    uint16_t  data = fetchingFunc(INSTRUCTION_MASKS::DATA, buffer[1]);
    if(wValue == 1u)
    {
        uint16_t highBitData = fetchingFunc(INSTRUCTION_MASKS::DATA, buffer[2]);
        highBitData <<=8;
        data ^= highBitData;
    }
    
    auto regString = (wValue) ? regNamesExtendedToStr[regValue] : regNamesToStr[regValue];
    auto valueString = std::to_string(data);
    std::stringstream ss;
    ss << instructionType << " " << regString  << "," << " " << valueString << std::endl;
    return ss.str();
}

std::string handleMovImmediateToRegister(std::array<uint8_t, 6>& buffer, std::ifstream& bytesStream)
{
    bytesStream.read(reinterpret_cast<char*>(&buffer[1]), sizeof(buffer[1]));
    return handleImmediateToRegister(buffer,bytesStream, OpCodeToString(OP_CODE_VALUES::MOV_IMMEDIATE));
}

std::string handleAddRegMemInstruction(std::array<uint8_t, 6>& buffer, std::ifstream& bytesStream)
{
    bytesStream.read(reinterpret_cast<char*>(&buffer[1]), sizeof(buffer[1]));
    auto fetchedValues = fetchingRegMemData(buffer);
    return handleRegMemModValues(fetchedValues,OpCodeToString(OP_CODE_VALUES::ADD_REG_MEM),bytesStream, buffer);
}

std::string handleAddImmediateToRegister(std::array<uint8_t, 6>& buffer, std::ifstream& bytesStream)
{
    bytesStream.read(reinterpret_cast<char*>(&buffer[1]), sizeof(buffer[1]));
    return handleImmediateToRegister(buffer,bytesStream, OpCodeToString(OP_CODE_VALUES::ADD_IMMEDIATE));
}