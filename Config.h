#ifndef GFRACT_CONFIG_H
#define GFRACT_CONFIG_H

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

/** Stores structured config information. Aims to be compatible with
    Python's ConfigParser in its syntax. Sample file:

[section1]
var1=value1
var2=value2

[section42]
var1=valueBlaa

All functions throw a GfractException on errors.
*/
class Config : boost::noncopyable
{
public:
    Config();

    void loadFromFile(const std::string& filename);

    void saveToFile(const std::string& filename);


    void loadFromMemory(const std::string& str);

    void saveToMemory(std::string* str);

    // get/set functions for all of the data types supported

    std::string getStr(const std::string& section,
        const std::string& key, const std::string& defaultVal);

    void setStr(const std::string& section, const std::string& key,
        const std::string& value);

    int getInt(const std::string& section, const std::string& key,
        int defaultVal);

    void setInt(const std::string& section, const std::string& key,
        int value);

    // debugging helper, prints results of saveToMemory to stdout
    void print();

private:
    class Section
    {
    public:
        Section();

        void saveToMemory(std::string* str);

        // get a value. if it was found, *found is set to true, otherwise
        // to false, in which case the return value should not be used.
        std::string getStr(const std::string& key, bool* found);

        void setStr(const std::string& key, const std::string& value);

    private:
        typedef std::map<std::string, std::string> values_t;

        values_t values;
    };

    // get the given string. if it does not exist and found is non-NULL,
    // that is set to false and an arbitrary value returned, otherwise a
    // GfractException is thrown.
    std::string getStr(const std::string& section, const std::string& key,
        bool* found);

    typedef boost::shared_ptr<Section> SectionPtr;
    typedef std::map<std::string, SectionPtr> sections_t;

    sections_t sections;

    // if ptr is not NULL, add the given section
    void addSection(SectionPtr ptr, const std::string& sectionName);
};

#endif
