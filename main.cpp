#include <iostream>
#include <string>
#include <cstring>
#include <map>
#include "ast.hpp"



extern int yyparse();
extern FILE* yyin;
extern ASTNode* root;

void print_usage() {
    std::cerr << "Usage: ./json2relcsv [--print-ast] [--out-dir DIR]" << std::endl;
}

int main(int argc, char** argv) {
    // Default settings
    bool print_ast = false;
    std::string out_dir = ".";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--print-ast") == 0) {
            print_ast = true;
        } 
        else if (std::strcmp(argv[i], "--out-dir") == 0) {
            if (i + 1 < argc) {
                out_dir = argv[++i];
            } else {
                std::cerr << "Error: --out-dir requires a directory path" << std::endl;
                print_usage();
                return 1;
            }
        } 
        else {
            std::cerr << "Error: Unknown option: " << argv[i] << std::endl;
            print_usage();
            return 1;
        }
    }
    
    // Use stdin as input (can be redirected from a file)
    yyin = stdin;
    
    // Parse the JSON input using the flex/bison parser
    if (yyparse() != 0) {
        std::cerr << "Error: JSON parsing failed" << std::endl;
        return 1;
    }
    
    // Verify we have a valid AST
    if (!root) {
        std::cerr << "Error: No AST root created" << std::endl;
        return 1;
    }
    
    // Print the AST if requested (for debugging)
    if (print_ast) {
        std::cout << "Abstract Syntax Tree:" << std::endl;
        root->print(std::cout);
        std::cout << std::endl;
    }
    
    // This will hold our generated tables
    std::map<std::string, TableData> tables;
    
    try {
        // Process the AST based on its type
        if (root->type == NODE_OBJECT) {
            // Single object at the root level
            root->to_relational_tables(tables, "", -1, "root");
        } 
        else if (root->type == NODE_ARRAY) {
            // Array of objects at the root level
            for (size_t i = 0; i < root->array_val->size(); ++i) {
                ASTNode* element = (*(root->array_val))[i];
                if (element->type == NODE_OBJECT) {
                    element->to_relational_tables(tables, "", -1, "root");
                }
            }
        } 
        else {
            std::cerr << "Warning: Root is neither an object nor an array" << std::endl;
        }
        
        // Write the tables to CSV files in the specified output directory
        write_csv_files(tables, out_dir);
        
        // Print summary of results
        std::cout << "Created " << tables.size() << " CSV files:" << std::endl;
        std::map<std::string, TableData>::const_iterator table_it;
        for (table_it = tables.begin(); table_it != tables.end(); ++table_it) {
            std::cout << "  " << table_it->first << ".csv: " 
                      << table_it->second.rows.size() << " rows, " 
                      << table_it->second.schema.columns.size() << " columns" 
                      << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error during CSV generation: " << e.what() << std::endl;
        delete root;
        return 1;
    }
    
    // Clean up
    delete root;
    
    return 0;
}

