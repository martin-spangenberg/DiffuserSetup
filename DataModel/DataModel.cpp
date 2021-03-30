#include "DataModel.h"

DataModel::DataModel(){}

DfTree* DataModel::GetDfTree(std::string name)
{
//  std::cout << "map size " << m_DfTrees.size() << std::endl;
//  std::cout << "GETTING from map " << name << "." << std::endl;
  if (m_dftrees.count(name))
    return m_dftrees[name];
  else
    return 0;
}

void DataModel::AddDfTree(std::string name, DfTree *tree)
{
  std::cout << "map size " << m_dftrees.size() << std::endl;
  std::cout << "ADDING to map " << name << ",\t" << tree << "." << std::endl;
  m_dftrees[name]=tree;
  std::cout << m_dftrees[name] << std::endl;
}


void DataModel::DeleteDfTree(std::string name)
{
  std::cout << "map size " << m_dftrees.size() << std::endl;
  std::cout << "REMOVING from map " << name << "." << std::endl;
  delete m_dftrees[name];
  m_dftrees.erase(name);
}

int DataModel::SizeDfTree()
{
  return m_dftrees.size();
}
