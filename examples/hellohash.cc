#include <stdio.h>
#include <functional>
#include <iostream>
#include <string>

#include <libcuckoo/cuckoohash_map.hh>

int main() {
    cuckoohash_map<int, std::string> Table;

    for (int i = 0; i < 100; i++) {
        Table[i] = "hello"+std::to_string(i);
    }

    for (int i = 0; i < 101; i++) {
        std::string out;

        if (Table.find(i, out)) {
            std::cout << i << "  " << out << std::endl;
        } else {
            std::cout << i << "  NOT FOUND" << std::endl;
        }
    }
}
