#include "XMLIndenter.h"

#include <algorithm>
#include <stdexcept>
#include <sstream>


// Skip whitespace characters, return new position
size_t XMLIndenter::skipWhitespace(const std::string& s, size_t pos)
{
    while (pos < s.size() && std::isspace((unsigned char)s[pos]))
        ++pos;
    return pos;
}

// Parse attributes from a tag's interior string (after the tag name).
// Handles both single- and double-quoted values.
std::vector<XMLIndenter::Attribute> XMLIndenter::parseAttributes(const std::string& s)
{
    std::vector<Attribute> attrs;
    size_t pos = 0;

    while (true)
    {
        pos = skipWhitespace(s, pos);
        if (pos >= s.size()) break;

        // Stop at '/' or '>' (shouldn't appear here, but be safe)
        if (s[pos] == '/' || s[pos] == '>') break;

        // Read attribute name
        size_t nameStart = pos;
        while (pos < s.size() && s[pos] != '=' && !std::isspace((unsigned char)s[pos]) && s[pos] != '/' && s[pos] != '>') ++pos;

        if (pos == nameStart)
            throw std::runtime_error("Expected attribute name at position " + std::to_string(pos));

        std::string name = s.substr(nameStart, pos - nameStart);
        pos = skipWhitespace(s, pos);

        if (pos >= s.size() || s[pos] != '=')
            throw std::runtime_error("Expected '=' after attribute name '" + name + "'");
        ++pos; // consume '='
        pos = skipWhitespace(s, pos);

        if (pos >= s.size() || (s[pos] != '"' && s[pos] != '\''))
            throw std::runtime_error("Expected quoted value for attribute '" + name + "'");

        char quote = s[pos++];
        size_t valueStart = pos;
        while (pos < s.size() && s[pos] != quote)
            ++pos;
        if (pos >= s.size())
            throw std::runtime_error("Unterminated attribute value for '" + name + "'");

        std::string value = s.substr(valueStart, pos - valueStart);
        ++pos; // consume closing quote

        attrs.push_back({name, value, quote});
    }

    std::sort(attrs.begin(), attrs.end(), [](const Attribute& a, const Attribute& b) { return a.name < b.name; });
    return attrs;
}

// Format a tag with its attributes sorted and each on its own indented line.
// `interior` is everything between '<' and '>' (excluding those chars).
// `indent` is the indentation string.
std::string XMLIndenter::formatTag(const std::string& interior, const std::string& indent)
{
    // Trim leading/trailing whitespace from interior
    size_t start = skipWhitespace(interior, 0);
    size_t end = interior.size();
    while (end > start && std::isspace((unsigned char)interior[end - 1])) --end;
    std::string trimmed = interior.substr(start, end - start);

    bool isClosing  = (!trimmed.empty() && trimmed[0] == '/');
    bool isSelfClose = (!trimmed.empty() && trimmed.back() == '/');

    // Strip leading '/' for closing tags
    if (isClosing) trimmed = trimmed.substr(1);
    // Strip trailing '/' for self-closing tags
    if (isSelfClose && !isClosing) trimmed = trimmed.substr(0, trimmed.size() - 1);

    // Split tag name from the rest
    size_t nameEnd = 0;
    while (nameEnd < trimmed.size() && !std::isspace((unsigned char)trimmed[nameEnd])) ++nameEnd;

    std::string tagName = trimmed.substr(0, nameEnd);
    std::string attrsPart = trimmed.substr(nameEnd);

    std::vector<Attribute> attrs = parseAttributes(attrsPart);

    std::ostringstream out;

    if (isClosing)
    {
        out << indent << "</" << tagName << ">";
        return out.str();
    }

    if (attrs.empty())
    {
        out << indent << "<" << tagName;
        if (isSelfClose) out << "/";
        out << ">";
    }
    else
    {
        out << indent << "<" << tagName << "\n";
        for (size_t i = 0; i < attrs.size(); ++i)
        {
            out << indent << "    "
                << attrs[i].name << "="
                << attrs[i].quote << attrs[i].value << attrs[i].quote;
            if (i + 1 < attrs.size()) out << "\n";
        }
        if (isSelfClose) out << "\n" << indent << "/>";
        else             out << ">";
    }

    return out.str();
}

// Main entry point: reformat XML string with sorted, per-line attributes
// and indentation that reflects nesting depth.
std::string XMLIndenter::reformatXml(const std::string& xml)
{
    const std::string indentUnit = "    "; // 4 spaces per level
    std::ostringstream out;
    int depth = 0;
    size_t pos = 0;
    const size_t len = xml.size();

    while (pos < len)
    {
        // Handle text nodes (content between tags)
        if (xml[pos] != '<')
        {
            size_t textStart = pos;
            while (pos < len && xml[pos] != '<') ++pos;
            std::string text = xml.substr(textStart, pos - textStart);
            // Only emit non-whitespace text nodes
            std::string trimText = text;
            size_t ts = skipWhitespace(trimText, 0);
            if (ts < trimText.size())
            {
                // Trim leading/trailing whitespace
                size_t te = trimText.size();
                while (te > ts && std::isspace((unsigned char)trimText[te-1])) --te;
                std::string indent;
                for (int i = 0; i < depth; ++i) indent += indentUnit;
                out << indent << trimText.substr(ts, te - ts) << "\n";
            }
            continue;
        }

        // We are at '<'
        ++pos; // consume '<'

        // Handle XML declaration / processing instructions / comments / CDATA verbatim
        if (pos < len && xml[pos] == '?')
        {
            // Processing instruction: <?...?>
            size_t piStart = pos - 1;
            while (pos + 1 < len && !(xml[pos] == '?' && xml[pos+1] == '>')) ++pos;
            pos += 2; // consume '?>'
            std::string indent;
            for (int i = 0; i < depth; ++i) indent += indentUnit;
            out << indent << xml.substr(piStart, pos - piStart) << "\n";
            continue;
        }

        if (pos + 2 < len && xml[pos] == '!' && xml[pos+1] == '-' && xml[pos+2] == '-')
        {
            // Comment: <!--...-->
            size_t cmtStart = pos - 1;
            pos += 3; // skip '!--'
            while (pos + 2 < len && !(xml[pos] == '-' && xml[pos+1] == '-' && xml[pos+2] == '>'))
                ++pos;
            pos += 3; // consume '-->'
            std::string indent;
            for (int i = 0; i < depth; ++i) indent += indentUnit;
            out << indent << xml.substr(cmtStart, pos - cmtStart) << "\n";
            continue;
        }

        if (pos + 7 < len && xml.substr(pos, 8) == "![CDATA[")
        {
            // CDATA section
            size_t cdStart = pos - 1;
            pos += 8;
            while (pos + 2 < len && !(xml[pos] == ']' && xml[pos+1] == ']' && xml[pos+2] == '>'))
                ++pos;
            pos += 3;
            std::string indent;
            for (int i = 0; i < depth; ++i) indent += indentUnit;
            out << indent << xml.substr(cdStart, pos - cdStart) << "\n";
            continue;
        }

        // Regular tag: read until '>'
        // Must handle '>' inside quoted attribute values
        size_t tagStart = pos;
        bool inQuote = false;
        char quoteChar = 0;
        while (pos < len)
        {
            char c = xml[pos];
            if (inQuote) {
                if (c == quoteChar) inQuote = false;
            } else {
                if (c == '"' || c == '\'') { inQuote = true; quoteChar = c; }
                else if (c == '>') break;
            }
            ++pos;
        }
        if (pos >= len)
            throw std::runtime_error("Unterminated tag starting at position " + std::to_string(tagStart));

        std::string interior = xml.substr(tagStart, pos - tagStart);
        ++pos; // consume '>'

        // Determine tag kind before formatting, to adjust depth
        std::string trimInterior = interior.substr(skipWhitespace(interior, 0));
        bool isClosing   = (!trimInterior.empty() && trimInterior[0] == '/');
        bool isSelfClose = (!trimInterior.empty() && trimInterior.back() == '/');

        if (isClosing) --depth;

        std::string indent;
        for (int i = 0; i < depth; ++i) indent += indentUnit;

        out << formatTag(interior, indent) << "\n";

        if (!isClosing && !isSelfClose) ++depth;
    }

    // Remove trailing newline
    std::string result = out.str();
    if (!result.empty() && result.back() == '\n')
        result.pop_back();
    return result;
}
