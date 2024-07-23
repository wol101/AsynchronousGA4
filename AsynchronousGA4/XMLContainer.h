/*
 *  XMLContainer.h
 *  MergeXML
 *
 *  Created by Bill Sellers on Wed Dec 17 2003.
 *  Copyright (c) 2003 Bill Sellers. All rights reserved.
 *
 */

#ifndef XMLContainer_h
#define XMLContainer_h

#include <map>
#include <string>
#include <vector>

namespace rapidxml { template<class Ch> class xml_document; }
namespace rapidxml { template<class Ch> class xml_node; }
namespace rapidxml { template<class Ch> class xml_attribute; }

class XMLContainer
{
public:

    XMLContainer();
    ~XMLContainer();

    void SetName(const std::string &name) { m_name = name; }
    const std::string &GetName() { return m_name; }

    char *DoXmlGetProp(rapidxml::xml_node<char> *cur, const char *name);
    rapidxml::xml_attribute<char> *DoXmlReplaceProp(rapidxml::xml_node<char> *cur, const char *name, const char *newValue);
    void DoXmlRemoveProp(rapidxml::xml_node<char> *cur, const char *name);
    rapidxml::xml_attribute<char> *DoXmlHasProp(rapidxml::xml_node<char> *cur, const char *name);
    rapidxml::xml_node<char> *CreateNewNode(rapidxml::xml_node<char> *parent, const char *tag);

    int LoadXML(const std::string &file);
    int WriteXML(const std::string &file);
    int Merge(XMLContainer *sim, const char *element, const char *idAttribute, const char *mergeAttribute, double proportion, int startIndex = 0, int endIndex = -1);
    int MergeID(XMLContainer *sim, const char *element, const char *idAttribute, const char *id, const char *mergeAttribute, double proportion, int startIndex = 0, int endIndex = -1);
    int Operate(const char *operation, const char *element, const char *idAttribute, const char *idValue, const char *changeAttribute, int offset, const std::map<std::string, double> &extraVariables);
    int Global(const char *globalName, const char *element, const char *idAttribute, const char *idValue, const char *changeAttribute, int offset, std::map<std::string, double> *extraVariables);
    int Set(const char *operation, const char *element, const char *idAttribute, const char *idValue, const char *changeAttribute, int offset);
    int Scale(const char *operation, const char *element, const char *idAttribute, const char *idValue, const char *changeAttribute, const char *referenceID);
    int Swap(const char *element, const char *idAttribute, const char *idValue, const char *changeAttribute, int offset1, int offset2);
    int Generate(const char *operation, const char *element, const char *idAttribute, const char *idValue, const char *changeAttribute);
    int Delete(const char *element, const char *idAttribute, const char *idValue, const char *changeAttribute);
    int Create(const char *element, const char *idAttribute, const char *idValue);

    int Merge(rapidxml::xml_node<char> *node1, rapidxml::xml_node<char> *node2, double proportion, const char *name, int startIndex, int endIndex);

private:
    rapidxml::xml_document<char> *m_doc = nullptr;
    std::vector<char> m_xmlDataStore;

    std::string m_rootNode;
    std::vector<rapidxml::xml_node<char> *> m_tagContentsList;

    std::string m_name;

    bool m_caseSensitiveXMLAttributes = true;
    int m_xmkContainerVerbosityLevel = 1;

};


#endif

