#pragma once

#include <unordered_map>
#include <string>

class Value;

class NameContext {
    std::unordered_map<const Value*, std::string> valueNames;
    int nextId = 0;

public:
    std::string getValueName(const Value* val) {
        auto it = valueNames.find(val);
        if (it != valueNames.end()) return it->second;
        std::string name = "%v" + std::to_string(nextId++);
        valueNames[val] = name;
        return name;
    }
};