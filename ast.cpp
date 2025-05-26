#include "ast.hpp"
#include <cstdlib>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <sys/stat.h>



ASTNode::ASTNode(NodeType t) : type(t) {
    switch(type) {
        case NODE_ARRAY:
            array_val = new NodeList();
            break;
        case NODE_OBJECT:
            object_val = new ObjectMap();
            break;
        case NODE_STRING:
        case NODE_NUMBER:
        case NODE_BOOL:
        case NODE_NULL:
            break;
    }
}

ASTNode::~ASTNode() {
    // Clean up resources based on node type
    switch(type) {
        case NODE_STRING:
            free(str_val);
            break;
        case NODE_ARRAY:
            for(NodeList::iterator it = array_val->begin(); it != array_val->end(); ++it) {
                delete *it;
            }
            delete array_val;
            break;
        case NODE_OBJECT:
            for(ObjectMap::iterator it = object_val->begin(); it != object_val->end(); ++it) {
                delete it->second;
            }
            delete object_val;
            break;
        case NODE_NUMBER:
        case NODE_BOOL:
        case NODE_NULL:
            break;
    }
}

void ASTNode::print(std::ostream& os, int indent) const {
    std::string padding(indent * 2, ' ');
    
    switch(type) {
        case NODE_OBJECT: {
            os << padding << "OBJECT {\n";
            ObjectMap::const_iterator it;
            for (it = object_val->begin(); it != object_val->end(); ++it) {
                os << padding << "  \"" << it->first << "\": ";
                it->second->print(os, indent + 2);
            }
            os << padding << "}\n";
            break;
        }
            
        case NODE_ARRAY: {
            os << padding << "ARRAY [\n";
            for (size_t i = 0; i < array_val->size(); ++i) {
                os << padding << "  [" << i << "]: ";
                (*array_val)[i]->print(os, indent + 2);
            }
            os << padding << "]\n";
            break;
        }
            
        case NODE_STRING:
            os << "STRING \"" << str_val << "\"\n";
            break;
            
        case NODE_NUMBER:
            os << "NUMBER " << num_val << "\n";
            break;
            
        case NODE_BOOL:
            os << "BOOL " << (bool_val ? "true" : "false") << "\n";
            break;
            
        case NODE_NULL:
            os << "NULL\n";
            break;
    }
}

std::string ASTNode::get_value_string() const {
    switch(type) {
        case NODE_STRING:
            return std::string(str_val);
            
        case NODE_NUMBER: {
            std::ostringstream oss;
            oss << num_val;
            return oss.str();
        }
            
        case NODE_BOOL:
            return bool_val ? "true" : "false";
            
        case NODE_NULL:
            return "";
            
        case NODE_OBJECT:
        case NODE_ARRAY:
            return "<complex>";
    }
    
    return "<unknown>";
}

std::string ASTNode::get_object_signature() const {
    if (type != NODE_OBJECT) {
        return "";
    }
    
    std::vector<std::string> keys;
    keys.reserve(object_val->size());
    
    ObjectMap::const_iterator it;
    for (it = object_val->begin(); it != object_val->end(); ++it) {
        keys.push_back(it->first);
    }
    
    std::sort(keys.begin(), keys.end());
    
    std::ostringstream sig;
    for (size_t i = 0; i < keys.size(); ++i) {
        sig << keys[i] << ",";
    }
    
    return sig.str();
}

std::string ASTNode::create_table_name(const std::string& field_name) {
    if (field_name.empty()) {
        return "root";
    }
    
    std::string name = field_name;
    
    // Replace invalid characters for table names
    for (size_t i = 0; i < name.size(); ++i) {
        if (!std::isalnum(name[i])) {
            name[i] = '_';
        }
    }
    
    // Ensure it starts with a letter
    if (!std::isalpha(name[0])) {
        name = "t_" + name;
    }
    
    return name;
}

void ASTNode::to_relational_tables(
    std::map<std::string, TableData>& tables,
    const std::string& parent_table,
    int parent_id,
    const std::string& field_name
) {
    // Generate a table name based on the field name
    std::string table_name = create_table_name(field_name);
    
    switch(type) {
        case NODE_OBJECT: {
            // R1: Object → table row: Objects with same keys go in one table
            std::string signature = get_object_signature();
            
            // Create a map for this row
            std::map<std::string, std::string> row;
            
            // Add the ID field
            static int next_id = 1;
            int this_id = next_id++;
            row["id"] = std::to_string(this_id);
            
            // Add the parent ID if needed
            if (!parent_table.empty()) {
                row[parent_table + "_id"] = std::to_string(parent_id);
            }
            
            // Check if this table schema already exists
            std::map<std::string, TableData>::iterator table_it = tables.find(table_name);
            bool table_exists = (table_it != tables.end());
            
            // Create schema if needed
            if (!table_exists) {
                TableSchema schema(table_name);
                schema.columns.insert("id");
                
                if (!parent_table.empty()) {
                    schema.columns.insert(parent_table + "_id");
                    schema.has_parent_id = true;
                }
                
                // Add fields from the object
                ObjectMap::const_iterator field_it;
                for (field_it = object_val->begin(); field_it != object_val->end(); ++field_it) {
                    if (field_it->second->type != NODE_OBJECT && field_it->second->type != NODE_ARRAY) {
                        schema.columns.insert(field_it->first);
                    }
                }
                
                tables[table_name] = TableData(schema);
            }
            
            // Process each field in the object
            ObjectMap::const_iterator field_it;
            for (field_it = object_val->begin(); field_it != object_val->end(); ++field_it) {
                const std::string& key = field_it->first;
                ASTNode* value = field_it->second;
                
                if (value->type == NODE_OBJECT || value->type == NODE_ARRAY) {
                    // Complex types are handled recursively
                    value->to_relational_tables(tables, table_name, this_id, key);
                } else {
                    // Add scalar values directly to the row
                    row[key] = value->get_value_string();
                }
            }
            
            // Add the row to the table
            tables[table_name].rows.push_back(row);
            break;
        }
        
        case NODE_ARRAY: {
            if (array_val->empty()) {
                break;  // Skip empty arrays
            }
            
            // Check array elements to see if this is an array of objects
            bool is_object_array = true;
            for (size_t i = 0; i < array_val->size(); ++i) {
                if ((*array_val)[i]->type != NODE_OBJECT) {
                    is_object_array = false;
                    break;
                }
            }
            
            if (is_object_array) {
                // R2: Array of objects → each becomes a row in child table
                for (size_t i = 0; i < array_val->size(); ++i) {
                    (*array_val)[i]->to_relational_tables(
                        tables, parent_table, parent_id, field_name);
                }
            } else {
                // R3: Array of scalars → junction table with parent_id, index, value
                std::string junction_table_name = parent_table + "_" + field_name;
                junction_table_name = create_table_name(junction_table_name);
                
                // Check if this table schema already exists
                std::map<std::string, TableData>::iterator table_it = tables.find(junction_table_name);
                bool table_exists = (table_it != tables.end());
                
                if (!table_exists) {
                    // Create a new junction table schema
                    TableSchema schema(junction_table_name);
                    schema.columns.insert("id");
                    schema.columns.insert("parent_id");
                    schema.columns.insert("index");
                    schema.columns.insert("value");
                    schema.has_parent_id = true;
                    schema.is_junction = true;
                    
                    tables[junction_table_name] = TableData(schema);
                }
                
                // Add rows for each element
                static int next_junction_id = 10000;  // Starting from a different range
                
                for (size_t i = 0; i < array_val->size(); ++i) {
                    std::map<std::string, std::string> row;
                    
                    row["id"] = std::to_string(next_junction_id++);
                    row["parent_id"] = std::to_string(parent_id);
                    row["index"] = std::to_string(i);
                    row["value"] = (*array_val)[i]->get_value_string();
                    
                    tables[junction_table_name].rows.push_back(row);
                }
            }
            break;
        }
        
        case NODE_STRING:
        case NODE_NUMBER:
        case NODE_BOOL:
        case NODE_NULL:
            break;
    }
}

void create_directory(const std::string& dir) {
#ifdef _WIN32
    mkdir(dir.c_str());
#else
    mkdir(dir.c_str(), 0777);
#endif
}

// Escapes a string for CSV format (handles quoting and escaping)
std::string escape_csv_value(const std::string& value) {
    // Check if we need to quote this value
    bool needs_quoting = 
        value.find(',') != std::string::npos || 
        value.find('"') != std::string::npos || 
        value.find('\n') != std::string::npos;
    
    if (!needs_quoting) {
        return value;
    }
    
    // Escape quotes by doubling them
    std::string escaped_value;
    escaped_value.reserve(value.size() + 10);
    
    escaped_value.push_back('"');
    
    for (size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        if (c == '"') {
            escaped_value.push_back('"');
        }
        escaped_value.push_back(c);
    }
    
    escaped_value.push_back('"');
    return escaped_value;
}

void write_csv_files(const std::map<std::string, TableData>& tables, const std::string& out_dir) {
    // Create the output directory if it doesn't exist
    create_directory(out_dir);
    
    // Write each table to a CSV file
    std::map<std::string, TableData>::const_iterator table_it;
    for (table_it = tables.begin(); table_it != tables.end(); ++table_it) {
        const std::string& table_name = table_it->first;
        const TableData& table_data = table_it->second;
        
        std::string filename = out_dir + "/" + table_name + ".csv";
        std::ofstream file(filename.c_str());
        
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << " for writing" << std::endl;
            continue;
        }
        
        // Write header row
        {
            bool first = true;
            std::set<std::string>::const_iterator col_it;
            for (col_it = table_data.schema.columns.begin(); 
                 col_it != table_data.schema.columns.end(); 
                 ++col_it) {
                if (!first) file << ",";
                file << *col_it;
                first = false;
            }
            file << std::endl;
        }
        
        // Write data rows
        for (size_t row_idx = 0; row_idx < table_data.rows.size(); ++row_idx) {
            const std::map<std::string, std::string>& row = table_data.rows[row_idx];
            bool first = true;
            
            std::set<std::string>::const_iterator col_it;
            for (col_it = table_data.schema.columns.begin(); 
                 col_it != table_data.schema.columns.end(); 
                 ++col_it) {
                
                if (!first) file << ",";
                
                // Find the value for this column
                std::map<std::string, std::string>::const_iterator val_it = row.find(*col_it);
                if (val_it != row.end()) {
                    file << escape_csv_value(val_it->second);
                }
                first = false;
            }
            file << std::endl;
        }
        
        file.close();
    }
}

