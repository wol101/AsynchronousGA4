/*
 *  XMLConverter.cc
 *  GA
 *
 *  Created by Bill Sellers on Fri Dec 12 2003.
 *  Copyright (c) 2003 Bill Sellers. All rights reserved.
 *
 */

#include "XMLConverter.h"

#include "pocketpy.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

using namespace std::string_literals;

XMLConverter::XMLConverter()
{
    m_pythonVM =  new pkpy::VM();
}

XMLConverter::~XMLConverter()
{
    if (m_pythonVM) delete m_pythonVM;
}

// load the base file for smart substitution file
int XMLConverter::LoadBaseXMLFile(const std::string &filename)
{
    std::ifstream in(filename, std::ios::binary);
    std::string str(std::istreambuf_iterator<char>{in}, {});
    LoadBaseXMLString(str);
    return 0;
}

void XMLConverter::Clear()
{
    m_textComponents.clear();
    m_parserText.clear();
    m_substitutionValues.clear();
    m_baseXMLString.clear();
}

// load the base XML for smart substitution file
int XMLConverter::LoadBaseXMLString(std::string_view data)
{
    m_textComponents.clear();
    m_parserText.clear();
    m_substitutionValues.clear();
    m_baseXMLString.assign(data);

    const char *ptr1 = m_baseXMLString.data();
    const char *ptr2 = strstr(ptr1, "{{");
    m_textComponentsSize = 0;
    while (ptr2)
    {
        std::string s(ptr1, static_cast<size_t>(ptr2 - ptr1));
        m_textComponentsSize += s.size();
        m_textComponents.push_back(std::move(s));

        ptr2 += 2;
        ptr1 = strstr(ptr2, "}}");
        if (ptr1 == nullptr)
        {
            std::cerr << "Error: could not find matching }}\n";
            exit(1);
        }
        std::string expressionParserText(ptr2, static_cast<size_t>(ptr1 - ptr2));
        m_parserText.push_back(std::move(expressionParserText));
        m_substitutionValues.push_back(0); // dummy values
        ptr1 += 2;
        ptr2 = strstr(ptr1, "{{");
    }
    std::string s(ptr1);
    m_textComponentsSize += s.size();
    m_textComponents.push_back(std::move(s));

    return 0;
}

void XMLConverter::GetFormattedXML(std::string *formattedXML)
{
    formattedXML->clear();
    formattedXML->reserve(m_textComponentsSize + 32 * m_substitutionValues.size());
    char buffer[32];
    for (size_t i = 0; i < m_substitutionValues.size(); i++)
    {
        formattedXML->append(m_textComponents[i]);
        int l = snprintf(buffer, sizeof(buffer), "%.17g", m_substitutionValues[i]);
        formattedXML->append(buffer, l);
    }
    formattedXML->append(m_textComponents[m_substitutionValues.size()]);
}

// this needs to be customised depending on how the genome interacts with
// the XML file specifying the simulation
int XMLConverter::ApplyGenome(const std::vector<double> &genomeData)
{
    // std::vector<std::string> stringList;
    // stringList.reserve(genomeData.size());
    // for (auto &&x : genomeData) { stringList.push_back(GSUtil::ToString(x)); }
    // std::string pythonString = "g=["s + pystring::join(","s, stringList) + "]"s;
    // m_pythonVM->exec(pythonString);

    pkpy::List g;
    for (size_t i =0; i < genomeData.size(); ++i) { g.push_back(pkpy::py_var(m_pythonVM, genomeData[i])); }
    pkpy::PyObject* obj = py_var(m_pythonVM, std::move(g));
    m_pythonVM->_main->attr().set(pkpy::StrName("g"), obj); // based on the pkpy_setglobal code

    for (size_t i = 0; i < m_parserText.size(); i++)
    {
        pkpy::PyObject *result = m_pythonVM->eval(m_parserText[i]);
        m_substitutionValues[i] = pkpy::py_cast<double>(m_pythonVM, result);
    }

    return 0;
}

double XMLConverter::eval(const std::string &expression)
{
    pkpy::PyObject *result = m_pythonVM->eval(expression);
    return pkpy::py_cast<double>(m_pythonVM, result);
}

int XMLConverter::exec(const std::string &expression)
{
    try
    {
        m_pythonVM->exec(expression);
    }
    catch (...)
    {
        return __LINE__;
    }
    return 0;
}

const std::string &XMLConverter::BaseXMLString() const
{
    return m_baseXMLString;
}



