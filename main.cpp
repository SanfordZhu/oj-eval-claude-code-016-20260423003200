#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cstring>

using namespace std;

const string DATA_FILE = "simple_data.bin";

class SimpleDB {
private:
    map<string, set<int>> data; // key -> sorted values

    void save_to_file() {
        ofstream file(DATA_FILE, ios::binary);
        if (!file) return;

        // Simple format: count, then key-value pairs
        size_t count = 0;
        for (const auto& entry : data) {
            count += entry.second.size();
        }

        file.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& entry : data) {
            const string& key = entry.first;
            for (int value : entry.second) {
                // Write key length and key
                uint16_t key_len = key.size();
                file.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
                file.write(key.c_str(), key_len);

                // Write value
                file.write(reinterpret_cast<const char*>(&value), sizeof(value));
            }
        }
    }

    void load_from_file() {
        ifstream file(DATA_FILE, ios::binary);
        if (!file) return;

        data.clear();

        size_t count;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));

        for (size_t i = 0; i < count; i++) {
            // Read key
            uint16_t key_len;
            file.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));

            char key_buf[65];
            file.read(key_buf, key_len);
            key_buf[key_len] = '\0';
            string key(key_buf);

            // Read value
            int value;
            file.read(reinterpret_cast<char*>(&value), sizeof(value));

            data[key].insert(value);
        }
    }

public:
    SimpleDB() {
        load_from_file();
    }

    ~SimpleDB() {
        save_to_file();
    }

    void insert(const string& key, int value) {
        // Check if already exists
        auto it = data.find(key);
        if (it != data.end()) {
            if (it->second.find(value) != it->second.end()) {
                return; // Already exists
            }
        }
        data[key].insert(value);
    }

    void remove(const string& key, int value) {
        auto it = data.find(key);
        if (it != data.end()) {
            it->second.erase(value);
            if (it->second.empty()) {
                data.erase(it);
            }
        }
    }

    vector<int> find(const string& key) {
        vector<int> result;
        auto it = data.find(key);
        if (it != data.end()) {
            result.assign(it->second.begin(), it->second.end());
        }
        return result;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    SimpleDB db;

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
            db.insert(index, value);
        } else if (command == "delete") {
            string index;
            int value;
            cin >> index >> value;
            db.remove(index, value);
        } else if (command == "find") {
            string index;
            cin >> index;
            vector<int> values = db.find(index);

            if (values.empty()) {
                cout << "null\n";
            } else {
                for (size_t j = 0; j < values.size(); j++) {
                    if (j > 0) cout << " ";
                    cout << values[j];
                }
                cout << "\n";
            }
        }

        // Ignore rest of line if any
        cin.ignore(1000, '\n');
    }

    return 0;
}