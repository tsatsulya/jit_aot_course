#pragma once
#include <unordered_map>
#include <string>
#include "instruction.h"

class NameContext {
    std::unordered_map<const Value*, std::string> valueNames;
    int nextId = 0;

public:
    std::string getValueName(const Value* val);
};