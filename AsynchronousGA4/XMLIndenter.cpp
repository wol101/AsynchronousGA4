#include "XMLIndenter.h"

#include <algorithm>
#include <sstream>
#include <cctype>


// ---------------------------------------------------------------------------
// Error codes
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
size_t XMLIndenter::skipWhitespace(const std::string& s, size_t pos)
{
    while (pos < s.size() && std::isspace((unsigned char)s[pos]))
        ++pos;
    return pos;
}

std::string XMLIndenter::makeIndent(int depth, const std::string& unit)
{
    std::string indent;
    indent.reserve(depth * unit.size());
    for (int i = 0; i < depth; ++i) indent += unit;
    return indent;
}

// ---------------------------------------------------------------------------
// Parse attributes out of a tag's interior (everything after the tag name).
// Returns XmlError::Ok on success; populates `attrs` sorted alphabetically.
// ---------------------------------------------------------------------------
XMLIndenter::XmlError XMLIndenter::parseAttributes(const std::string& s, std::vector<Attribute>& attrs)
{
    size_t pos = 0;

    while (true) {
        pos = skipWhitespace(s, pos);
        if (pos >= s.size() || s[pos] == '/' || s[pos] == '>') break;

        // Attribute name
        size_t nameStart = pos;
        while (pos < s.size()
               && s[pos] != '='
               && !std::isspace((unsigned char)s[pos])
               && s[pos] != '/'
               && s[pos] != '>')
            ++pos;

        if (pos == nameStart) return XmlError::MissingAttributeName;

        std::string name = s.substr(nameStart, pos - nameStart);
        pos = skipWhitespace(s, pos);

        if (pos >= s.size() || s[pos] != '=')
            return XmlError::MissingAttributeEquals;
        ++pos; // consume '='
        pos = skipWhitespace(s, pos);

        if (pos >= s.size() || (s[pos] != '"' && s[pos] != '\''))
            return XmlError::MissingAttributeValue;

        char quote = s[pos++];
        size_t valueStart = pos;
        while (pos < s.size() && s[pos] != quote) ++pos;
        if (pos >= s.size()) return XmlError::UnterminatedAttributeValue;

        attrs.push_back({name, s.substr(valueStart, pos - valueStart), quote});
        ++pos; // consume closing quote
    }

    std::sort(attrs.begin(), attrs.end(),
              [](const Attribute& a, const Attribute& b) { return a.name < b.name; });
    return XmlError::Ok;
}

// ---------------------------------------------------------------------------
// Format a regular element tag (<tag attrs/> or <tag attrs> or </tag>).
// `interior` is everything between '<' and '>' exclusive.
// ---------------------------------------------------------------------------
XMLIndenter::XmlError XMLIndenter::formatElementTag(const std::string& interior, const std::string& indent, std::string& out)
{
    size_t start = skipWhitespace(interior, 0);
    size_t end = interior.size();
    while (end > start && std::isspace((unsigned char)interior[end - 1])) --end;
    std::string trimmed = interior.substr(start, end - start);

    bool isClosing   = (!trimmed.empty() && trimmed[0] == '/');
    bool isSelfClose = (!trimmed.empty() && trimmed.back() == '/');

    if (isClosing)   trimmed = trimmed.substr(1);            // strip leading '/'
    if (isSelfClose && !isClosing)
        trimmed = trimmed.substr(0, trimmed.size() - 1);     // strip trailing '/'

    size_t nameEnd = 0;
    while (nameEnd < trimmed.size() && !std::isspace((unsigned char)trimmed[nameEnd]))
        ++nameEnd;

    std::string tagName  = trimmed.substr(0, nameEnd);
    std::string attrsPart = trimmed.substr(nameEnd);

    std::vector<Attribute> attrs;
    if (XmlError err = parseAttributes(attrsPart, attrs); err != XmlError::Ok)
        return err;

    std::ostringstream oss;

    if (isClosing) {
        oss << indent << "</" << tagName << ">";
    } else if (attrs.empty()) {
        oss << indent << "<" << tagName << (isSelfClose ? "/>" : ">");
    } else {
        oss << indent << "<" << tagName << "\n";
        for (size_t i = 0; i < attrs.size(); ++i) {
            oss << indent << "    "
                << attrs[i].name << "="
                << attrs[i].quote << attrs[i].value << attrs[i].quote;
            if (i + 1 < attrs.size()) oss << "\n";
        }
        oss << (isSelfClose ? "\n" + indent + "/>" : ">");
    }

    out = oss.str();
    return XmlError::Ok;
}

// ---------------------------------------------------------------------------
// Main public function
//
//   XmlError err = reformatXml(input, output);
//   if (err != XmlError::Ok) { /* handle */ }
// ---------------------------------------------------------------------------
XMLIndenter::XmlError XMLIndenter::reformatXml(const std::string& xml, std::string& result) {
    const std::string indentUnit = "    "; // 4 spaces per level
    std::ostringstream out;
    int    depth = 0;
    size_t pos   = 0;
    const  size_t len = xml.size();

    while (pos < len) {
        // ----------------------------------------------------------------
        // Text node
        // ----------------------------------------------------------------
        if (xml[pos] != '<') {
            size_t textStart = pos;
            while (pos < len && xml[pos] != '<') ++pos;

            const std::string text = xml.substr(textStart, pos - textStart);
            size_t ts = skipWhitespace(text, 0);
            if (ts < text.size()) {
                size_t te = text.size();
                while (te > ts && std::isspace((unsigned char)text[te - 1])) --te;
                out << makeIndent(depth, indentUnit)
                    << text.substr(ts, te - ts) << "\n";
            }
            continue;
        }

        ++pos; // consume '<'

        // ----------------------------------------------------------------
        // Processing instruction  <?...?>
        // ----------------------------------------------------------------
        if (pos < len && xml[pos] == '?') {
            const size_t piStart = pos - 1;
            ++pos; // skip '?'
            while (pos + 1 < len && !(xml[pos] == '?' && xml[pos + 1] == '>'))
                ++pos;
            if (pos + 1 >= len) return XmlError::UnterminatedProcessingInstruction;
            pos += 2; // consume '?>'
            out << makeIndent(depth, indentUnit)
                << xml.substr(piStart, pos - piStart) << "\n";
            continue;
        }

        // ----------------------------------------------------------------
        // Declarations / special nodes starting with '!'
        // ----------------------------------------------------------------
        if (pos < len && xml[pos] == '!') {
            ++pos; // consume '!'

            // -- Comment  <!--...-->
            if (pos + 1 < len && xml[pos] == '-' && xml[pos + 1] == '-') {
                const size_t cmtStart = pos - 2; // back to '<'
                pos += 2; // skip '--'
                while (pos + 2 < len &&
                       !(xml[pos] == '-' && xml[pos+1] == '-' && xml[pos+2] == '>'))
                    ++pos;
                if (pos + 2 >= len) return XmlError::UnterminatedComment;
                pos += 3; // consume '-->'
                out << makeIndent(depth, indentUnit)
                    << xml.substr(cmtStart, pos - cmtStart) << "\n";
                continue;
            }

            // -- CDATA  <![CDATA[...]]>
            if (pos + 6 < len && xml.substr(pos, 7) == "[CDATA[") {
                const size_t cdStart = pos - 2;
                pos += 7; // skip '[CDATA['
                while (pos + 2 < len &&
                       !(xml[pos] == ']' && xml[pos+1] == ']' && xml[pos+2] == '>'))
                    ++pos;
                if (pos + 2 >= len) return XmlError::UnterminatedCData;
                pos += 3; // consume ']]>'
                out << makeIndent(depth, indentUnit)
                    << xml.substr(cdStart, pos - cdStart) << "\n";
                continue;
            }

            // -- DOCTYPE  <!DOCTYPE ... > or <!DOCTYPE ... [ ... ]>
            //    The internal subset [...] may itself contain '>' characters
            //    inside declaration markup, so we track bracket nesting.
            if (pos + 6 < len && xml.substr(pos, 7) == "DOCTYPE") {
                const size_t dtStart = pos - 2; // back to '<'
                pos += 7; // skip 'DOCTYPE'

                bool inInternalSubset = false;
                bool inQuote          = false;
                char quoteChar        = 0;

                while (pos < len) {
                    const char c = xml[pos];

                    if (inQuote) {
                        if (c == quoteChar) inQuote = false;
                    } else if (c == '"' || c == '\'') {
                        inQuote = true; quoteChar = c;
                    } else if (c == '[') {
                        inInternalSubset = true;
                    } else if (c == ']') {
                        inInternalSubset = false;
                    } else if (c == '>' && !inInternalSubset) {
                        break;
                    }
                    ++pos;
                }
                if (pos >= len) return XmlError::UnterminatedDoctype;
                ++pos; // consume '>'

                out << makeIndent(depth, indentUnit)
                    << xml.substr(dtStart, pos - dtStart) << "\n";
                continue;
            }

            // Unknown '!...' construct — pass through verbatim until '>'
            {
                const size_t unknownStart = pos - 2;
                while (pos < len && xml[pos] != '>') ++pos;
                if (pos >= len) return XmlError::UnterminatedTag;
                ++pos;
                out << makeIndent(depth, indentUnit)
                    << xml.substr(unknownStart, pos - unknownStart) << "\n";
                continue;
            }
        }

        // ----------------------------------------------------------------
        // Regular element tag — read interior respecting quoted '>'
        // ----------------------------------------------------------------
        {
            const size_t tagStart = pos;
            bool inQuote  = false;
            char quoteChar = 0;

            while (pos < len) {
                const char c = xml[pos];
                if (inQuote) {
                    if (c == quoteChar) inQuote = false;
                } else {
                    if (c == '"' || c == '\'') { inQuote = true; quoteChar = c; }
                    else if (c == '>')          break;
                }
                ++pos;
            }
            if (pos >= len) return XmlError::UnterminatedTag;

            const std::string interior = xml.substr(tagStart, pos - tagStart);
            ++pos; // consume '>'

            // Determine closing/self-closing before adjusting depth
            const size_t ts = skipWhitespace(interior, 0);
            const bool isClosing   = (ts < interior.size() && interior[ts] == '/');
            const bool isSelfClose = (!interior.empty() && interior.back() == '/');

            if (isClosing) --depth;

            std::string formatted;
            if (XmlError err = formatElementTag(interior,
                                                makeIndent(depth, indentUnit),
                                                formatted);
                err != XmlError::Ok)
                return err;

            out << formatted << "\n";

            if (!isClosing && !isSelfClose) ++depth;
        }
    }

    result = out.str();
    if (!result.empty() && result.back() == '\n')
        result.pop_back();
    return XmlError::Ok;
}
