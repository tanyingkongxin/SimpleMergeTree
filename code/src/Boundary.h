#pragma once

#include "DataManager.h"

#include <hpx/hpx.hpp>

#include <set>
#include <vector>

class DataComparator {
public:
    DataComparator(DataManager* d)
        : data(d)
    {
    }

    bool operator()(uint64_t n1, uint64_t n2) const
    {
        return this->data->less(n1, n2);
        // return this->data->getValue(n1) > this->data->getValue(n2);
    }

private:
    DataManager* data;
};

class Boundary {
public:
    Boundary(DataManager* data):vertices(DataComparator(data)){

    }

    typedef std::set<uint64_t, DataComparator> set_type;

    void add(uint64_t t)
    {
        vertices.emplace(t);
    }

    void remove(uint64_t t)
    {
        vertices.erase(t);
    }

    void unite(Boundary& other)
    {
        if (other.vertices.size() > vertices.size())
            vertices.swap(other.vertices);
        for (const uint64_t& c : other.vertices)
            vertices.emplace(c);
        other.vertices.clear();
    }

    /*
     * result = A ∩ B, A = A - result, B = B - result
     */
    std::vector<uint64_t> intersect(Boundary& other)
    {
        std::vector<uint64_t> result;
        set_type* big = (vertices.size() > other.vertices.size()) ? &vertices : &other.vertices;
        set_type* small = (vertices.size() > other.vertices.size()) ? &other.vertices : &vertices;

        for (uint64_t c : *small) {
            if (big->count(c)) {
                result.push_back(c);
            }
        }

        for (uint64_t c : result) {
            big->erase(c);
            small->erase(c);
        }

        return result;
    }

    uint64_t min() const
    {
        return *vertices.begin();
    }

    bool empty() const
    {
        return vertices.empty();
    }
private:
    std::set<uint64_t, DataComparator> vertices;
};