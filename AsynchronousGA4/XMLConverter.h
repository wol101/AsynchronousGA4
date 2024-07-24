/*
 *  XMLConverter.h
 *  GA
 *
 *  Created by Bill Sellers on Fri Dec 12 2003.
 *  Copyright (c) 2003 Bill Sellers. All rights reserved.
 *
 */

#ifndef XMLConverter_h
#define XMLConverter_h

#include <vector>
#include <string>

namespace pkpy { class VM; }

class Genome;
class DataFile;
class ExpressionParser;

class XMLConverter
{
public:
    XMLConverter();
    virtual ~XMLConverter();

    int LoadBaseXMLFile(const std::string &filename);
    int LoadBaseXMLString(std::string_view data);
    int ApplyGenome(const std::vector<double> &genomeData);
    void GetFormattedXML(std::string *formattedXML);

    const std::string &BaseXMLString() const;

    void Clear();

    double eval(const std::string &expression);
    int exec(const std::string &expression);

private:

    std::string m_baseXMLString;
    std::vector<std::string> m_textComponents;
    std::vector<std::string> m_parserText;
    std::vector<double> m_substitutionValues;
    size_t m_textComponentsSize = 0;
    pkpy::VM *m_pythonVM = nullptr;
};


#endif
