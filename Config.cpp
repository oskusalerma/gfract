#include "Config.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <list>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include "Exception.h"
#include "misc.h"

Config::Config()
{
}

void Config::loadFromFile(const std::string& filename)
{
    sections.clear();

    FILE* fp = fopen(filename.c_str(), "rt");

    if (!fp)
    {
        throw GfractException("Can't open file '" + filename + "': " +
            strerror(errno));
    }

    gf_a(fseek(fp, 0, SEEK_END) == 0);

    long len = ftell(fp);
    gf_a(len >= 0);

    if (len == 0)
    {
        gf_a(fclose(fp) == 0);

        return;
    }

    rewind(fp);

    std::auto_ptr<char> dataPtr(new char[len + 1]);
    char* data = dataPtr.get();

    size_t ret = fread(data, 1, len, fp);
    data[ret] = '\0';

    gf_a(fclose(fp) == 0);

    loadFromMemory(data);
}

void Config::saveToFile(const std::string& filename)
{
    std::string data;
    size_t ret = 0;

    saveToMemory(&data);

    FILE* fp = fopen(filename.c_str(), "wt");

    if (!fp)
    {
        goto err;
    }

    ret = fwrite(data.data(), 1, data.size(), fp);
    if (ret != data.size())
    {
        goto err;
    }

    if (fclose(fp) != 0)
    {
        goto err;
    }

    return;

err:
    throw GfractException("Can't write file '" + filename + "': " +
        strerror(errno));
}

void Config::loadFromMemory(const std::string& str)
{
    using namespace boost;

    // [section]
    static const regex reSection("\\[([a-zA-Z0-9_-]+)\\]");

    // key=val
    static const regex reValue("([a-zA-Z0-9_-]+)\\s*=\\s*(.*)");

    std::list<std::string> lines;
    SectionPtr section;
    std::string sectionName;
    smatch matches;
    int lineNr = 1;

    split(lines, str, is_any_of("\n\r"));

    for (std::list<std::string>::iterator it = lines.begin();
         it != lines.end(); ++it, ++lineNr)
    {
        std::string line = trim_copy_if(*it, is_any_of(" \t\v\f"));

        if (line.empty() || (line[0] == '#'))
        {
            continue;
        }

        if (regex_match(line, matches, reValue))
        {
            if (!section)
            {
                throw GfractException("line " +
                    lexical_cast<std::string>(lineNr) + ": no active section");
            }

            section->setStr(matches[1], matches[2]);
        }
        else if (regex_match(line, matches, reSection))
        {
            addSection(section, sectionName);
            sectionName = matches[1];
            section = SectionPtr(new Section());
        }
        else
        {
            throw GfractException("line " +
                lexical_cast<std::string>(lineNr) + ": unknown syntax");
        }
    }

    addSection(section, sectionName);
}

void Config::saveToMemory(std::string* str)
{
    std::string tmp;

    *str = "";

    for (sections_t::iterator it = sections.begin(); it != sections.end();
         ++it)
    {
        it->second->saveToMemory(&tmp);

        *str += "[" + it->first + "]\n" + tmp + "\n";
    }
}

std::string Config::getStr(const std::string& section,
    const std::string& key, const std::string& defaultVal)
{
    bool found;

    std::string res = getStr(section, key, &found);

    if (!found)
    {
        res = defaultVal;
    }

    return res;
}

void Config::setStr(const std::string& section, const std::string& key,
    const std::string& value)
{
    sections_t::iterator it = sections.find(section);

    if (it == sections.end())
    {
        it = sections.insert(std::make_pair(section, new Section())).first;
    }

    it->second->setStr(key, value);
}

int Config::getInt(const std::string& section, const std::string& key,
    int defaultVal)
{
    bool found;

    std::string res = getStr(section, key, &found);

    if (found)
    {
        try
        {
            return boost::lexical_cast<int>(res);
        }
        catch (boost::bad_lexical_cast& e)
        {
            throw GfractException("Invalid integer value '" + res + "' for" +
                " config key " + section + "/" + key);
        }
    }
    else
    {
        return defaultVal;
    }
}

void Config::setInt(const std::string& section, const std::string& key,
    int value)
{
    setStr(section, key, boost::lexical_cast<std::string>(value));
}

std::string Config::getStr(const std::string& section, const std::string& key,
    bool* found)
{
    std::string res;
    bool myFound = false;

    sections_t::iterator it = sections.find(section);

    if (it != sections.end())
    {
        std::string value = it->second->getStr(key, &myFound);

        if (myFound)
        {
            res = value;
        }
    }

    if (found)
    {
        *found = myFound;

        return res;
    }

    if (myFound)
    {
        return res;
    }
    else
    {
        throw GfractException("Config value '" + section + "/" + key +
            "' not found");
    }
}

void Config::addSection(SectionPtr ptr, const std::string& sectionName)
{
    if (ptr)
    {
        sections[sectionName] = ptr;
    }
}

void Config::print(void)
{
    std::string s;

    saveToMemory(&s);
    printf("%s", s.c_str());
}

Config::Section::Section()
{
}

void Config::Section::saveToMemory(std::string* str)
{
    *str = "";

    for (values_t::iterator it = values.begin(); it != values.end(); ++it)
    {
        *str += it->first + "=" + it->second + "\n";
    }
}

std::string Config::Section::getStr(const std::string& key, bool* found)
{
    values_t::iterator it = values.find(key);

    if (it != values.end())
    {
        *found = true;

        return it->second;
    }
    else
    {
        *found = false;

        return "";
    }
}

void Config::Section::setStr(const std::string& key, const std::string& value)
{
    values[key] = value;
}
