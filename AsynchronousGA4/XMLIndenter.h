#ifndef XMLINDENTER_H
#define XMLINDENTER_H

#include <string>
#include <vector>

class XMLIndenter
{
public:

    // ---------------------------------------------------------------------------
    // Error codes
    // ---------------------------------------------------------------------------
    enum class XmlError
    {
        Ok = 0,
        UnterminatedTag,
        UnterminatedAttributeValue,
        MissingAttributeEquals,
        MissingAttributeValue,
        MissingAttributeName,
        UnterminatedComment,
        UnterminatedCData,
        UnterminatedDoctype,
        UnterminatedProcessingInstruction,
    };
    static const char* xmlErrorMessage(XmlError e)
    {
        switch (e)
        {
        case XmlError::Ok:                                return "OK";
        case XmlError::UnterminatedTag:                   return "Unterminated tag";
        case XmlError::UnterminatedAttributeValue:        return "Unterminated attribute value";
        case XmlError::MissingAttributeEquals:            return "Expected '=' after attribute name";
        case XmlError::MissingAttributeValue:             return "Expected quoted value for attribute";
        case XmlError::MissingAttributeName:              return "Expected attribute name";
        case XmlError::UnterminatedComment:               return "Unterminated comment";
        case XmlError::UnterminatedCData:                 return "Unterminated CDATA section";
        case XmlError::UnterminatedDoctype:               return "Unterminated DOCTYPE declaration";
        case XmlError::UnterminatedProcessingInstruction: return "Unterminated processing instruction";
        }
        return "Unknown error";
    }

    static XmlError reformatXml(const std::string& xml, std::string& result);

private:

    // ---------------------------------------------------------------------------
    // Internal types
    // ---------------------------------------------------------------------------
    struct Attribute
    {
        std::string name;
        std::string value;
        char quote; // '"' or '\''
    };

    static size_t skipWhitespace(const std::string& s, size_t pos);
    static std::string makeIndent(int depth, const std::string& unit);
    static XmlError parseAttributes(const std::string& s, std::vector<Attribute>& attrs);
    static XmlError formatElementTag(const std::string& interior, const std::string& indent, std::string& out);

};

#endif // XMLINDENTER_H
