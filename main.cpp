#include "bpt.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    BPlusTree bpt("bpt_data.bin");

    int n;
    cin >> n;
    cin.ignore(); // Ignore newline after n

    for (int i = 0; i < n; i++) {
        string command;
        cin >> command;

        if (command == "insert") {
            string index;
            int value;
            cin >> index >> value;
            bpt.insert(index, value);
        } else if (command == "delete") {
            string index;
            int value;
            cin >> index >> value;
            bpt.remove(index, value);
        } else if (command == "find") {
            string index;
            cin >> index;
            vector<int> values = bpt.find(index);

            if (values.empty()) {
                cout << "null\n";
            } else {
                // Sort values (they should already be sorted from B+ Tree traversal)
                sort(values.begin(), values.end());

                for (size_t j = 0; j < values.size(); j++) {
                    if (j > 0) cout << " ";
                    cout << values[j];
                }
                cout << "\n";
            }
            cout.flush(); // Ensure output is written
        }

        // Ignore rest of line if any
        cin.ignore(1000, '\n');
    }

    return 0;
}