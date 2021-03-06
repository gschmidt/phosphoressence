// __BEGIN_LICENSE__
// Copyright (C) 2006, 2007 United States Government as represented by
// the Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
// __END_LICENSE__

#ifndef __VW_GPU_UTILITIES__
#define __VW_GPU_UTILITIES__

#include <map> 
#include <string>

namespace vw { 
namespace GPU {

// TokenReplacer
class TokenReplacer {
  std::map<std::string, std::string> stringMap;
public:
  void AddVariable(std::string& variableName, std::string& variableValue);
  void AddVariable(const char* variableName, const char*  variableValue);
  void Replace(const std::string& inText, std::string& outText);
  void Clear() { stringMap.clear(); }
  
};

// File I/O
bool ReadFileAsString(const std::string& path, std::string& outString);

}} // namespaces vw::GPU


#endif // __VW_GPU_UTILITIES__
