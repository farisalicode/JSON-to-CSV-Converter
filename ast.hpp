#ifndef AST_HPP
#define AST_HPP

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>



// Node types in our AST
enum NodeType {
    NODE_OBJECT,
    NODE_ARRAY,
    NODE_STRING,
    NODE_NUMBER,
    NODE_BOOL,
    NODE_NULL
};

// Forward declaration
struct ASTNode;

// Type aliases for readability
typedef std::vector<ASTNode*> NodeList;
typedef std::map<std::string, ASTNode*> ObjectMap;

struct TableSchema {
    std::string name;
    std::set<std::string> columns;
    bool has_parent_id;
    bool is_junction;
    
    explicit TableSchema(const std::string& n) 
        : name(n), has_parent_id(false), is_junction(false) {}
};

struct TableData {
    TableSchema schema;
    std::vector<std::map<std::string, std::string>> rows;
    
    // Default constructor
    TableData() : schema("") {}
    
    // Constructor with schema
    explicit TableData(const TableSchema& s) : schema(s) {}
};

struct ASTNode {
    NodeType type;
    
    // Value storage as a union to save memory
    union {
        double num_val;
        bool bool_val;
        char* str_val;
        NodeList* array_val;
        ObjectMap* object_val;
    };
    
    // Constructor and destructor
    explicit ASTNode(NodeType t);
    ~ASTNode();
    
    void print(std::ostream& os, int indent = 0) const;
    
    // Get string representation of a value
    std::string get_value_string() const;
    
    void to_relational_tables(
        std::map<std::string, TableData>& tables, 
        const std::string& parent_table = "", 
        int parent_id = -1,
        const std::string& field_name = "");
    
    // Helper method to determine object signature (unique set of keys)
    std::string get_object_signature() const;
    
    // Helper method to generate a valid table name from a field name
    static std::string create_table_name(const std::string& field_name);
};

void write_csv_files(
    const std::map<std::string, TableData>& tables, 
    const std::string& out_dir);

#endif

