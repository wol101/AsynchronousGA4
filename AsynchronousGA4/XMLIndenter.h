#ifndef XMLINDENTER_H
#define XMLINDENTER_H

#include <string>
#include <vector>

class XMLIndenter
{
public:
    static std::string reformatXml(const std::string& xml);

private:
    // Represents a single parsed attribute: name="value"
    struct Attribute
    {
        std::string name;
        std::string value;
        char quote; // ' or "
    };

    static size_t skipWhitespace(const std::string& s, size_t pos);
    static std::vector<Attribute> parseAttributes(const std::string& s);
    static std::string formatTag(const std::string& interior, const std::string& indent);

};

#endif // XMLINDENTER_H
