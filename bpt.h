#ifndef BPT_H
#define BPT_H

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <fstream>

// Constants for B+ Tree
const int MAX_KEY_LEN = 64;
const int ORDER = 100; // Adjust based on performance needs

// Key structure
struct Key {
    char str[MAX_KEY_LEN + 1]; // +1 for null terminator
    int value;

    Key();
    Key(const std::string& s, int v);
    bool operator<(const Key& other) const;
    bool operator==(const Key& other) const;
};

// Node structure for B+ Tree
struct Node {
    bool is_leaf;
    int key_count;
    Key keys[ORDER];
    int children[ORDER + 1]; // For internal nodes: child pointers (file offsets)
    int next_leaf; // For leaf nodes: next leaf pointer
    int parent;

    Node();
};

// B+ Tree class
class BPlusTree {
private:
    std::string filename;
    std::fstream file;
    int root_offset;
    int free_list_head;

    // Cache: store all nodes in memory for performance
    std::unordered_map<int, Node> node_cache;
    std::unordered_set<int> dirty_nodes;

    // File operations
    void read_node(int offset, Node& node);
    void write_node(int offset, const Node& node);
    void flush_all();
    int allocate_node();
    void deallocate_node(int offset);

    // Tree operations
    int find_leaf(const std::string& key);
    void insert_into_leaf(int leaf_offset, const Key& key);
    void insert_into_parent(int left_offset, const Key& key, int right_offset);
    void split_leaf(int leaf_offset, int& new_leaf_offset, Key& middle_key);
    void split_internal(int internal_offset, int& new_internal_offset, Key& middle_key);

public:
    BPlusTree(const std::string& fname = "bpt_data.bin");
    ~BPlusTree();

    // Public interface
    void insert(const std::string& key, int value);
    void remove(const std::string& key, int value);
    std::vector<int> find(const std::string& key);

    // For debugging
    void print_tree();
};

#endif // BPT_H