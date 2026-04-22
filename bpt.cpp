#include "bpt.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <algorithm>

using namespace std;

// Key implementation
Key::Key() {
    str[0] = '\0';
    value = 0;
}

Key::Key(const string& s, int v) {
    strncpy(str, s.c_str(), MAX_KEY_LEN);
    str[MAX_KEY_LEN] = '\0';
    value = v;
}

bool Key::operator<(const Key& other) const {
    int cmp = strcmp(str, other.str);
    if (cmp < 0) return true;
    if (cmp > 0) return false;
    return value < other.value;
}

bool Key::operator==(const Key& other) const {
    return strcmp(str, other.str) == 0 && value == other.value;
}

// Node implementation
Node::Node() : is_leaf(false), key_count(0), next_leaf(-1), parent(-1) {
    for (int i = 0; i < ORDER + 1; i++) {
        children[i] = -1;
    }
}

// BPlusTree implementation
BPlusTree::BPlusTree(const string& fname) : filename(fname), root_offset(-1), free_list_head(-1) {
    // Try to open existing file
    fstream file(filename, ios::in | ios::binary);
    if (file) {
        // Read header: root_offset, free_list_head
        file.read(reinterpret_cast<char*>(&root_offset), sizeof(root_offset));
        file.read(reinterpret_cast<char*>(&free_list_head), sizeof(free_list_head));
        file.close();
    } else {
        // Create new file
        root_offset = -1;
        free_list_head = -1;

        // Write header
        fstream new_file(filename, ios::out | ios::binary);
        new_file.write(reinterpret_cast<const char*>(&root_offset), sizeof(root_offset));
        new_file.write(reinterpret_cast<const char*>(&free_list_head), sizeof(free_list_head));
        new_file.close();
    }
}

BPlusTree::~BPlusTree() {
    // Update header
    fstream file(filename, ios::in | ios::out | ios::binary);
    if (file) {
        file.write(reinterpret_cast<const char*>(&root_offset), sizeof(root_offset));
        file.write(reinterpret_cast<const char*>(&free_list_head), sizeof(free_list_head));
    }
}

void BPlusTree::read_node(int offset, Node& node) {
    fstream file(filename, ios::in | ios::binary);
    if (!file) return;

    file.seekg(sizeof(root_offset) + sizeof(free_list_head) + offset * sizeof(Node));
    file.read(reinterpret_cast<char*>(&node), sizeof(Node));
}

void BPlusTree::write_node(int offset, const Node& node) {
    fstream file(filename, ios::in | ios::out | ios::binary);
    if (!file) return;

    file.seekp(sizeof(root_offset) + sizeof(free_list_head) + offset * sizeof(Node));
    file.write(reinterpret_cast<const char*>(&node), sizeof(Node));
}

int BPlusTree::allocate_node() {
    if (free_list_head != -1) {
        // Reuse from free list
        int offset = free_list_head;
        Node free_node;
        read_node(offset, free_node);
        free_list_head = free_node.children[0]; // Use first child slot to store next free node
        return offset;
    } else {
        // Append new node at end
        fstream file(filename, ios::in | ios::out | ios::binary | ios::ate);
        if (!file) return -1;

        // Get current file size
        file.seekg(0, ios::end);
        long file_size = file.tellg();

        // Calculate offset (0-based index of nodes)
        int offset = (file_size - (sizeof(root_offset) + sizeof(free_list_head))) / sizeof(Node);

        // Write empty node
        Node new_node;
        file.write(reinterpret_cast<const char*>(&new_node), sizeof(Node));

        return offset;
    }
}

void BPlusTree::deallocate_node(int offset) {
    Node node;
    read_node(offset, node);

    // Add to free list
    node.children[0] = free_list_head;
    free_list_head = offset;

    write_node(offset, node);
}

int BPlusTree::find_leaf(const string& key_str) {
    if (root_offset == -1) return -1;

    int current = root_offset;
    Node node;

    while (true) {
        read_node(current, node);

        if (node.is_leaf) {
            return current;
        }

        // Find child to follow
        int i = 0;
        while (i < node.key_count && strcmp(key_str.c_str(), node.keys[i].str) >= 0) {
            i++;
        }

        current = node.children[i];
    }
}

void BPlusTree::insert(const string& key_str, int value) {
    Key new_key(key_str, value);

    if (root_offset == -1) {
        // Create first leaf node
        root_offset = allocate_node();
        Node root;
        root.is_leaf = true;
        root.key_count = 1;
        root.keys[0] = new_key;
        write_node(root_offset, root);
        return;
    }

    // Find leaf node where key should be inserted
    int leaf_offset = find_leaf(key_str);
    Node leaf;
    read_node(leaf_offset, leaf);

    // Check if key already exists
    for (int i = 0; i < leaf.key_count; i++) {
        if (leaf.keys[i] == new_key) {
            // Key-value pair already exists, do nothing
            return;
        }
    }

    // Insert into leaf
    insert_into_leaf(leaf_offset, new_key);
}

void BPlusTree::insert_into_leaf(int leaf_offset, const Key& key) {
    Node leaf;
    read_node(leaf_offset, leaf);

    // Find position to insert
    int pos = 0;
    while (pos < leaf.key_count && leaf.keys[pos] < key) {
        pos++;
    }

    // Shift keys to make room
    for (int i = leaf.key_count; i > pos; i--) {
        leaf.keys[i] = leaf.keys[i - 1];
    }

    // Insert new key
    leaf.keys[pos] = key;
    leaf.key_count++;

    write_node(leaf_offset, leaf);

    // Check if leaf is full
    if (leaf.key_count == ORDER) {
        // Split leaf
        int new_leaf_offset;
        Key middle_key;
        split_leaf(leaf_offset, new_leaf_offset, middle_key);

        if (leaf.parent == -1) {
            // Create new root
            int new_root_offset = allocate_node();
            Node new_root;
            new_root.is_leaf = false;
            new_root.key_count = 1;
            new_root.keys[0] = middle_key;
            new_root.children[0] = leaf_offset;
            new_root.children[1] = new_leaf_offset;

            // Update parent pointers
            leaf.parent = new_root_offset;
            Node new_leaf;
            read_node(new_leaf_offset, new_leaf);
            new_leaf.parent = new_root_offset;
            write_node(new_leaf_offset, new_leaf);

            write_node(new_root_offset, new_root);
            root_offset = new_root_offset;
        } else {
            // Insert into parent
            insert_into_parent(leaf_offset, middle_key, new_leaf_offset);
        }
    }
}

void BPlusTree::split_leaf(int leaf_offset, int& new_leaf_offset, Key& middle_key) {
    Node leaf;
    read_node(leaf_offset, leaf);

    // Create new leaf
    new_leaf_offset = allocate_node();
    Node new_leaf;
    new_leaf.is_leaf = true;
    new_leaf.parent = leaf.parent;
    new_leaf.next_leaf = leaf.next_leaf;

    // Update old leaf's next pointer
    leaf.next_leaf = new_leaf_offset;

    // Split keys
    int split_point = ORDER / 2;

    // Copy second half to new leaf
    for (int i = split_point; i < ORDER; i++) {
        new_leaf.keys[i - split_point] = leaf.keys[i];
        new_leaf.key_count++;
    }

    // Update old leaf count
    leaf.key_count = split_point;

    // Middle key is first key of new leaf
    middle_key = new_leaf.keys[0];

    write_node(leaf_offset, leaf);
    write_node(new_leaf_offset, new_leaf);
}

void BPlusTree::insert_into_parent(int left_offset, const Key& key, int right_offset) {
    Node left, parent;
    read_node(left_offset, left);

    int parent_offset = left.parent;
    read_node(parent_offset, parent);

    // Find position to insert key
    int pos = 0;
    while (pos < parent.key_count && strcmp(key.str, parent.keys[pos].str) >= 0) {
        pos++;
    }

    // Shift keys and children
    for (int i = parent.key_count; i > pos; i--) {
        parent.keys[i] = parent.keys[i - 1];
    }
    for (int i = parent.key_count + 1; i > pos + 1; i--) {
        parent.children[i] = parent.children[i - 1];
    }

    // Insert key and child
    parent.keys[pos] = key;
    parent.children[pos + 1] = right_offset;
    parent.key_count++;

    write_node(parent_offset, parent);

    // Update parent pointer of right child
    Node right;
    read_node(right_offset, right);
    right.parent = parent_offset;
    write_node(right_offset, right);

    // Check if parent is full
    if (parent.key_count == ORDER) {
        int new_parent_offset;
        Key middle_key;
        split_internal(parent_offset, new_parent_offset, middle_key);

        if (parent.parent == -1) {
            // Create new root
            int new_root_offset = allocate_node();
            Node new_root;
            new_root.is_leaf = false;
            new_root.key_count = 1;
            new_root.keys[0] = middle_key;
            new_root.children[0] = parent_offset;
            new_root.children[1] = new_parent_offset;

            // Update parent pointers
            parent.parent = new_root_offset;
            Node new_parent;
            read_node(new_parent_offset, new_parent);
            new_parent.parent = new_root_offset;

            write_node(parent_offset, parent);
            write_node(new_parent_offset, new_parent);
            write_node(new_root_offset, new_root);

            root_offset = new_root_offset;
        } else {
            insert_into_parent(parent_offset, middle_key, new_parent_offset);
        }
    }
}

void BPlusTree::split_internal(int internal_offset, int& new_internal_offset, Key& middle_key) {
    Node internal;
    read_node(internal_offset, internal);

    // Create new internal node
    new_internal_offset = allocate_node();
    Node new_internal;
    new_internal.is_leaf = false;
    new_internal.parent = internal.parent;

    // Split point
    int split_point = ORDER / 2;
    middle_key = internal.keys[split_point];

    // Copy second half to new internal node
    for (int i = split_point + 1; i < ORDER; i++) {
        new_internal.keys[i - split_point - 1] = internal.keys[i];
        new_internal.children[i - split_point - 1] = internal.children[i];
        new_internal.key_count++;
    }
    new_internal.children[new_internal.key_count] = internal.children[ORDER];

    // Update old internal node
    internal.key_count = split_point;

    // Update parent pointers of children moved to new internal node
    for (int i = 0; i <= new_internal.key_count; i++) {
        if (new_internal.children[i] != -1) {
            Node child;
            read_node(new_internal.children[i], child);
            child.parent = new_internal_offset;
            write_node(new_internal.children[i], child);
        }
    }

    write_node(internal_offset, internal);
    write_node(new_internal_offset, new_internal);
}

void BPlusTree::remove(const string& key_str, int value) {
    Key key_to_remove(key_str, value);

    if (root_offset == -1) return;

    // Find leaf node containing the key
    int leaf_offset = find_leaf(key_str);
    Node leaf;
    read_node(leaf_offset, leaf);

    // Find key position
    int pos = -1;
    for (int i = 0; i < leaf.key_count; i++) {
        if (leaf.keys[i] == key_to_remove) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        // Key not found, do nothing as per problem statement
        return;
    }

    // Remove key from leaf
    for (int i = pos; i < leaf.key_count - 1; i++) {
        leaf.keys[i] = leaf.keys[i + 1];
    }
    leaf.key_count--;

    write_node(leaf_offset, leaf);

    // Note: For simplicity, we're not implementing rebalancing/merging
    // This might cause performance issues but should work for basic functionality
}

vector<int> BPlusTree::find(const string& key_str) {
    vector<int> result;

    if (root_offset == -1) return result;

    // Find first leaf node containing the key
    int leaf_offset = find_leaf(key_str);

    while (leaf_offset != -1) {
        Node leaf;
        read_node(leaf_offset, leaf);

        for (int i = 0; i < leaf.key_count; i++) {
            if (strcmp(leaf.keys[i].str, key_str.c_str()) == 0) {
                result.push_back(leaf.keys[i].value);
            } else if (strcmp(leaf.keys[i].str, key_str.c_str()) > 0) {
                // Keys are sorted, so we can stop early
                return result;
            }
        }

        // Move to next leaf
        leaf_offset = leaf.next_leaf;
    }

    return result;
}

void BPlusTree::print_tree() {
    // For debugging
    cout << "B+ Tree (root: " << root_offset << ")" << endl;
}