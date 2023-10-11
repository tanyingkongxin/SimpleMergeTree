#pragma once

#include <limits>
#include <cstdint>

#include <hpx/hpx.hpp>

enum ValueType{
    FLOAT = 0,
    CHAR = 1
};

template<typename T>
class Value {
public:
    T value;
    uint64_t vertex;

    Value(){
    }

    Value(T value, uint64_t vertex)
        : value(value)
        , vertex(vertex)
    {
    }

    friend std::ostream & operator<< (std::ostream &out, Value const &t){
        out << "(" << t.value << "," << t.vertex << ")";
        return out;
    }

private:
    friend class hpx::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & value & vertex;
    }

public:
    inline bool operator<(const Value& other) const
    {
        if (this->value == other.value)
            return this->vertex < other.vertex;
        return this->value < other.value;
    }

    inline bool operator>(const Value& other) const
    {
        if (this->value == other.value)
            return this->vertex > other.vertex;
        return this->value > other.value;
    }

    inline bool operator<=(const Value& other) const
    {
        return !(this->operator>(other));
    }

    inline bool operator>=(const Value& other) const
    {
        return !(this->operator<(other));
    }

    inline bool operator==(const Value& other) const
    {
        return ((this->value == other.value) && (this->vertex == other.vertex));
    }

    inline bool operator!=(const Value& other) const
    {
        return !(this->operator==(other));
    }
};

namespace std {
template <typename T>
class numeric_limits<Value<T>> {
public:
    static Value<T> min()
    {
        return Value(std::numeric_limits<T>::min(), std::numeric_limits<uint64_t>::min());
    }

    static Value<T> max()
    {
        return Value(std::numeric_limits<T>::max(), std::numeric_limits<uint64_t>::max());
    }
};
}