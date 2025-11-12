#ifndef ENGINE_H
#define ENGINE_H

#include <iostream>   // for input/output
#include <iomanip>    // for setprecision (formatting numeric output)
#include <vector>     // for dynamic arrays (used for the heap and RID lists)
#include <algorithm>
#include "BST.h"      // our Binary Search Tree class definition
#include "Record.h"

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
        Record rec = recIn;
        int rid = (int)heap.size();   // RID = index position in heap
        heap.push_back(rec);

        // Update ID index (unique key)
        idIndex.insert(rec.id, rid);

        // Update last-name index (non-unique, lowercase)
        string key = toLower(rec.last);
        auto vecPtr = lastIndex.find(key);
        if (!vecPtr)
            return;
            //lastIndex.insert(key, vector<int>{rid});
        else
            vecPtr->push_back(rid);

        return rid;
    }

    // Deletes a record logically (marks as deleted and updates indexes)
    // Returns true if deletion succeeded.
    bool deleteById(int id) {
        int *ridPtr = idIndex.find(id);
        if (!ridPtr) return false;            // not found in index
        int rid = *ridPtr;
        if (rid < 0 || rid >= (int)heap.size()) return false;
        if (heap[rid].deleted) return false;  // already deleted

        // Remove RID from last-name index
        string key = toLower(heap[rid].last);
        auto listPtr = lastIndex.find(key);
        if (listPtr) {
            auto &vec = *listPtr;
            vec.erase(std::remove(vec.begin(), vec.end(), rid), vec.end());
            if (vec.empty()) lastIndex.erase(key);
        }

        // Remove from ID index
        idIndex.erase(id);

        // Mark record as deleted (soft delete)
        heap[rid].deleted = true;
        return true;
    }

    // Finds a record by student ID.
    // Returns a pointer to the record, or nullptr if not found.
    // Outputs the number of comparisons made in the search.
    const Record *findById(int id, int &cmpOut) {
        idIndex.resetMetrics();              // reset comparison counter
        int *rid = idIndex.find(id);         // search in ID index
        cmpOut = idIndex.comparisons;        // record how many comparisons occurred
        if (!rid) return nullptr;
        if (*rid < 0 || *rid >= (int)heap.size()) return nullptr;
        const Record &rec = heap[*rid];
        return rec.deleted ? nullptr : &rec; // return only if not deleted
    }

    // Returns all records with ID in the range [lo, hi].
    // Also reports the number of key comparisons performed.
    vector<const Record *> rangeById(int lo, int hi, int &cmpOut) {
        idIndex.resetMetrics();
        vector<const Record *> out;
        idIndex.rangeApply(lo, hi, [&](const int &k, int &rid) {
            if (rid >= 0 && rid < (int)heap.size() && !heap[rid].deleted)
                out.push_back(&heap[rid]);
        });
        cmpOut = idIndex.comparisons;
        return out;
    }

    // Returns all records whose last name begins with a given prefix.
    // Case-insensitive using lowercase comparison.
    vector<const Record *> prefixByLast(const string &prefix, int &cmpOut) {
        lastIndex.resetMetrics();
        vector<const Record *> out;
        string lo = toLower(prefix);        // prefix lower bound
        string hi = lo + '\xFF';            // upper bound (just beyond all matches)
        lastIndex.rangeApply(lo, hi, [&](const string &key, vector<int> &rids) {
            for (int rid : rids) {
                if (rid >= 0 && rid < (int)heap.size() && !heap[rid].deleted)
                    out.push_back(&heap[rid]);
            }
        });
        cmpOut = lastIndex.comparisons;
        return out;
    }
};

#endif
