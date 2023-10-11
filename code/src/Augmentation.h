#pragma once

#include "DataManager.h"
#include "RawManager.h"

#include <cstdint>
#include <sys/types.h>
#include <vector>
#include <algorithm>

struct SkipNode {
    uint64_t key;
    std::vector<SkipNode*> forward;

    SkipNode(uint64_t k, int level): key(k){
        forward.resize(level, nullptr);
    }
};

class SkipListSet {
public:
    //SkipListSet(): head(nullptr), back(nullptr){}

    SkipListSet(float prob, int maxLevel, DataManager* data)
        : data(data){
        this->probability = prob;
        this->maxLevel = maxLevel;
        head = new SkipNode(0, maxLevel); // key 任意取值都可以
        back = new SkipNode(INVALID_VERTEX, maxLevel);
        for (int i = 0; (i < head->forward.size()); i++) {
            head->forward[i] = back;
            back->forward[i] = head;
        }
    }
    
    void clear(){
        for (int i = 0; (i < head->forward.size()); i++) {
            head->forward[i] = back;
            back->forward[i] = head;
        }
    }

    void reclaim(){
        delete head;
        delete back;
    }

    class Iterator {
        friend class SkipListSet;

    private:
        Iterator(SkipNode* node): node(node){}

    public:
        Iterator(): node(nullptr){}

        Iterator& operator++()
        {
            node = node->forward[0];
            return *this;
        }

        SkipNode* operator*(){
            return node;
        }

        bool operator!=(const Iterator& other)
        {
            return !(this->operator==(other));
        }

        bool operator==(const Iterator& other)
        {
            return node == other.node;
        }

    protected:
        SkipNode* node;
    };

    Iterator begin() { return Iterator(head->forward[0]); }
    Iterator end() { return Iterator(back); }

    /* 返回 SkipListSet 加上没有析构函数，会不会出现问题 */
    SkipListSet split(uint64_t splitKey){
        SkipNode* x = head;
        SkipListSet result = SkipListSet(0.5f, 24, this->data);

        for (int i = maxLevel - 1; i >= 0; i--) {
            while ( this->data->less(x->forward[i]->key, splitKey) ) {
                x = x->forward[i];
            }
            if (x->forward[i] != back) {
                result.injectRange(x->forward[i], back->forward[i], i);
                x->forward[i] = back;
                back->forward[i] = x;
            }
        }

        return result;
    }

    void injectLast(SkipNode* node, int level){
        for (int i = level; i >= 0; i--) {
            back->forward[i]->forward[i] = node;
            node->forward[i] = back;
            back->forward[i] = node;
        }
    }

    void injectRange(SkipNode* front, SkipNode* last, int level){
        head->forward[level] = front;
        last->forward[level] = back;
        back->forward[level] = last;
    }

    void insert(uint64_t searchKey){
        SkipNode* x = nullptr;
        x = find(searchKey);
        if (x) {
            return;
        }

        // vector of pointers that needs to be updated to account for the new node
        std::vector<SkipNode*> update(head->forward);
        int currentMaximum = nodeLevel(head->forward);
        x = head;

        // search the list
        for (int i = currentMaximum - 1; i >= 0; i--) {
            while( this->data->less(x->forward[i]->key, searchKey) ){
                x = x->forward[i];
            }
            update[i] = x;
        }
        x = x->forward[0];

        // create new node
        int newNodeLevel = 1;
        if (x->key != searchKey) {

            newNodeLevel = randomLevel();
            int currentLevel = nodeLevel(update);

            if (newNodeLevel > currentLevel) {
                for (int i = newNodeLevel-1; i >= currentLevel-1; i--) {
                    update[i] = head;
                }
            }

            x = makeNode(searchKey, newNodeLevel);
        }

        // connect pointers of predecessors and new node to successors
        for (int i = 0; i < newNodeLevel; i++) {

            x->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = x;
            if (x->forward[i] == back)
                back->forward[i] = x;
        }
    }

    DataManager* getDataManager(){
        return this->data;
    }

private:
    int randomLevel(){
        int v = 1;

        while ((((double) std::rand() / RAND_MAX)) < probability && v < maxLevel) {
            v += 1;
        }
        return v;
    }

    /*
     * input: head->forward
     * return: the level number of skip list 
     */
    int nodeLevel(const std::vector<SkipNode*>& v){
        int currentLevel = 0;
        // last element's key is the largest
        uint64_t nilKey = INVALID_VERTEX;

        for (size_t i = 0; i < v.size(); i++) {
            currentLevel++;
            if (v[i]->key == nilKey) {
                break;
            }
        }
        return currentLevel;
    }

    SkipNode* makeNode(uint64_t key, int level){
        return new SkipNode(key, level);
    }

    SkipNode* find(uint64_t searchKey){
        SkipNode* x = head;
        int currentMaximum = nodeLevel(head->forward);

        for (int i = currentMaximum - 1; i >= 0; i--) {
            while (x->forward[i]->key < searchKey) {
                x = x->forward[i];
            }
        }
        x = x->forward[0];

        if (x->key == searchKey) {
            return x;
        } else {
            return nullptr;
        }
    }

    SkipNode* head;
    SkipNode* back; // 这个 node 比较特殊，是往回指的
    
    float probability;
    int maxLevel;

    DataManager* data;
};

/* 一些 while 循环的比较，可以用 x->forward[i] == back 来判断是否到末尾了 */
/* 用 INVALID_INDEX 来表示无穷大，less() 中需要保证 INVALID_INDEX 返回的值足够大 */

/* skipListSet 的析构函数没有 delete head 和 back 两个节点 */

/* 在 inherit 时，形参 heritage 里面所有的 skipListSet 的 head 和 back 节点会被回收；
 * 那么谁来负责回收当前的 Augmentation 呢？
 */

class Augmentation {
public:

    Augmentation(DataManager* data) : vertices(0.5f, 24, data)
    {}

    void sweep(uint64_t v){
        vertices.insert(v);
    }

    void clear(){
        vertices.clear();
    }

    /*
     * param heritage: 执行结束后 vector 中的元素会被清空
     */
    void inherit(std::vector<Augmentation>& heritage){
        if (heritage.size() == 0)
            return;
        if (heritage.size() == 1) {
            vertices = heritage.at(0).vertices;
            return;
        }
        DataManager* data = heritage[0].vertices.getDataManager();

        std::vector<SkipListSet::Iterator> iters;
        for (Augmentation& current : heritage) {
            iters.push_back(current.vertices.begin());
        }
        vertices = SkipListSet(0.5f, 24, data); // 新的 SkipListSet 
        while (true) {
            int smallest = 0;
            for (int i = 1; i < iters.size(); i++) {
                if ( data->less( (*iters[i])->key,  (*iters[smallest])->key)){ // TODO
                    smallest = i;
                }
            }
            if ((*iters[smallest])->key == INVALID_VERTEX )
                break;
            
            SkipNode* next = *iters[smallest];
            ++iters[smallest];
            vertices.injectLast(next, next->forward.size() - 1);
        }
        for (Augmentation& current : heritage) {
            current.vertices.reclaim();
        }
        heritage.clear(); // 会调用 heritage 每个 Augmentation 的析构函数
    }

    // inherit from child to parent
    Augmentation heritage(uint64_t saddle){
        Augmentation result(this->vertices.getDataManager());
        result.vertices = vertices.split(saddle);
        return result;
    }

    SkipListSet vertices;
};