# My Database - A Lightweight NoSQL Implementation

A C++ database system featuring B+ tree indexing, transaction support, and JSON document storage.

## Acknowledgments
Special thanks to Prof. Dr. Atif Hussain phpmysqlguru@gmail.com for guidance on this project.

## Key Features

- **Complete CRUD support**: Create, Read, Update, and Delete databases, collections, and documents
- **B+ Tree Indexing**: Fast document retrieval with configurable tree degree
- **ACID Transactions**: Full transaction support with BEGIN/COMMIT/ROLLBACK
- **JSON Document Storage**: Store and query JSON documents
- **CLI Interface**: Interactive command-line interface
- **Range Queries**: Efficient range searches using B+ tree indices

## Quick Start

```bash
# Clone the repository
git clone https://github.com/b1yay/My-Database.git

# Compile (requires C++17)
g++ -std=c++17 main.cpp -o database

# Run
./database
