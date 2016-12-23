/* We demonstrate how to nest hash tables within one another, to store
 * unstructured data, kind of like JSON. There's still the limitation that it's
 * statically typed. */

#include <iostream>
#include <string>
#include <memory>
#include <utility>

#include <boost/foreach.hpp>
#include <boost/move/move.hpp>
#include <boost/move/unique_ptr.hpp>

#include "../src/cuckoohash_map.hh"

typedef cuckoohash_map<std::string, std::string> InnerTable;
typedef cuckoohash_map<std::string, boost::movelib::unique_ptr<InnerTable> >
    OuterTable;

struct UpdateFn1 {
    void operator()(boost::movelib::unique_ptr<InnerTable>& innerTbl) const {
        innerTbl->insert("nickname", "jimmy");
        innerTbl->insert("pet", "dog");
        innerTbl->insert("food", "bagels");
    }
};

struct UpdateFn2 {
    void operator()(boost::movelib::unique_ptr<InnerTable>& innerTbl) const {
        innerTbl->insert("friend", "bob");
        innerTbl->insert("activity", "sleeping");
        innerTbl->insert("language", "javascript");
    }
};

int main() {
    OuterTable tbl;

    boost::movelib::unique_ptr<InnerTable> it1(new InnerTable);
    tbl.insert("bob", boost::move(it1));
    tbl.update_fn("bob", UpdateFn1());

    boost::movelib::unique_ptr<InnerTable> it2(new InnerTable);
    tbl.insert("jack", boost::move(it2));
    tbl.update_fn("jack", UpdateFn2());

    {
        OuterTable::locked_table lt = tbl.lock_table();
        BOOST_FOREACH (const OuterTable::value_type& item, lt) {
            std::cout << "Properties for " << item.first << std::endl;
            InnerTable::locked_table innerLt = item.second->lock_table();
            BOOST_FOREACH (InnerTable::value_type innerItem, innerLt) {
                std::cout << "\t" << innerItem.first << " = "
                          << innerItem.second << std::endl;
            }
        }
    }
}
