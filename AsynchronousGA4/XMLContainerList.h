/*
 *  XMLContainerList.h
 *  MergeXML
 *
 *  Created by Bill Sellers on Mon Sep 19 2005.
 *  Copyright (c) 2005 Bill Sellers. All rights reserved.
 *
 */

#ifndef XMLContainerList_h
#define XMLContainerList_h

#include "XMLContainer.h"

#include <vector>
#include <memory>

class XMLContainer;

class XMLContainerList
{
public:

   XMLContainerList();

   void Add(std::unique_ptr<XMLContainer> object);
   XMLContainer *Get(int index);
   XMLContainer *Get(const std::string &name);
   int Find(const std::string &name);
   void Clear();

private:

   std::vector<std::unique_ptr<XMLContainer>> m_listObjects;
};

#endif
