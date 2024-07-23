/*
 *  MergeXML.h
 *  MergeXML
 *
 *  Created by Bill Sellers on Wed Dec 17 2003.
 *  Copyright (c) 2003 Bill Sellers. All rights reserved.
 *
 */

#ifndef MergeXML_h
#define MergeXML_h

#include "XMLContainerList.h"

#include <vector>
#include <string>
#include <map>

class MergeXML
{
public:
    MergeXML();

    void ExecuteInstructionFile(const std::string &bigString);

    const std::map<int, std::vector<std::string> > &errorList() const;

    const std::map<int, std::string> &errorMessageList() const;

private:
    void ParseError(const std::vector<std::string> &tokens, size_t numTokens, int count, const char *message);

    XMLContainerList m_objectList;
    std::vector<std::string> m_oldStringList;
    std::vector<std::string> m_newStringList;
    std::map<int, std::vector<std::string> > m_errorList;
    std::map<int, std::string> m_errorMessageList;
    std::map<std::string, double> m_globalVariablesList;

    int m_mergeXMLVerbosityLevel = 0;
};


#endif

