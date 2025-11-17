#ifndef ENGINE_H
#define ENGINE_H

#include <iostream>   
#include <vector>     
#include <string>
#include "BST.h"      
#include "Record.h"
//add header files as needed

using namespace std;

// Converts a string to lowercase (used for case-insensitive searches)
static inline string toLower(string s) {
    for (char &c : s) c = (char)tolower((unsigned char)c);
    return s;
}

// ================== Index Engine ==================
// Acts like a small "database engine" that manages records and two BST indexes:
// 1) idIndex: maps student_id → record index (unique key)
// 2) lastIndex: maps lowercase(last_name) → list of record indices (non-unique key)
struct Engine {
    vector<Record> heap;                  // the main data store (simulates a heap file)
    BST<int, int> idIndex;                // index by student ID
    BST<string, vector<int>> lastIndex;   // index by last name (can have duplicates)

    // Inserts a new record and updates both indexes.
    // Returns the record ID (RID) in the heap.
    int insertRecord(const Record &recIn) {
        heap.push_back(recIn);
        int hsize = (int)heap.size()-1;
        idIndex.insert(recIn.id, hsize);

        vector<int>* found = lastIndex.find(toLower(recIn.last));
        if (!found) {
            lastIndex.insert(toLower(recIn.last), {hsize});
        }
        else {
            found->push_back(hsize);
        }

        return recIn.id;
    }

    // Deletes a record logically (marks as deleted and updates indexes)
    // Returns true if deletion succeeded.
    bool deleteById(int id) {
        int* recordindx = idIndex.find(id);
        
        if (!recordindx) {
            return false;
        }
        
        bool &status = heap[*recordindx].deleted;

        if (status == true) {
            return false;
        }

        //delete from heap
        status = true;

        //delete from idIndex
        idIndex.erase(id);

        std::string lastname = heap[*recordindx].last;

        vector<int>* vectorofindx = lastIndex.find(toLower(lastname));

        //delete from lastIndex
        if ((int)vectorofindx->size()==1) {
            lastIndex.erase(lastname);
        }
        else {
            for (int i = 0; i<(int)vectorofindx->size(); i++) {
                if ((*vectorofindx)[i] == *recordindx) {
                    (*vectorofindx)[i] = (*vectorofindx)[vectorofindx->size()-1];
                    vectorofindx->pop_back();
                    break;
                }
            }
            
        }

        return true;
        
    }

    // Finds a record by student ID.
    // Returns a pointer to the record, or nullptr if not found.
    // Outputs the number of comparisons made in the search.
    const Record *findById(int id, int &cmpOut) {
        idIndex.resetMetrics();
        int* key = idIndex.find(id);

        cmpOut=idIndex.comparisons;

        if (!key) {
            return nullptr;
        }

        return &heap[*key];
        }

    // Returns all records with ID in the range [lo, hi].
    // Also reports the number of key comparisons performed.
    vector<const Record *> rangeById(int lo, int hi, int &cmpOut) {
        idIndex.resetMetrics();
        vector<const Record*> pointerrecord;
        idIndex.rangeApply(lo, hi, [&](int k, int v){
            pointerrecord.push_back(&heap[v]);
        });
        cmpOut=idIndex.comparisons;
        return pointerrecord;
    }

    // Returns all records whose last name begins with a given prefix.
    // Case-insensitive using lowercase comparison.





    
    vector<const Record *> prefixByLast(const string &prefix, int &cmpOut) {
        vector<const Record*> output;
        lastIndex.resetMetrics();

        string low = toLower(prefix);
        string high = low + '\x7F';

        lastIndex.rangeApply(low, high, [&](std::string k, vector<int> v){
            
            if (k.compare(0, low.length(), low)==0) {
                //cmpOut++;
                for (int i=0; i<(int)v.size(); i++) {
                    output.push_back(&heap[v[i]]);
                }
            }

        });
        cmpOut = lastIndex.comparisons;
        
        return output;
        
    }
};

#endif
