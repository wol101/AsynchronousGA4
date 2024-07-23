/*
 *  MergeXML.cc
 *  MergeXML
 *
 *  Created by Bill Sellers on Wed Dec 17 2003.
 *  Copyright (c) 2003 Bill Sellers. All rights reserved.
 *
 */

#include "MergeXML.h"
#include "XMLContainer.h"
#include "XMLContainerList.h"
#include "MergeUtil.h"

#include "pystring.h"

#include <string.h>
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <regex>

using namespace std::string_literals;

MergeXML::MergeXML()
{
}

// read and execute the instruction file
void MergeXML::ExecuteInstructionFile(const std::string &bigString)
{
    // do the global substitution
    std::string instructionString = bigString;
    if (m_oldStringList.size() > 0)
    {
        for (unsigned int i = 0; i < m_oldStringList.size(); i++)
        {
            instructionString = pystring::replace(instructionString, m_oldStringList[i], m_newStringList[i]);
        }
    }

    std::vector<std::string> instructionLines;
    pystring::splitlines(instructionString, instructionLines);
    int count = 0;
    std::string buffer;
    buffer.reserve(1024);
    for (auto &&instructionLine : instructionLines)
    {
        count++;
        buffer.clear();
        for (size_t c = 0; c < instructionLine.size(); c++)
        {
            if (instructionLine[c] == '#') break;
            buffer.push_back(instructionLine[c]);
        }
        std::vector<std::string> tokens;
        std::regex regexExpression("(?:\\\".*?\\\"|\\S)+"); // this will match whitespace delimited objects and leave quoted objects intact
        std::smatch matchResult;
        std::string::const_iterator searchStart( buffer.cbegin() );
        while (std::regex_search(searchStart, buffer.cend(), matchResult, regexExpression))
        {
            std::string token = matchResult[0];
            if (token.size() >= 2 && pystring::startswith(token, "\"") && pystring::endswith(token, "\""))
            {
                token.pop_back();
                token.erase(token.begin());
            }
            tokens.push_back(std::move(token));
            searchStart = matchResult.suffix().first;
        }
        size_t numTokens = tokens.size();
        if (numTokens == 0) continue;

        if (m_mergeXMLVerbosityLevel > 1)
            std::cerr << "ExecuteInstructionFile:\tnumTokens\t" << numTokens << "\n";

        if (m_mergeXMLVerbosityLevel) std::cerr << instructionLine << "\n";

        // read in a new XML file
        // ReadXMLFile XMLObjectName Filename
        if (tokens[0] == "ReadXMLFile"s)
        {
            if (numTokens != 3) { ParseError(tokens, numTokens, count, "Wrong number of tokens"); continue; }
            if (m_objectList.Get(tokens[1])) { ParseError(tokens, numTokens, count, "XMLObject name not unique"); continue; }
            std::unique_ptr<XMLContainer> object = std::make_unique<XMLContainer>();
            object->SetName(tokens[1]);
            if (object->LoadXML(tokens[2]))
            { ParseError(tokens, numTokens, count, "Error reading XML file"); continue; }
            m_objectList.Add(std::move(object));
            continue;
        }

        // write a FacetedObject to a .obj file
        // WriteXMLFile XMLObjectName Filename
        if (tokens[0] == "WriteXMLFile"s)
        {
            if (numTokens != 3) { ParseError(tokens, numTokens, count, "Wrong number of tokens"); continue; }
            XMLContainer *object = m_objectList.Get(tokens[1]);
            if (object == nullptr) { ParseError(tokens, numTokens, count, "XMLObject name not found"); continue; }
            if (object->WriteXML(tokens[2]))
            { ParseError(tokens, numTokens, count, "Error writing XML file"); continue; }
            continue;
        }

        // merge two XML files
        // Merge XMLObjectName1 XMLObjectName2 Element IDAttribute MergeAttribute Fraction
        // Merge XMLObjectName1 XMLObjectName2 Element IDAttribute MergeAttribute Fraction StartIndex EndIndex
        if (tokens[0] == "Merge"s)
        {
            if (numTokens != 7 && numTokens != 9) { ParseError(tokens, numTokens, count, "Wrong number of tokens"); continue; }
            XMLContainer *object = m_objectList.Get(tokens[1]);
            if (object == nullptr) { ParseError(tokens, numTokens, count, "XMLObjectName name not found"); continue; }
            XMLContainer *object2 = m_objectList.Get(tokens[2]);
            if (object2 == nullptr) { ParseError(tokens, numTokens, count, "XMLObjectName name not found"); continue; }
            if (numTokens == 7)
            {
                if (object->Merge(object2, tokens[3].c_str(), tokens[4].c_str(), tokens[5].c_str(), MergeUtil::Double(tokens[6])))
                { ParseError(tokens, numTokens, count, "Error merging XML files"); continue; }
            }
            else if (numTokens == 9)
            {
                if (object->Merge(object2, tokens[3].c_str(), tokens[4].c_str(), tokens[5].c_str(), MergeUtil::Double(tokens[6]), MergeUtil::Int(tokens[7]), MergeUtil::Int(tokens[8])))
                { ParseError(tokens, numTokens, count, "Error merging XML files"); continue; }
            }
            continue;
        }

        // merge two XML files only specified IDAttribute
        // MergeID XMLObjectName1 XMLObjectName2 Element IDAttribute ID MergeAttribute Fraction
        // MergeID XMLObjectName1 XMLObjectName2 Element IDAttribute ID MergeAttribute Fraction StartIndex EndIndex
        if (tokens[0] == "MergeID"s)
        {
            if (numTokens != 8 && numTokens != 10) { ParseError(tokens, numTokens, count, "Wrong number of tokens"); continue; }
            XMLContainer *object = m_objectList.Get(tokens[1]);
            if (object == nullptr) { ParseError(tokens, numTokens, count, "XMLObjectName name not found"); continue; }
            XMLContainer *object2 = m_objectList.Get(tokens[2]);
            if (object2 == nullptr) { ParseError(tokens, numTokens, count, "XMLObjectName name not found"); continue; }
            if (numTokens == 8)
            {
                if (object->MergeID(object2, tokens[3].c_str(), tokens[4].c_str(), tokens[5].c_str(), tokens[6].c_str(), MergeUtil::Double(tokens[7])))
                { ParseError(tokens, numTokens, count, "Error merging XML files"); continue; }
            }
            else if (numTokens == 10)
            {
                if (object->MergeID(object2, tokens[3].c_str(), tokens[4].c_str(), tokens[5].c_str(), tokens[6].c_str(), MergeUtil::Double(tokens[7]), MergeUtil::Int(tokens[8]), MergeUtil::Int(tokens[9])))
                { ParseError(tokens, numTokens, count, "Error merging XML files"); continue; }
            }
            continue;
        }

        // Operate on an XML value
        // Operate XMLObjectName1 Operation Element IDAttribute IDValue ChangeAttribute Offset
        if (tokens[0] == "Operate"s)
        {
            if (numTokens != 8) { ParseError(tokens, numTokens, count, "Wrong number of tokens"); continue; }
            XMLContainer *object = m_objectList.Get(tokens[1]);
            if (object == nullptr) { ParseError(tokens, numTokens, count, "XMLObjectName name not found"); continue; }
            if (object->Operate(tokens[2].c_str(), tokens[3].c_str(), tokens[4].c_str(), tokens[5].c_str(), tokens[6].c_str(), MergeUtil::Int(tokens[7]), m_globalVariablesList))
            { ParseError(tokens, numTokens, count, "Error performing Operate"); continue; }
            continue;
        }

        // Set value of a Global
        // Global XMLObjectName1 GlobalName Element IDAttribute IDValue ChangeAttribute Offset
        if (tokens[0] == "Global"s)
        {
            if (numTokens != 8) { ParseError(tokens, numTokens, count, "Wrong number of tokens"); continue; }
            XMLContainer *object = m_objectList.Get(tokens[1]);
            if (object == nullptr) { ParseError(tokens, numTokens, count, "XMLObjectName name not found"); continue; }
            if (object->Global(tokens[2].c_str(), tokens[3].c_str(), tokens[4].c_str(), tokens[5].c_str(), tokens[6].c_str(), MergeUtil::Int(tokens[7]), &m_globalVariablesList))
            { ParseError(tokens, numTokens, count, "Error performing Operate"); continue; }
            continue;
        }

        // Set an XML value (will work with string values and can also increase the length of the argument list by 1)
        // If Offset < 0 then the whole of the attribute is set to the new value
        // Set XMLObjectName1 Operation Element IDAttribute IDValue ChangeAttribute Offset
        if (tokens[0] == "Set"s)
        {
            if (numTokens != 8) { ParseError(tokens, numTokens, count, "Wrong number of tokens"); continue; }
            XMLContainer *object = m_objectList.Get(tokens[1]);
            if (object == nullptr) { ParseError(tokens, numTokens, count, "XMLObjectName name not found"); continue; }
            if (object->Set(tokens[2].c_str(), tokens[3].c_str(), tokens[4].c_str(), tokens[5].c_str(), tokens[6].c_str(), MergeUtil::Int(tokens[7])))
            { ParseError(tokens, numTokens, count, "Error performing Set"); continue; }
            continue;
        }

        // Scale an XML position key
        // Scale XMLObjectName1 Operation Element IDAttribute IDValue ChangeAttribute ReferenceID
        if (tokens[0] == "Scale"s)
        {
            if (numTokens != 8) { ParseError(tokens, numTokens, count, "Wrong number of tokens"); continue; }
            XMLContainer *object = m_objectList.Get(tokens[1]);
            if (object == nullptr) { ParseError(tokens, numTokens, count, "XMLObjectName name not found"); continue; }
            if (object->Scale(tokens[2].c_str(), tokens[3].c_str(), tokens[4].c_str(), tokens[5].c_str(), tokens[6].c_str(), tokens[7].c_str()))
            { ParseError(tokens, numTokens, count, "Error performing Scale"); continue; }
            continue;
        }

        // Swap two XML values
        // Swap XMLObjectName1 Element IDAttribute IDValue ChangeAttribute Offset1 Offset2
        if (tokens[0] == "Swap"s)
        {
            if (numTokens != 8) { ParseError(tokens, numTokens, count, "Wrong number of tokens"); continue; }
            XMLContainer *object = m_objectList.Get(tokens[1]);
            if (object == nullptr) { ParseError(tokens, numTokens, count, "XMLObjectName name not found"); continue; }
            if (object->Swap(tokens[2].c_str(), tokens[3].c_str(), tokens[4].c_str(), tokens[5].c_str(), MergeUtil::Int(tokens[6]), MergeUtil::Int(tokens[7])))
            { ParseError(tokens, numTokens, count, "Error performing Swap"); continue; }
            continue;
        }

        // Generate a new XML values
        // Generate XMLObjectName1 Operation Element IDAttribute IDValue GenerateAttribute
        if (tokens[0] == "Generate"s)
        {
            if (numTokens != 7) { ParseError(tokens, numTokens, count, "Wrong number of tokens"); continue; }
            XMLContainer *object = m_objectList.Get(tokens[1]);
            if (object == nullptr) { ParseError(tokens, numTokens, count, "XMLObjectName name not found"); continue; }
            if (object->Generate(tokens[2].c_str(), tokens[3].c_str(), tokens[4].c_str(), tokens[5].c_str(), tokens[6].c_str()))
            { ParseError(tokens, numTokens, count, "Error performing Generate"); continue; }
            continue;
        }

        // Delete an XML values
        // Delete XMLObjectName1 Element IDAttribute IDValue DeleteAttribute

        if (tokens[0] == "Delete"s)
        {
            if (numTokens != 6) { ParseError(tokens, numTokens, count, "Wrong number of tokens"); continue; }
            XMLContainer *object = m_objectList.Get(tokens[1]);
            if (object == nullptr) { ParseError(tokens, numTokens, count, "XMLObjectName name not found"); continue; }
            if (object->Delete(tokens[2].c_str(), tokens[3].c_str(), tokens[4].c_str(), tokens[5].c_str()))
            { ParseError(tokens, numTokens, count, "Error performing Delete"); continue; }
            continue;
        }

        // Create a new XML tag
        // Create XMLObjectName1 Element IDAttribute IDValue

        if (tokens[0] == "Create"s)
        {
            if (numTokens != 5) { ParseError(tokens, numTokens, count, "Wrong number of tokens"); continue; }
            XMLContainer *object = m_objectList.Get(tokens[1]);
            if (object == nullptr) { ParseError(tokens, numTokens, count, "XMLObjectName name not found"); continue; }
            if (object->Create(tokens[2].c_str(), tokens[3].c_str(), tokens[4].c_str()))
            { ParseError(tokens, numTokens, count, "Error performing Create"); continue; }
            continue;
        }

        ParseError(tokens, numTokens, count, "Unrecognised line");
    }
}

// Produce a parse error message
void MergeXML::ParseError(const std::vector<std::string> &tokens, size_t numTokens, int count, const char *message)
{
    m_errorMessageList[count] = message;
    std::vector<std::string> errorTokens;
    for (int i = 0; i < numTokens; i++) errorTokens.push_back(tokens[i]);
    m_errorList[count] = std::move(errorTokens);
}

const std::map<int, std::string> &MergeXML::errorMessageList() const
{
    return m_errorMessageList;
}

const std::map<int, std::vector<std::string> > &MergeXML::errorList() const
{
    return m_errorList;
}



