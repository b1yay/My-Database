#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <conio.h>
#include <unordered_map>  // Add this line with other includes
#include <Windows.h>

using namespace std;
namespace fs = std::filesystem;
// Forward declarations to resolve circular dependencies
class DatabaseManager;
class CollectionManager;
class TransactionManager;
// ────── UTILITY FUNCTIONS ──────
int getConsoleWidth() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}

// Add this helper function to display aligned output
void printAlignedCommand(const string& command, const string& structure) {
    int consoleWidth = getConsoleWidth();
    int padding = consoleWidth - command.length() - structure.length() - 4; // 4 spaces for spacing

    if (padding > 0) {
        cout << command << string(padding, ' ') << "[ " << structure << " ]";
    }
    else {
        // If console is too narrow, just print normally
        cout << command << "\nCommand structure: " << structure;
    }
    cout << endl;
}
void printPrettyJson(const string& json) {
    int indent = 0;
    bool inQuotes = false;

    for (char c : json) {
        if (c == '"' && (json.empty() || json.back() != '\\')) {
            inQuotes = !inQuotes;
        }

        if (!inQuotes) {
            if (c == '{' || c == '[') {
                cout << c << "\n" << string(indent + 2, ' ');
                indent += 2;
                continue;
            }
            else if (c == '}' || c == ']') {
                indent = max(0, indent - 2);
                cout << "\n" << string(indent, ' ') << c;
                continue;
            }
            else if (c == ',') {
                cout << c << "\n" << string(indent, ' ');
                continue;
            }
            else if (c == ':') {
                cout << " " << c << " ";
                continue;
            }
        }
        cout << c;
    }
    cout << "\n";
}

// ────── B+ Tree Implementation ──────
template <typename KeyType, typename ValueType>
class BPlusTree {
 
private:
    struct Node {
        bool is_leaf;
        vector<KeyType> keys;
        vector<Node*> children;  // For non-leaf nodes
        vector<ValueType> values; // For leaf nodes
        Node* next;             // For leaf nodes (linked list)
        Node* parent;

        Node(bool leaf = false) : is_leaf(leaf), next(nullptr), parent(nullptr) {}
    };
    Node* root;
    int degree; // Minimum degree (t)

    // Helper functions
    void insertInternal(KeyType key, Node* cursor, Node* child) {
        if (cursor->keys.size() < 2 * degree) {
            // Find position to insert
            auto it = upper_bound(cursor->keys.begin(), cursor->keys.end(), key);
            int pos = it - cursor->keys.begin();

            cursor->keys.insert(it, key);
            cursor->children.insert(cursor->children.begin() + pos + 1, child);
            child->parent = cursor;
        }
        else {
            // Split node
            Node* newInternal = new Node();
            vector<KeyType> tempKeys(cursor->keys);
            vector<Node*> tempChildren(cursor->children);

            // Insert the new key
            auto it = upper_bound(tempKeys.begin(), tempKeys.end(), key);
            int pos = it - tempKeys.begin();
            tempKeys.insert(it, key);
            tempChildren.insert(tempChildren.begin() + pos + 1, child);

            // Split the node
            cursor->keys.clear();
            cursor->children.clear();
            for (int i = 0; i < degree; i++) {
                cursor->keys.push_back(tempKeys[i]);
                cursor->children.push_back(tempChildren[i]);
            }

            KeyType middleKey = tempKeys[degree];

            for (int i = degree + 1; i < tempKeys.size(); i++) {
                newInternal->keys.push_back(tempKeys[i]);
            }
            for (int i = degree + 1; i < tempChildren.size(); i++) {
                newInternal->children.push_back(tempChildren[i]);
                tempChildren[i]->parent = newInternal;
            }

            if (cursor == root) {
                Node* newRoot = new Node();
                newRoot->keys.push_back(middleKey);
                newRoot->children.push_back(cursor);
                newRoot->children.push_back(newInternal);
                root = newRoot;
                cursor->parent = root;
                newInternal->parent = root;
            }
            else {
                insertInternal(middleKey, cursor->parent, newInternal);
            }
        }
    }
    Node* findParent(Node* cursor, Node* child) {
        /* Implementation to find parent of a node */
    }

public:
    // Add default constructor
    BPlusTree() : degree(3), root(nullptr) {}  // Default degree of 3

    // Keep your existing constructor
    BPlusTree(int _degree) : degree(_degree), root(nullptr) {}

    void insert(const KeyType& key, const ValueType& value) {
        if (root == nullptr) {
            root = new Node(true);
            root->keys.push_back(key);
            root->values.push_back(value);
        }
        else {
            Node* cursor = root;
            Node* parent = nullptr;

            // Traverse to the appropriate leaf node
            while (!cursor->is_leaf) {
                parent = cursor;
                int idx = upper_bound(cursor->keys.begin(), cursor->keys.end(), key) - cursor->keys.begin();
                cursor = cursor->children[idx];
            }

            // Insert in leaf node
            int pos = upper_bound(cursor->keys.begin(), cursor->keys.end(), key) - cursor->keys.begin();
            cursor->keys.insert(cursor->keys.begin() + pos, key);
            cursor->values.insert(cursor->values.begin() + pos, value);

            // Handle overflow
            if (cursor->keys.size() > 2 * degree) {
                // Split implementation
            }
        }
    }
    // Single Valuetype return krta hai
    // Single result search (for _id fields)
    ValueType searchSingle(const KeyType& key) {
        if (root == nullptr) {
            return ValueType(); // Return default value if not found
        }

        Node* cursor = root;
        while (!cursor->is_leaf) {
            int idx = upper_bound(cursor->keys.begin(), cursor->keys.end(), key) - cursor->keys.begin();
            cursor = cursor->children[idx];
        }

        auto it = lower_bound(cursor->keys.begin(), cursor->keys.end(), key);
        if (it != cursor->keys.end() && *it == key) {
            int pos = it - cursor->keys.begin();
            return cursor->values[pos];
        }

        return ValueType(); // Not found
    }

    // Add this new method for multiple matches
// Multiple results search (for non-unique fields)
    vector<ValueType> searchAll(const KeyType& key) {
        vector<ValueType> results;
        if (root == nullptr) return results;

        Node* cursor = root;
        while (!cursor->is_leaf) {
            int idx = upper_bound(cursor->keys.begin(), cursor->keys.end(), key) - cursor->keys.begin();
            cursor = cursor->children[idx];
        }

        // Find all matching keys
        for (size_t i = 0; i < cursor->keys.size(); i++) {
            if (cursor->keys[i] == key) {
                results.push_back(cursor->values[i]);
            }
        }

        return results;
    }
    // Range k liye
    vector<ValueType> rangeSearch(const KeyType& start, const KeyType& end) {
        vector<ValueType> results;
        if (root == nullptr) return results;

        Node* cursor = root;
        // Find the starting leaf node
        while (!cursor->is_leaf) {
            int idx = upper_bound(cursor->keys.begin(), cursor->keys.end(), start) - cursor->keys.begin();
            cursor = cursor->children[idx];
        }

        // Traverse leaf nodes until we exceed the end key
        while (cursor != nullptr) {
            for (size_t i = 0; i < cursor->keys.size(); i++) {
                if (cursor->keys[i] >= start && cursor->keys[i] <= end) {
                    results.push_back(cursor->values[i]);
                }
                // Stop if we've passed the end key
                if (cursor->keys[i] > end) {
                    return results;
                }
            }
            cursor = cursor->next;
        }
        return results;
    }
};

// ────── CLASS: DatabaseManager ──────
class DatabaseManager {
public:
    string currentDatabase;

    void createDatabase(const string& dbName) {
        string path = "data/" + dbName;
        if (!fs::exists("data")) fs::create_directory("data");

        if (!fs::exists(path)) {
            fs::create_directory(path);
            cout << "Database '" << dbName << "' created successfully.\n";
        }
        else {
            cout << "Database already exists.\n";
        }
    }

    void useDatabase(const string& dbName) {
        string path = "data/" + dbName;
        if (fs::exists(path)) {
            currentDatabase = dbName;
            cout << "Switched to database '" << dbName << "'.\n";
        }
        else {
            cout << "Database does not exist.\n";
        }
    }
    vector<string> listDatabases() {
        vector<string> databases;
        if (fs::exists("data")) {
            for (const auto& entry : fs::directory_iterator("data")) {
                if (entry.is_directory()) {
                    databases.push_back(entry.path().filename().string());
                }
            }
        }
        return databases;
    }

    vector<string> listCollections(const string& dbName) {
        vector<string> collections;
        string path = "data/" + dbName;
        if (fs::exists(path)) {
            for (const auto& entry : fs::directory_iterator(path)) {
                if (entry.is_regular_file() && entry.path().extension() == ".json") {
                    collections.push_back(entry.path().stem().string());
                }
            }
        }
        return collections;
    }

    bool renameDatabase(const string& oldName, const string& newName) {
        string oldPath = "data/" + oldName;
        string newPath = "data/" + newName;

        if (!fs::exists(oldPath)) {
            cout << "Error: Database '" << oldName << "' does not exist.\n";
            return false;
        }

        if (fs::exists(newPath)) {
            cout << "Error: Database '" << newName << "' already exists.\n";
            return false;
        }

        try {
            fs::rename(oldPath, newPath);

            // Update current database if it's the one being renamed
            if (currentDatabase == oldName) {
                currentDatabase = newName;
            }

            cout << "Database renamed from '" << oldName << "' to '" << newName << "' successfully.\n";
            return true;
        }
        catch (const fs::filesystem_error& e) {
            cerr << "Error renaming database: " << e.what() << "\n";
            return false;
        }
    }

    vector<string> viewCollectionData(const string& dbName, const string& collectionName) {
        vector<string> documents;

        // Trim whitespace from collection name
        string trimmedName = collectionName;
        trimmedName.erase(0, trimmedName.find_first_not_of(" \t\n\r\f\v"));
        trimmedName.erase(trimmedName.find_last_not_of(" \t\n\r\f\v") + 1);

        string path = "data/" + dbName + "/" + trimmedName + ".json";

        cout << "[DEBUG] Attempting to open: " << path << endl;

        if (!fs::exists(path)) {
            cout << "Error: Collection '" << trimmedName << "' doesn't exist\n";
            return documents;
        }

        ifstream file(path);
        if (!file.is_open()) {
            cout << "Error: Could not open collection '" << trimmedName << "'\n";
            return documents;
        }

        string line;
        while (getline(file, line)) {
            if (!line.empty()) {
                documents.push_back(line);
            }
        }

        return documents;
    }
};

// ────── CLASS: TransactionManager ──────
class TransactionManager {
private:
    struct Transaction {
        string id;
        unordered_map<string, string> collectionSnapshots; // path -> content
        time_t startTime;
    };

    unordered_map<string, Transaction> activeTransactions;
    DatabaseManager* dbManager;
    CollectionManager* colManager;

    string generateTransactionId() {
        return "txn_" + to_string(time(nullptr)) + "_" + to_string(rand() % 1000);
    }

public:
    TransactionManager(DatabaseManager* db = nullptr, CollectionManager* col = nullptr)
        : dbManager(db), colManager(col) {}

    void setCollectionManager(CollectionManager* col) { colManager = col; }

    string beginTransaction() {
        string txnId = generateTransactionId();
        activeTransactions[txnId] = { txnId, {}, time(nullptr) };
        cout << "Transaction started. ID: " << txnId << "\n";
        return txnId;
    }

    bool commitTransaction(const string& txnId) {
        if (activeTransactions.find(txnId) == activeTransactions.end()) {
            cout << "Error: No such transaction exists\n";
            return false;
        }
        activeTransactions.erase(txnId);
        cout << "Transaction committed successfully\n";
        return true;
    }

    bool rollbackTransaction(const string& txnId) {
        auto it = activeTransactions.find(txnId);
        if (it == activeTransactions.end()) {
            cout << "Error: No such transaction exists\n";
            return false;
        }

        for (auto& entry : it->second.collectionSnapshots) {
            ofstream file(entry.first);
            if (file) {
                file << entry.second;
                file.close();
            }
        }

        activeTransactions.erase(txnId);
        cout << "Transaction rolled back successfully\n";
        return true;
    }

    void recordChange(const string& txnId, const string& filePath) {
        auto it = activeTransactions.find(txnId);
        if (it != activeTransactions.end() &&
            it->second.collectionSnapshots.find(filePath) == it->second.collectionSnapshots.end()) {
            ifstream inFile(filePath);
            if (inFile) {
                string content((istreambuf_iterator<char>(inFile)),
                    istreambuf_iterator<char>());
                it->second.collectionSnapshots[filePath] = content;
            }
        }
    }

    bool isInTransaction() const {
        return !activeTransactions.empty();
    }

    string getCurrentTxnId() const {
        return activeTransactions.empty() ? "" : activeTransactions.begin()->first;
    }
};

// ────── CLASS: CollectionManager ──────
class CollectionManager {
private:
    // Stores all indices - primary key index will be collectionName._id
    unordered_map<string, BPlusTree<string, streampos>> indices;

    // Transaction Manager to support krne k liye
    TransactionManager* txnManager;

    // Helper to get primary key index name
    string getPrimaryIndexName(const string& collectionName) {
        return collectionName + "._id";
    }

public:
    // Modify constructor to accept TransactionManager
    CollectionManager(TransactionManager* txnMgr = nullptr)
        : txnManager(txnMgr) {}
    // Delete krne k liye 
    bool deleteDocuments(const string& dbName, const string& collectionName,
        const string& field, const string& value) {
        if (dbName.empty()) return false;

        string path = "data/" + dbName + "/" + collectionName + ".json";
        if (!fs::exists(path)) return false;

        // Take snapshot if in transaction
        if (txnManager && txnManager->isInTransaction()) {
            txnManager->recordChange(txnManager->getCurrentTxnId(), path);
        }

        // Read all documents
        ifstream inFile(path);
        vector<string> documents;
        string line;
        while (getline(inFile, line)) {
            documents.push_back(line);
        }
        inFile.close();

        // Filter out matching documents
        vector<string> newDocuments;
        bool deleted = false;
        for (const string& doc : documents) {
            size_t fieldPos = doc.find("\"" + field + "\":");
            if (fieldPos != string::npos) {
                string storedValue = doc.substr(fieldPos + field.length() + 3);
                storedValue = storedValue.substr(0, storedValue.find_first_of(",}"));

                // Remove quotes if present
                if (!storedValue.empty() && storedValue.front() == '"') {
                    storedValue = storedValue.substr(1, storedValue.length() - 2);
                }

                if (storedValue == value) {
                    deleted = true;
                    continue; // Skip this document (delete it)
                }
            }
            newDocuments.push_back(doc);
        }

        // Write remaining documents back
        if (deleted) {
            ofstream outFile(path);
            for (const string& doc : newDocuments) {
                outFile << doc << "\n";
            }
            outFile.close();
        }

        return deleted;
    }

    bool dropCollection(const string& dbName, const string& collectionName) {
        if (dbName.empty()) return false;

        string path = "data/" + dbName + "/" + collectionName + ".json";
        if (fs::exists(path)) {
            // Take snapshot if in transaction
            if (txnManager && txnManager->isInTransaction()) {
                txnManager->recordChange(txnManager->getCurrentTxnId(), path);
            }
            indices.erase(collectionName + "._id"); // Remove primary key index
            return fs::remove(path);
        }
        return false;
    }

    bool dropDatabase(const string& dbName) {
        string path = "data/" + dbName;

        // Debug output to verify path
        cout << "[DEBUG] Attempting to drop database at path: " << path << endl;

        if (fs::exists(path)) {
            // Debug output for existing indices
            cout << "[DEBUG] Current indices before deletion:" << endl;
            for (const auto& index : indices) {
                cout << "- " << index.first << endl;
            }

            // Remove all indices for collections in this database
            string prefix = dbName + ".";
            for (auto it = indices.begin(); it != indices.end(); ) {
                if (it->first.find(prefix) == 0) {
                    cout << "[DEBUG] Removing index: " << it->first << endl;
                    it = indices.erase(it);
                }
                else {
                    ++it;
                }
            }

            // Remove directory and all contents
            try {
                uintmax_t result = fs::remove_all(path);
                cout << "[DEBUG] Deleted " << result << " files/directories" << endl;
                return result > 0;
            }
            catch (const fs::filesystem_error& e) {
                cerr << "[ERROR] Filesystem error: " << e.what() << endl;
                return false;
            }
        }
        else {
            cout << "[DEBUG] Database path does not exist: " << path << endl;
        }
        return false;
    }
    // collection name update k liye
    bool renameCollection(const string& dbName, const string& oldName, const string& newName) {
        if (dbName.empty()) {
            cout << "Error: No database selected.\n";
            return false;
        }
        string oldPath = "data/" + dbName + "/" + oldName + ".json";
        string newPath = "data/" + dbName + "/" + newName + ".json";

        if (!fs::exists(oldPath)) {
            cout << "Error: Collection '" << oldName << "' does not exist.\n";
            return false;
        }

        if (fs::exists(newPath)) {
            cout << "Error: Collection '" << newName << "' already exists.\n";
            return false;
        }

        // Take snapshot if in transaction
        if (txnManager && txnManager->isInTransaction()) {
            txnManager->recordChange(txnManager->getCurrentTxnId(), oldPath);
        }
        try {
            fs::rename(oldPath, newPath);

            // Update indices if they exist
            string oldIndexPrefix = oldName + ".";
            string newIndexPrefix = newName + ".";

            for (auto it = indices.begin(); it != indices.end(); ) {
                if (it->first.find(oldIndexPrefix) == 0) {
                    string newKey = newIndexPrefix + it->first.substr(oldIndexPrefix.length());
                    indices[newKey] = move(it->second);
                    it = indices.erase(it);
                }
                else {
                    ++it;
                }
            }

            cout << "Collection renamed from '" << oldName << "' to '" << newName << "' successfully.\n";
            return true;
        }
        catch (const fs::filesystem_error& e) {
            cerr << "Error renaming collection: " << e.what() << "\n";
            return false;
        }
    }
    //Update krne k liye
    bool updateDocument(const string& dbName, const string& collectionName,
        const string& whereField, const string& whereValue,
        const string& updateField, const string& updateValue) {
        if (dbName.empty()) return false;

        string path = "data/" + dbName + "/" + collectionName + ".json";
        if (!fs::exists(path)) return false;

        // Take snapshot if in transaction
        if (txnManager && txnManager->isInTransaction()) {
            txnManager->recordChange(txnManager->getCurrentTxnId(), path);
        }

        // Read all documents
        ifstream inFile(path);
        vector<string> documents;
        string line;
        while (getline(inFile, line)) {
            documents.push_back(line);
        }
        inFile.close();

        // Find and update matching documents
        bool updated = false;
        for (string& doc : documents) {
            size_t fieldPos = doc.find("\"" + whereField + "\":");
            if (fieldPos != string::npos) {
                string storedValue = doc.substr(fieldPos + whereField.length() + 3);
                storedValue = storedValue.substr(0, storedValue.find_first_of(",}"));

                // Remove quotes if present
                if (!storedValue.empty() && storedValue.front() == '"') {
                    storedValue = storedValue.substr(1, storedValue.length() - 2);
                }

                if (storedValue == whereValue) {
                    // Update the field
                    size_t updatePos = doc.find("\"" + updateField + "\":");
                    if (updatePos != string::npos) {
                        size_t valueStart = updatePos + updateField.length() + 3;
                        size_t valueEnd = doc.find_first_of(",}", valueStart);
                        string newValue = (updateValue.front() == '"' && updateValue.back() == '"')
                            ? updateValue
                            : "\"" + updateValue + "\"";
                        doc.replace(valueStart, valueEnd - valueStart, newValue);
                        updated = true;
                    }
                }
            }
        }

        // Write all documents back
        if (updated) {
            ofstream outFile(path);
            for (const string& doc : documents) {
                outFile << doc << "\n";
            }
            outFile.close();
        }

        return updated;
    }
    vector<string> rangeQuery(const string& dbName, const string& collectionName,
        const string& field, const string& start, const string& end) {
        vector<string> results;
        string path = "data/" + dbName + "/" + collectionName + ".json";
        ifstream file(path);

        string indexName = collectionName + "." + field;
        if (field == "_id") {
            indexName = getPrimaryIndexName(collectionName);
        }

        // Try indexed search first
        if (indices.find(indexName) != indices.end()) {
            vector<streampos> positions = indices[indexName].rangeSearch(start, end);
            for (streampos pos : positions) {
                file.seekg(pos);
                string document;
                if (getline(file, document)) {
                    results.push_back(document);
                }
            }
            return results;
        }

        // Fallback to full scan
        string line;
        while (getline(file, line)) {
            size_t fieldPos = line.find("\"" + field + "\":");
            if (fieldPos != string::npos) {
                string storedValue = line.substr(fieldPos + field.length() + 3);
                storedValue = storedValue.substr(0, storedValue.find_first_of(",}"));

                // Remove quotes if present
                if (!storedValue.empty() && storedValue.front() == '"') {
                    storedValue = storedValue.substr(1, storedValue.length() - 2);
                }

                // Check if numeric comparison
                bool isNumeric = !storedValue.empty() && isdigit(storedValue[0]);

                if (isNumeric) {
                    try {
                        int numValue = stoi(storedValue);
                        int numStart = start.empty() ? INT_MIN : stoi(start);
                        int numEnd = end.empty() ? INT_MAX : stoi(end);

                        if (numValue >= numStart && numValue <= numEnd) {
                            results.push_back(line);
                        }
                    }
                    catch (...) {
                        // If conversion fails, do string comparison
                        if ((start.empty() || storedValue >= start) &&
                            (end.empty() || storedValue <= end)) {
                            results.push_back(line);
                        }
                    }
                }
                else {
                    // String comparison
                    if ((start.empty() || storedValue >= start) &&
                        (end.empty() || storedValue <= end)) {
                        results.push_back(line);
                    }
                }
            }
        }
        return results;
    }
    // When creating a new index, specify the degree (3 in this case)
    void createIndex(const string& dbName, const string& collectionName,
        const string& fieldName) {
        string path = "data/" + dbName + "/" + collectionName + ".json";
        ifstream file(path);
        string line;

        // Initialize with degree 3
        indices[collectionName + "." + fieldName] = BPlusTree<string, streampos>(3);

        while (getline(file, line)) {
            streampos position = file.tellg();
            size_t fieldPos = line.find("\"" + fieldName + "\":");
            if (fieldPos != string::npos) {
                string value = line.substr(fieldPos + fieldName.length() + 3);
                value = value.substr(0, value.find_first_of(",}\""));

                if (!value.empty() && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.length() - 2);
                }

                indices[collectionName + "." + fieldName].insert(value, position);
            }
        }
        cout << "Index created on " << fieldName << ".\n";
    }
   void createCollection(const string& dbName, const string& collectionName) {
    if (dbName.empty()) {
        cout << "Please select a database using 'USE <name>' first.\n";
        return;
    }

    // Trim and validate collection name
    string trimmedName = collectionName;
    trimmedName.erase(0, trimmedName.find_first_not_of(" \t\n\r\f\v"));
    trimmedName.erase(trimmedName.find_last_not_of(" \t\n\r\f\v") + 1);

    if (trimmedName.empty()) {
        cout << "Error: Collection name cannot be empty\n";
        return;
    }

    string path = "data/" + dbName + "/" + trimmedName + ".json";
    if (fs::exists(path)) {
        cout << "Collection '" << trimmedName << "' already exists.\n";
        return;
    }
    
    // Create parent directory if needed
    fs::create_directories("data/" + dbName);
    ofstream file(path);
    if (file) {
        file.close();
        cout << "Collection '" << trimmedName << "' created successfully.\n";
    }
    else {
        cout << "Error: Failed to create collection file\n";
    }
}
    // Modified insertDocument to automatically index _id
   void insertDocument(const string& dbName, const string& collectionName,
       const string& jsonDocument) {
       if (dbName.empty()) {
           cout << "No database selected.\n";
           return;
       }

       string path = "data/" + dbName + "/" + collectionName + ".json";
       if (!fs::exists(path)) {
           cout << "Error: Collection '" << collectionName << "' doesn't exist.\n";
           return;
       }

       // Take snapshot if in transaction
       if (txnManager && txnManager->isInTransaction()) {
           txnManager->recordChange(txnManager->getCurrentTxnId(), path);
       }


       // Open file to get size first
       ifstream inFile(path, ios::ate);
       streampos position = inFile.tellg();
       inFile.close();

       // Now append the document
       ofstream file(path, ios::app);
       if (file) {
           file << jsonDocument << "\n";
           file.close();

           // Automatically index _id field
           size_t id_pos = jsonDocument.find("\"_id\":");
           if (id_pos != string::npos) {
               string id_value = jsonDocument.substr(id_pos + 6);
               id_value = id_value.substr(0, id_value.find_first_of(",}\""));

               // Remove quotes if present and trim
               if (!id_value.empty() && id_value.front() == '"') {
                   id_value = id_value.substr(1, id_value.length() - 2);
               }
               id_value.erase(0, id_value.find_first_not_of(" \t"));
               id_value.erase(id_value.find_last_not_of(" \t") + 1);

               string indexName = getPrimaryIndexName(collectionName);
               if (indices.find(indexName) == indices.end()) {
                   indices[indexName] = BPlusTree<string, streampos>(3);
               }

               // Add to primary key index
               indices[indexName].insert(id_value, position);
           }

           cout << "Document inserted.\n";
       }
       else {
           cout << "Error opening collection.\n";
       }
   }
   // Keep this for _id searches
   string findDocument(const string& dbName, const string& collectionName,
       const string& field, const string& value) {
       string path = "data/" + dbName + "/" + collectionName + ".json";
       ifstream file(path);
       string line;

       // Special handling for _id field
       if (field == "_id") {
           string indexName = getPrimaryIndexName(collectionName);
           if (indices.find(indexName) != indices.end()) {
               // Search using the primary key index
               vector<streampos> positions = indices[indexName].searchAll(value);
               if (!positions.empty()) {
                   file.seekg(positions[0]);
                   if (getline(file, line)) {
                       return line;
                   }
               }
           }
       }

       // Fallback to full scan
       while (getline(file, line)) {
           size_t fieldPos = line.find("\"" + field + "\":");
           if (fieldPos != string::npos) {
               string storedValue = line.substr(fieldPos + field.length() + 3);
               storedValue = storedValue.substr(0, storedValue.find_first_of(",}"));

               // Remove quotes if present
               if (!storedValue.empty() && storedValue.front() == '"') {
                   storedValue = storedValue.substr(1, storedValue.length() - 2);
               }

               if (storedValue == value) {
                   return line;
               }
           }
       }

       return "Document not found.";
   }
   // Modified findDocument to use primary key index automatically
   vector<string> findDocuments(const string& dbName, const string& collectionName,
       const string& field, const string& value) {
       vector<string> results;
       string path = "data/" + dbName + "/" + collectionName + ".json";
       ifstream file(path);
       string line;

       string indexName = collectionName + "." + field;
       if (indices.find(indexName) != indices.end()) {
           // For indexed fields, use searchAll
           vector<streampos> positions = indices[indexName].searchAll(value);
           for (streampos pos : positions) {
               file.seekg(pos);
               string document;
               if (getline(file, document)) {
                   results.push_back(document);
               }
           }
           return results;
       }

       // Fallback to full scan if no index
       while (getline(file, line)) {
           size_t fieldPos = line.find("\"" + field + "\":");
           if (fieldPos != string::npos) {
               string storedValue = line.substr(fieldPos + field.length() + 3);
               storedValue = storedValue.substr(0, storedValue.find_first_of(",}"));

               // Remove quotes if present
               if (!storedValue.empty() && storedValue.front() == '"') {
                   storedValue = storedValue.substr(1, storedValue.length() - 2);
               }

               // Trim whitespace
               storedValue.erase(0, storedValue.find_first_not_of(" \t"));
               storedValue.erase(storedValue.find_last_not_of(" \t") + 1);

               if (storedValue == value) {
                   results.push_back(line);
               }
           }
       }

       return results;
   }
};

// ────── CLASS: QueryParser ──────
class QueryParser {
private:
    DatabaseManager& dbManager;
    CollectionManager& colManager;
    TransactionManager& txnManager;

    // Helper function to parse WHERE clause
    pair<string, string> parseWhereClause(const string& whereClause) {
        size_t eqPos = whereClause.find('=');
        if (eqPos == string::npos) return { "", "" };

        string field = whereClause.substr(0, eqPos);
        string value = whereClause.substr(eqPos + 1);

        // Trim whitespace
        field.erase(0, field.find_first_not_of(" \t"));
        field.erase(field.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // Remove outer quotes if present
        if (!value.empty() && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.length() - 2);
        }

        return { field, value };
    }
    // Helper method
    void printIndexUsageInfo(const string& collectionName, const string& field) {
        if (field == "_id") {
            cout << "[System] Note: Primary key (_id) queries are automatically indexed\n";
        }
        else {
            cout << "[System] Tip: Create an index with 'CREATE INDEX ON "
                << collectionName << "(" << field << ")' for faster searches\n";
        }
    }
    string trim(const string& str) {
        string result = str;
        result.erase(0, result.find_first_not_of(" \t"));
        result.erase(result.find_last_not_of(" \t") + 1);
        return result;
    }
public:
    QueryParser(DatabaseManager& db, CollectionManager& col, TransactionManager& txn)
        : dbManager(db), colManager(col), txnManager(txn) {}

    void parseRangeQuery(const string& command) {
        string upperCommand = command;
        transform(upperCommand.begin(), upperCommand.end(), upperCommand.begin(), ::toupper);

        size_t wherePos = upperCommand.find(" WHERE ");
        if (wherePos == string::npos) {
            cout << "Error: Missing WHERE clause in range query\n";
            return;
        }

        string collectionName = command.substr(11, wherePos - 11);
        string condition = command.substr(wherePos + 7);

        // Trim collection name
        collectionName.erase(0, collectionName.find_first_not_of(" \t"));
        collectionName.erase(collectionName.find_last_not_of(" \t") + 1);

        // Parse condition
        size_t opPos = condition.find_first_of("<>=");
        string field, start, end;

        if (upperCommand.find("BETWEEN") != string::npos) {
            // BETWEEN format
            size_t betweenPos = upperCommand.find("BETWEEN", wherePos);
            size_t andPos = upperCommand.find("AND", betweenPos);

            field = trim(condition.substr(0, betweenPos - (wherePos + 7)));
            start = trim(condition.substr(betweenPos + 7 - (wherePos + 7),
                andPos - (betweenPos + 7)));
            end = trim(condition.substr(andPos + 3 - (wherePos + 7)));
        }
        else {
            // Comparison format
            field = trim(condition.substr(0, opPos));
            string op = condition.substr(opPos, condition.find_first_not_of("<>=", opPos) - opPos);
            string value = trim(condition.substr(opPos + op.length()));

            if (op == ">" || op == ">=") {
                start = value;
                if (op == ">") start += "\x01"; // For exclusive >
            }
            else if (op == "<" || op == "<=") {
                end = value;
                if (op == "<") end += "\x01"; // For exclusive <
            }
        }

        // Remove quotes if present
        if (!start.empty() && start.front() == '"') start = start.substr(1, start.length() - 2);
        if (!end.empty() && end.front() == '"') end = end.substr(1, end.length() - 2);

        vector<string> results = colManager.rangeQuery(
            dbManager.currentDatabase, collectionName, field, start, end);

        if (results.empty()) {
            cout << "No documents found in range.\n";
        }
        else {
            cout << "Found " << results.size() << " documents:\n";
            for (const string& doc : results) {
                printPrettyJson(doc);
                cout << "-----------------------------------------\n";
            }
        }
    }
    // SINGLE parse() function combining all functionality
    void parse(const string& command) {
        if (command == "BEGIN TRANSACTION") {
            txnManager.beginTransaction();
            return;
        }
        else if (command == "COMMIT") {
            txnManager.commitTransaction(txnManager.getCurrentTxnId());
            return;
        }
        else if (command == "ROLLBACK") {
            txnManager.rollbackTransaction(txnManager.getCurrentTxnId());
            return;
        }

        if (command.find("CREATE DATABASE ") == 0) {
            string dbName = command.substr(16);
            dbManager.createDatabase(dbName);
        }
        else if (command.find("USE ") == 0) {
            string dbName = command.substr(4);
            dbManager.useDatabase(dbName);
        }
        else if (command.find("CREATE COLLECTION ") == 0) {
            string colName = command.substr(17);  // Fixed length to 17
            colManager.createCollection(dbManager.currentDatabase, colName);
        }
        else if (command.find("INSERT INTO ") == 0) {
            size_t valuesPos = command.find(" VALUES ");
            if (valuesPos != string::npos) {
                string collectionName = command.substr(12, valuesPos - 12);
                string jsonDoc = command.substr(valuesPos + 8);
                colManager.insertDocument(dbManager.currentDatabase, collectionName, jsonDoc);
            }
        }
        else if (command.find("CREATE INDEX ON ") == 0) {
            size_t fieldPos = command.find('(');
            size_t endPos = command.find(')');
            if (fieldPos != string::npos && endPos != string::npos) {
                string collectionName = command.substr(16, fieldPos - 16);
                string fieldName = command.substr(fieldPos + 1, endPos - fieldPos - 1);
                colManager.createIndex(dbManager.currentDatabase, collectionName, fieldName);
            }
        }
        else if (command == "LIST DATABASES") {
            vector<string> databases = dbManager.listDatabases();
            cout << "Available Databases:\n";
            for (const auto& db : databases) {
                cout << "- " << db << "\n";
            }
        }
        else if (command.find("FIND FROM ") == 0) {
            size_t wherePos = command.find(" WHERE ");
            if (wherePos == string::npos) {
                wherePos = command.find(" where "); // Case-insensitive
            }

            if (wherePos != string::npos) {
                string collectionName = command.substr(10, wherePos - 10);
                // Trim whitespace from collection name
                collectionName.erase(0, collectionName.find_first_not_of(" \t"));
                collectionName.erase(collectionName.find_last_not_of(" \t") + 1);

                string whereClause = command.substr(wherePos + 7);
                auto [field, value] = parseWhereClause(whereClause);


                if (field == "_id") {
                    cout << "[System] Using built-in primary key index...\n";
                    string result = colManager.findDocument(
                        dbManager.currentDatabase, collectionName, field, value);
                    if (result == "Document not found.") {
                        cout << "Error: No document found with " << field << " = " << value << "\n";
                    }
                    else {
                        cout << "Found document:\n";
                        printPrettyJson(result);
                    }
                }
                else {
                    vector<string> results = colManager.findDocuments(
                        dbManager.currentDatabase, collectionName, field, value);
                    if (results.empty()) {
                        cout << "Error: No documents found with " << field << " = " << value << "\n";
                    }
                    else {
                        cout << "Found " << results.size() << " documents:\n";
                        for (const string& doc : results) {
                            printPrettyJson(doc);
                            cout << "-----------------------------------------\n";
                        }
                    }
                }
            }
            else {
                cout << "Error: FIND command requires WHERE clause\n";
            }
        }
        else if (command.find("DELETE FROM ") == 0) {
            size_t wherePos = command.find(" WHERE ");
            if (wherePos != string::npos) {
                string collectionName = command.substr(12, wherePos - 12);
                string whereClause = command.substr(wherePos + 7);
                auto [field, value] = parseWhereClause(whereClause);

                if (colManager.deleteDocuments(dbManager.currentDatabase, collectionName, field, value)) {
                    cout << "Document(s) deleted successfully.\n";
                }
                else {
                    cout << "Failed to delete document(s).\n";
                }
            }
            else {
                cout << "Invalid DELETE syntax. Use: DELETE FROM <collection> WHERE <field>=<value>\n";
            }
        }
        else if (command.find("DROP COLLECTION ") == 0) {
            string collectionName = command.substr(16);
            if (colManager.dropCollection(dbManager.currentDatabase, collectionName)) {
                cout << "Collection dropped successfully.\n";
            }
            else {
                cout << "Failed to drop collection.\n";
            }
        }
        else if (command.find("DROP DATABASE ") == 0) {
            string dbName = command.substr(13);
            dbName.erase(0, dbName.find_first_not_of(" \t"));
            dbName.erase(dbName.find_last_not_of(" \t") + 1);

            cout << "[DEBUG] Preparing to drop database: " << dbName << endl;

            if (colManager.dropDatabase(dbName)) {
                // Clear current database if it's the one being dropped
                if (dbManager.currentDatabase == dbName) {
                    dbManager.currentDatabase = "";
                }
                cout << "Database dropped successfully.\n";
            }
            else {
                cout << "Failed to drop database. See debug output for details.\n";
            }
            }
        else if (command == "LIST COLLECTIONS") {
            if (dbManager.currentDatabase.empty()) {
                cout << "Please select a database first using 'USE <name>'\n";
                return;
            }
            vector<string> collections = dbManager.listCollections(dbManager.currentDatabase);
            cout << "Collections in " << dbManager.currentDatabase << ":\n";
            for (const auto& col : collections) {
                cout << "- " << col << "\n";
            }
        }
        else if (command.find("VIEW COLLECTION DATA ") == 0) {
            if (dbManager.currentDatabase.empty()) {
                cout << "Please select a database first using 'USE <name>'\n";
                return;
            }

            string collectionName = command.substr(20);

            // Trim whitespace from collection name
            collectionName.erase(0, collectionName.find_first_not_of(" \t"));
            collectionName.erase(collectionName.find_last_not_of(" \t") + 1);

            vector<string> documents = dbManager.viewCollectionData(dbManager.currentDatabase, collectionName);

            cout << "\nDocuments in " << collectionName << " (" << documents.size() << "):\n";
            cout << "-----------------------------------------\n";
            for (size_t i = 0; i < documents.size(); i++) {
                cout << "Data #" << (i + 1) << ":\n";
                printPrettyJson(documents[i]);
                cout << "-----------------------------------------\n";
            }
        }
        else if (command.find("CREATE INDEX ON ") == 0) {
            size_t fieldPos = command.find('(');
            size_t endPos = command.find(')');
            if (fieldPos != string::npos && endPos != string::npos) {
                string collectionName = command.substr(16, fieldPos - 16);
                string fieldName = command.substr(fieldPos + 1, endPos - fieldPos - 1);
                colManager.createIndex(dbManager.currentDatabase, collectionName, fieldName);
            }
        }
        else if (command.find("UPDATE DATABASE NAME ") == 0) {
            size_t toPos = command.find(" TO ");
            if (toPos != string::npos) {
                string oldName = command.substr(20, toPos - 20);
                string newName = command.substr(toPos + 4);

                // Trim whitespace
                oldName.erase(0, oldName.find_first_not_of(" \t"));
                oldName.erase(oldName.find_last_not_of(" \t") + 1);
                newName.erase(0, newName.find_first_not_of(" \t"));
                newName.erase(newName.find_last_not_of(" \t") + 1);

                if (dbManager.renameDatabase(oldName, newName)) {  // Call through dbManager
                    cout << "Database name updated successfully.\n";
                }
                else {
                    cout << "Failed to update database name.\n";
                }
            }
            else {
                cout << "Invalid syntax. Use: UPDATE DATABASE NAME <old_name> TO <new_name>\n";
            }
            }
        else if (command.find("UPDATE TABLE NAME ") == 0) {
            size_t toPos = command.find(" TO ");
            if (toPos != string::npos) {
                string oldName = command.substr(17, toPos - 17);
                string newName = command.substr(toPos + 4);

                // Trim whitespace
                oldName.erase(0, oldName.find_first_not_of(" \t"));
                oldName.erase(oldName.find_last_not_of(" \t") + 1);
                newName.erase(0, newName.find_first_not_of(" \t"));
                newName.erase(newName.find_last_not_of(" \t") + 1);

                if (colManager.renameCollection(dbManager.currentDatabase, oldName, newName)) {
                    cout << "Collection renamed successfully.\n";
                }
                else {
                    cout << "Failed to rename collection.\n";
                }
            }
            else {
                cout << "Invalid syntax. Use: UPDATE TABLE NAME <old_name> TO <new_name>\n";
            }
            }
        else if (command.find("UPDATE ") == 0) {
            size_t setPos = command.find(" SET ");
            size_t wherePos = command.find(" WHERE ");

            if (setPos != string::npos && wherePos != string::npos) {
                string collectionName = command.substr(7, setPos - 7);
                string setClause = command.substr(setPos + 5, wherePos - (setPos + 5));
                string whereClause = command.substr(wherePos + 7);

                // Parse SET clause (field=value)
                size_t eqPos = setClause.find('=');
                string updateField = setClause.substr(0, eqPos);
                string updateValue = setClause.substr(eqPos + 1);

                // Parse WHERE clause (field=value)
                auto [whereField, whereValue] = parseWhereClause(whereClause);

                // Trim whitespace
                updateField.erase(0, updateField.find_first_not_of(" \t"));
                updateField.erase(updateField.find_last_not_of(" \t") + 1);
                updateValue.erase(0, updateValue.find_first_not_of(" \t"));
                updateValue.erase(updateValue.find_last_not_of(" \t") + 1);

                if (colManager.updateDocument(dbManager.currentDatabase, collectionName,
                    whereField, whereValue, updateField, updateValue)) {
                    cout << "Document updated successfully.\n";
                }
                else {
                    cout << "Failed to update document.\n";
                }
            }
            else {
                cout << "Invalid UPDATE syntax. Use: UPDATE <collection> SET <field>=<value> WHERE <field>=<value>\n";
            }
        }
        else if (command.find("RANGE FROM ") == 0) {
            parseRangeQuery(command);
        }
        else if (command.find("FIND FROM ") == 0) {
            size_t wherePos = command.find(" WHERE ");
            if (wherePos == string::npos) {
                wherePos = command.find(" where "); // Case-insensitive
            }

            if (wherePos != string::npos) {
                string collectionName = command.substr(10, wherePos - 10);
                // Trim whitespace from collection name
                collectionName.erase(0, collectionName.find_first_not_of(" \t"));
                collectionName.erase(collectionName.find_last_not_of(" \t") + 1);

                string whereClause = command.substr(wherePos + 7);
                auto [field, value] = parseWhereClause(whereClause);

                if (field == "_id") {
                    cout << "[System] Using built-in primary key index...\n";
                    string result = colManager.findDocument(  // Using findDocument for _id
                        dbManager.currentDatabase, collectionName, field, value);
                    if (result == "Document not found.") {
                        cout << "Error: No document found with " << field << " = " << value << "\n";
                    }
                    else {
                        cout << "Found document:\n";
                        printPrettyJson(result);
                    }
                }
                else {
                    vector<string> results = colManager.findDocuments(  // Using findDocuments for others
                        dbManager.currentDatabase, collectionName, field, value);
                    if (results.empty()) {
                        cout << "Error: No documents found with " << field << " = " << value << "\n";
                    }
                    else {
                        cout << "Found " << results.size() << " documents:\n";
                        for (const string& doc : results) {
                            printPrettyJson(doc);
                            cout << "-----------------------------------------\n";
                        }
                    }
                }
            }
            else {
                cout << "Error: FIND command requires WHERE clause\n";
            }
        }
        else {
            cout << "Unknown or unsupported command.\n";
        }
    }
};

// ────── UI Functions ──────
void showWelcomeScreen() {
    system("cls");
    cout << R"(
  _______               ______   _______ _________ _______  ______   _______  _______  _______ 
(       )|\     /|    (  __  \ (  ___  )\__   __/(  ___  )(  ___ \ (  ___  )(  ____ \(  ____ \
| () () |( \   / )    | (  \  )| (   ) |   ) (   | (   ) || (   ) )| (   ) || (    \/| (    \/
| || || | \ (_) /     | |   ) || (___) |   | |   | (___) || (__/ / | (___) || (_____ | (__    
| |(_)| |  \   /      | |   | ||  ___  |   | |   |  ___  ||  __ (  |  ___  |(_____  )|  __)   
| |   | |   ) (       | |   ) || (   ) |   | |   | (   ) || (  \ \ | (   ) |      ) || (      
| )   ( |   | |       | (__/  )| )   ( |   | |   | )   ( || )___) )| )   ( |/\____) || (____/\
|/     \|   \_/       (______/ |/     \|   )_(   |/     \||/ \___/ |/     \|\_______)(_______/
                                                                                              
)";
    cout << "\nPress any key to go to Main Menu...";
    int ch = _getch();
    system("cls");
}

void showMainMenu() {
    system("cls");
    int consoleWidth = getConsoleWidth();
    const int MIN_CONSOLE_WIDTH = 80; // Minimum width for two-column display

    cout << "=========== MAIN MENU ===========\n\n";
    cout << "Available Commands:\n\n";

    vector<pair<string, string>> commands = {
        {"1. USE <name>", "USE mydatabase"},
        {"2. CREATE DATABASE <name>", "CREATE DATABASE mydatabase"},
        {"3. CREATE COLLECTION <name>", "CREATE COLLECTION users"},
        {"4. INSERT INTO <collection> VALUES <JSON>", "INSERT INTO users VALUES {\"_id\":\"1\", \"name\":\"John\", \"age\":30}"},
        {"5. CREATE INDEX ON <collection>(<field>)", "CREATE INDEX ON users(name)"},
        {"6. LIST DATABASES", "LIST DATABASES"},
        {"7. LIST COLLECTIONS", "LIST COLLECTIONS"},
        {"8. FIND FROM <collection> WHERE <field>=<value>", "FIND FROM users WHERE name=\"John\""},
        {"9. RANGE FROM <collection> WHERE <field> BETWEEN <value1> AND <value2>", "RANGE FROM users WHERE age BETWEEN 20 AND 30"},
        {"10. RANGE FROM <collection> WHERE <field> >= <value>", "RANGE FROM users WHERE age >= 25"},
        {"11. RANGE FROM <collection> WHERE <field> <= <value>", "RANGE FROM users WHERE age <= 40"},
        {"12. VIEW COLLECTION DATA <collection>", "VIEW COLLECTION DATA users"},
        {"13. UPDATE <collection> SET <field>=<value> WHERE <field>=<value>", "UPDATE users SET name=\"John Doe\" WHERE id=\"1\""},
        {"14. UPDATE TABLE NAME <old_name> TO <new_name>", "UPDATE TABLE NAME customers TO clients"},
        {"15. UPDATE DATABASE NAME <old_name> TO <new_name>", "UPDATE DATABASE NAME olddb TO newdb"},
        {"16. DELETE FROM <collection> WHERE <field>=<value>", "DELETE FROM users WHERE id=\"1\""},
        {"17. DROP COLLECTION <name>", "DROP COLLECTION users"},
        {"18. DROP DATABASE <name>", "DROP DATABASE mydatabase"},
        {"19. BEGIN TRANSACTION", "BEGIN TRANSACTION"},
        {"19. COMMIT", "COMMIT"},
        {"20. ROLLBACK", "ROLLBACK"},
        {"21. EXIT", "EXIT"}
    };

    if (consoleWidth >= MIN_CONSOLE_WIDTH) {
        // Two-column display
        int leftWidth = 0;
        for (const auto& cmd : commands) {
            if (cmd.first.length() > leftWidth) {
                leftWidth = cmd.first.length();
            }
        }
        leftWidth += 4; // Add some padding

        for (const auto& cmd : commands) {
            cout << left << setw(leftWidth) << cmd.first
                << "[ " << cmd.second << " ]" << endl;
        }
    }
    else {
        // Single-column display for narrow consoles
        for (const auto& cmd : commands) {
            cout << cmd.first << endl;
            cout << "   Example: " << cmd.second << endl << endl;
        }
    }

    cout << "\nEnter Command:\n> ";
}
// ────── MAIN FUNCTION ──────
int main() {
    // int main() {
    // DEBUG: Show program starting
    cout << "[DEBUG] Starting database system..." << endl;

    // Set console to UTF-8
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    // DEBUG: Verify console encoding
    cout << "[DEBUG] Console encoding set to: " << GetConsoleOutputCP() << endl;
    // Initialize components in correct order
    DatabaseManager dbManager;
    TransactionManager txnManager(&dbManager, nullptr);  // colManager not available yet
    CollectionManager colManager(&txnManager);
    txnManager.setCollectionManager(&colManager);  // Now we can set it

    QueryParser parser(dbManager, colManager, txnManager);
    string command;
    showWelcomeScreen();
    showMainMenu();

    while (true) {
        cout << "> ";
        getline(cin, command);

        // Convert to uppercase for case-insensitive comparison
        string upperCommand = command;
        transform(upperCommand.begin(), upperCommand.end(), upperCommand.begin(), ::toupper);

        if (upperCommand == "EXIT" || upperCommand == "21") {
            cout << "Exiting...\n";
            break;
        }

        parser.parse(command);
        cout << "\n";
    }

    return 0;
}
