#include <stdio.h>
#include <string>
#include <iostream>

#include <libcuckoo/cuckoohash_map.hh>
#include <boost/lexical_cast.hpp>

int main() {
    cuckoohash_map<int, std::string> Table;

    for (int i = 0; i < 100; i++) {
        Table[i] = "hello" + boost::lexical_cast<std::string>(i);
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
