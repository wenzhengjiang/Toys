//
//  Database.h
//  Generic Database
//
//  Created by Wenzheng Jiang on 28/02/2015.
//  Copyright (c) 2015 Wenzheng Jiang. All rights reserved.
//

#ifndef Generic_Database_Database_h
#define Generic_Database_Database_h

#include <iostream>
#include <vector>
#include <utility>

typedef enum {Equal, NotEqual, LessThan, GreaterThan } DBQueryOperator;
typedef enum {AllRecords, SelectedRecords } DBScope;
typedef enum {Add, Remove, Refine } DBSelectOperation;

template <typename> class Record;
template <typename T>
std::ostream& operator<<(std::ostream&, const Record<T>&);
template <typename T>
std::istream& operator>>(std::istream&, Record<T>&);
template <typename T>
class Record {
public:
    typedef T Value;
    Record():selected(false) { };
    ~Record() { };
    bool isSelected() const { return selected; }
    void setSelected(bool val) { selected = val; }
    bool matchesQuery(const std::string &attr, DBQueryOperator op,
                      const Value &want) const;
    friend std::ostream& operator<< <T> (std::ostream &os,const Record<T> &record);
    friend std::istream& operator>> <T> (std::istream&, Record<T>&);
    
private:
    bool selected;
    std::vector<std::pair<std::string,T>> data;
};

template <typename T>
bool Record<T>::matchesQuery(const std::string &attr, DBQueryOperator op, const Value &want) const {
    for (auto &p : data) {
        if (p.first == attr || attr == "*") {
            switch (op) {
                case Equal:
                    if (p.second == want)
                        return true;
                    break;
                case NotEqual:
                    if (p.second != want)
                        return true;
                    break;
                case LessThan:
                    if (p.second < want)
                        return true;
                    break;
                case GreaterThan:
                    if (p.second > want)
                        return true;
                    break;
                default:
                    break;
            }
        }
    }
    return false;
}
template <typename T>
std::ostream& operator<<(std::ostream &os,const Record<T> &record) {
    os << "{" << std::endl;
    for (auto &p : record.data) {
        os << p.first << " = " << p.second << std::endl;
    }
    os << "}" << std::endl;
    return os;
}

template <typename T>
std::istream& operator>>(std::istream &is, Record<T> &record) {
    record.data.clear();
    record.selected = false;
    
    std::string str ;
    while ((is >> str) && str != "{");
    while ((is >> str) && str != "}"){
        std::string attr = str;
        T val;
        while ((is >> str) && str != "=") {
            attr += " " + str;
        }
        is >> val;
        record.data.push_back({attr, val});
    }
    return is;
}
template <>
std::istream& operator>>(std::istream &is, Record<std::string> &record) {
    std::string str ;
    while ((is >> str) && str != "{");
    while ((is >> str) && str != "}"){
        std::string attr = str;
        std::string val;
        while ((is >> str) && str != "=") {
            attr += " " + str;
        }
        getline(is, val);
        record.data.push_back({attr, val.substr(1)}); // remove the whilespace between = and value string
    }
    return is;
}

template <typename T>
class Database {
public:
    Database() = default;
    ~Database() = default;
    // constant time
    int numRecords() const {return nrecords; }
    int numSelected() const {return nselected; }
    void write(std::ostream &os, DBScope scope) const
    {
        for (auto d : data) {
            if (scope == SelectedRecords) {
                if (d.isSelected()) {
                    os << d;
                }
            } else {
                os << d;
            }
        }
    }
    void read(std::istream &is)
    {
        deleteRecord(AllRecords);
        Record<T> r;
        while (is >> r) {
            data.push_back(r);
        }
    }
    void deleteRecord(DBScope scope)
    {
        for (auto it = data.begin(); it != data.end();) {
            if (scope == SelectedRecords) {
                if (it->isSelected()) {
                    it = data.erase(it);
                } else {
                    ++it;
                }
            } else {
                it = data.erase(it);
            }
        }
    }
    void selectAll()
    {
        for (auto &d : data) {
            d.setSelected(true);
        }
    }
    void deseleteAll()
    {
        for (auto &d : data) {
            d.setSelected(false);
        }
    }
    void select(DBSelectOperation selOp, const std::string& attr,
                DBQueryOperator op, const T& val)
    {
        for (auto &d : data) {
            if (d.matchesQuery(attr, op, val)) {
                if (selOp == Add || selOp == Refine) {
                    d.setSelected(true);
                } else if (selOp == Remove) {
                    d.setSelected(false);
                }
            } else if (selOp == Refine) {
                d.setSelected(false);
            }
        }
    }
private:
    int nrecords, nselected;
    std::vector<Record<T>> data;
};
#endif