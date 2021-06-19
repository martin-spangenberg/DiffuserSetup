#ifndef PYTHON_API
#define PYTHON_API

#include "BoostStore.h"
#include <boost/core/demangle.hpp>
#include <boost/lexical_cast.hpp>
#include <typeinfo>
#include <exception>
#include <vector>
#include <map>
#include "DataModel.h"
#include "BoostStore.h"

//#include <regex>
#include <boost/regex.hpp> // since std::regex doesn't work
#include <boost/regex/pattern_except.hpp>

static std::map<std::string,std::string> typename_to_python_type{{"int","i"},{"long long","l"},{"double","d"},{"char","c"},{"std::string","s"},{"bool","i"}};

static DataModel* gstore;        // m_data, set in PythonScript.cpp
static Store* gconfig;           // the Store containing the current script's config variables
/*static BoostStore* activestore;  // the Store within m_data requested by the python user*/

struct StoreWrapper{
  StoreWrapper(){};
  StoreWrapper(BoostStore* sin) : boost_store(sin){};
  StoreWrapper(Store* sin) : ascii_store(sin){};
  BoostStore* boost_store;
  Store* ascii_store;
  
  // BoostStores can Get/Set a far greater variety of classes,
  // while ASCII stores can only Get/Set those with a suitable ASCII streamer operator.
  // We need to prevent the compiler from trying to generate functions that invoke
  // Get/Set on an ASCII store with an unsupported type. We'll use a tag dispatch method.
  template <typename T>
  bool Get(std::true_type, std::string name, T& value){
    //std::cout<<"StoreWrapper::Get called on a universally supported type"
    //         <<boost::core::demangle(typeid(value).name())<<std::endl;
    if(boost_store) return boost_store->Get(name,value);
    else if(ascii_store) return ascii_store->Get(name,value);
    else { std::cerr<<"Get call on uninitialized StoreWrapper!"<<std::endl; return false; }
  };
  template <typename T>
  bool Get(std::false_type, std::string name, T& value){
    //std::cout<<"StoreWrapper::Get called on non-ASCII type "
    //         <<boost::core::demangle(typeid(value).name())<<std::endl;
    if(boost_store){ return boost_store->Get(name,value);
    } else if(ascii_store){
      std::cerr<<"StoreWrapper::Get called to return object of type "
               <<boost::core::demangle(typeid(value).name())
               <<" but requested store is an ASCII store, and cannot store objects of this type"<<std::endl;
      return false;
    } else {
      std::cerr<<"Get call on uninitialized StoreWrapper!"<<std::endl;
      return false;
    }
  };
  template <typename T>
  bool Get(std::string name, T& value){
    constexpr bool t_is_ascii_compatible = 
        std::is_fundamental<T>::value || std::is_same<T, std::string>::value;
    using tag = std::integral_constant<bool, t_is_ascii_compatible>;
    //std::cout<<"StoreWrapper Get called with type "<<boost::core::demangle(typeid(value).name())
    //         <<" with tag "<<boost::core::demangle(typeid(tag).name())<<std::endl;
    return StoreWrapper::Get(tag{}, name, value);
  }
  
  template<typename T>
  void Set(std::true_type, std::string name, T& value){
    if(boost_store) boost_store->Set(name,value);
    else if(ascii_store) ascii_store->Set(name,value);
    else std::cerr<<"Set call on uninitialized StoreWrapper!"<<std::endl;
  };
  template<typename T>
  void Set(std::false_type, std::string name, T& value){
    if(boost_store){ boost_store->Set(name,value);
    } else if(ascii_store){
      std::cerr<<"StoreWrapper::Set called to set object of type "<<boost::core::demangle(typeid(value).name())
               <<" but requested store is an ASCII store, and cannot store objects of this type"<<std::endl;
    } else {
      std::cerr<<"Set call on uninitialized StoreWrapper!"<<std::endl;
    }
  };
  template <typename T>
  void Set(std::string name, T& value){
    constexpr bool t_is_ascii_compatible = 
        std::is_fundamental<T>::value || std::is_same<T, std::string>::value;
    using tag = std::integral_constant<bool, t_is_ascii_compatible>;
    StoreWrapper::Set(tag{}, name, value);
  }
  
  std::string Type(std::string name){
    if(boost_store){ return boost_store->Type(name); }
    else { return "unsupported by ASCII stores"; }
  };
  StoreWrapper& operator= (Store* instore){ ascii_store=instore; boost_store=nullptr; return *this; }
  StoreWrapper& operator= (BoostStore* instore){ boost_store=instore; ascii_store=nullptr; return *this; }
  StoreWrapper* operator-> () { return this; }
  std::string StoreType(){
    if(boost_store) return "boost";
    if(ascii_store) return "ascii";
    return "none";
  }
};

static StoreWrapper activestore;

static PyObject* GetStoreInt(PyObject *self, PyObject *args){
  const char *command=nullptr;
  if (!PyArg_ParseTuple(args, "s", &command)) return NULL;
  int ret=0;
  activestore->Get(command,ret);
  return Py_BuildValue("i", ret);

}

static PyObject* GetStoreDouble(PyObject *self, PyObject *args){  
  const char *command=nullptr;
  if (!PyArg_ParseTuple(args, "s", &command)) return NULL;
  double ret=0;
  activestore->Get(command,ret);
  return Py_BuildValue("d", ret);

}

static PyObject* GetStoreString(PyObject *self, PyObject *args){
  const char *command=nullptr;
  if (!PyArg_ParseTuple(args, "s", &command)) return NULL;
  std::string ret="";
  activestore->Get(command,ret);
  return Py_BuildValue("s", ret.c_str());

}

// ======================================================

// general typename match of arithmetic types, char and bool
template<typename T> PyObject* GetStoreVariable(std::string variablename, T tempvar, bool isptr, bool isvector){
  std::string thetypename = boost::core::demangle(typeid(tempvar).name());
  // confirm we recognise the type and have a suitable python conversion
  if(typename_to_python_type.count(thetypename)==0){
    std::cerr<<"PythonAPI::GetStoreVariable variable type "<<thetypename
             <<" not in type conversion map"<<std::endl;
             return NULL;
  }
  /// get the format string describing the python type
  const char* python_type = typename_to_python_type.at(thetypename).c_str();
  // get the variable from the BoostStore
  if(not isvector){
    if(isptr){
      // if the type in the BoostStore is actually a pointer to this type of object
      // we need to copy it to a temporary
      T* tempvar2=nullptr;
      int get_ok = activestore->Get(variablename,tempvar2);
      if(not get_ok){
        std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
        return NULL;
      }
      tempvar = *tempvar2;
    } else {
      int get_ok = activestore->Get(variablename,tempvar);
      if(not get_ok){
        std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
        return NULL;
      }
    }
    // convert to python object and return
    return Py_BuildValue(python_type,tempvar);
  } else {
    // additional layer of complexity: the item in the store is a vector of objects.
    // we will not support vectors of pointers
    std::vector<T> tempvec;
    int get_ok = activestore->Get(variablename,tempvec);
    if(not get_ok){
      std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
      return NULL;
    }
    PyObject *PList = PyList_New(0);
    for(auto&& element : tempvec){
      PyList_Append(PList, Py_BuildValue(python_type,element));
    }
    return PList;
  }
}

// specialization for std::string, which we must convert to char* before converting to Python object
template<>
inline PyObject* GetStoreVariable<std::string>(std::string variablename, std::string tempvar, bool isptr, bool isvector){
  // get the variable from the BoostStore
  if(not isvector){
    if(isptr){
      // if the type in the BoostStore is actually a pointer to this type of object
      // we need to copy it to a temporary
      std::string* tempvar2=nullptr;
      int get_ok = activestore->Get(variablename,tempvar2);
      if(not get_ok){
        std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
        return NULL;
      }
      tempvar = *tempvar2;
    } else {
      int get_ok = activestore->Get(variablename,tempvar);
      if(not get_ok){
        std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
        return NULL;
      }
    }
    // convert to python object and return
    return Py_BuildValue("s",tempvar.c_str());
  } else {
    // additional layer of complexity: the item in the store is a vector of objects.
    // we will not support vectors of pointers
    std::vector<std::string> tempvec;
    int get_ok = activestore->Get(variablename,tempvec);
    if(not get_ok){
      std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
      return NULL;
    }
    PyObject *PList = PyList_New(0);
    for(auto&& element : tempvec){
      PyList_Append(PList, Py_BuildValue("s",element.c_str()));
    }
    return PList;
    //if (PyTuple_SetItem(args, 1, datum1) != 0) {    // equivalent to return a tuple
    //  return false;
    //}
  }
}

// // specialization for Position, which we must converted to Python tuple
// template<>
// inline PyObject* GetStoreVariable<Position>(std::string variablename, Position tempvar, bool isptr, bool isvector){
//   // get the variable from the BoostStore
//   if(not isvector){
//     if(isptr){
//       // if the type in the BoostStore is actually a pointer to this type of object
//       // we need to copy it to a temporary
//       Position* tempvar2;
//       int get_ok = activestore->Get(variablename,tempvar2);
//       if(not get_ok){
//         std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
//         return NULL;
//       }
//       tempvar = *tempvar2;
//     } else {
//       int get_ok = activestore->Get(variablename,tempvar);
//       if(not get_ok){
//         std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
//         return NULL;
//       }
//     }
//     // convert to python object and return
//     return Py_BuildValue("(ddd)",tempvar.X(), tempvar.Y(), tempvar.Z());
//   } else {
//     // additional layer of complexity: the item in the store is a vector of objects.
//     // we will not support vectors of pointers
//     std::vector<Position> tempvec;
//     int get_ok = activestore->Get(variablename,tempvec);
//     if(not get_ok){
//       std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
//       return NULL;
//     }
//     PyObject *PList = PyList_New(0);
//     for(auto&& element : tempvec){
//       PyList_Append(PList, Py_BuildValue("(ddd)",element.X(), element.Y(), element.Z()));
//     }
//     return PList;
//   }
// }

// // specialization for Direction, which we must convert to Python tuple
// template<>
// inline PyObject* GetStoreVariable<Direction>(std::string variablename, Direction tempvar, bool isptr, bool isvector){
//   // get the variable from the BoostStore
//   if(not isvector){
//     if(isptr){
//       // if the type in the BoostStore is actually a pointer to this type of object
//       // we need to copy it to a temporary
//       Direction* tempvar2;
//       int get_ok = activestore->Get(variablename,tempvar2);
//       if(not get_ok){
//         std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
//         return NULL;
//       }
//       tempvar = *tempvar2;
//     } else {
//       int get_ok = activestore->Get(variablename,tempvar);
//       if(not get_ok){
//         std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
//         return NULL;
//       }
//     }
//     int get_ok = activestore->Get(variablename,tempvar);
//     if(not get_ok){
//       std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
//       return NULL;
//     }
//     // convert to python object and return
//     return Py_BuildValue("(ddd)",tempvar.X(), tempvar.Y(), tempvar.Z());
//   } else {
//     // additional layer of complexity: the item in the store is a vector of objects.
//     // we will not support vectors of pointers
//     std::vector<Direction> tempvec;
//     int get_ok = activestore->Get(variablename,tempvec);
//     if(not get_ok){
//       std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
//       return NULL;
//     }
//     PyObject *PList = PyList_New(0);
//     for(auto&& element : tempvec){
//       PyList_Append(PList, Py_BuildValue("(ddd)",element.X(), element.Y(), element.Z()));
//     }
//     return PList;
//   }
// }

// // specialization for TimeClass, which we must call GetNS before converting to Python object
// template<>
// inline PyObject* GetStoreVariable<TimeClass>(std::string variablename, TimeClass tempvar, bool isptr, bool isvector){
//   // get the variable from the BoostStore
//   if(not isvector){
//     if(isptr){
//       // if the type in the BoostStore is actually a pointer to this type of object
//       // we need to copy it to a temporary
//       TimeClass* tempvar2;
//       int get_ok = activestore->Get(variablename,tempvar2);
//       if(not get_ok){
//         std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
//         return NULL;
//       }
//       tempvar = *tempvar2;
//     } else {
//       int get_ok = activestore->Get(variablename,tempvar);
//       if(not get_ok){
//         std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
//         return NULL;
//       }
//     }
//     int get_ok = activestore->Get(variablename,tempvar);
//     if(not get_ok){
//       std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
//       return NULL;
//     }
//     // convert to python object and return
//     return Py_BuildValue("l",tempvar.GetNs());
//   } else {
//     // additional layer of complexity: the item in the store is a vector of objects.
//     // we will not support vectors of pointers
//     std::vector<TimeClass> tempvec;
//     int get_ok = activestore->Get(variablename,tempvec);
//     if(not get_ok){
//       std::cerr<<"PythonAPI::GetStoreVariable failed to get object from store!"<<std::endl;
//       return NULL;
//     }
//     PyObject *PList = PyList_New(0);
//     for(auto&& element : tempvec){
//       PyList_Append(PList, Py_BuildValue("l", element.GetNs()));
//     }
//     return PList;
//   }
// }

// =============================================

// generic get wrapper based on the type of variable found in the store
static PyObject* GetStoreVariable(PyObject *self, PyObject *args){
  const char* storename=nullptr;
  const char* variablename=nullptr;
  const char* variabletype=nullptr;
  if (!PyArg_ParseTuple(args, "ss|s", &storename, &variablename, &variabletype)) return NULL;
  
  // Set the activestore pointer to the appropriate BoostStore
  // =========================================================
  if(strcmp(storename,"Config")==0){
    // config variables are retrieved from a Store, not a BoostStore, so we don't have access to the
    // variable typename. On other other hand these variables are by definition passed through a text file,
    // so we can always retrieve them as a string
    std::string tempvar="";
    gconfig->Get(variablename,tempvar);
    // for convenience we can also try to see if it's numeric:
    try {
      double tempnum = boost::lexical_cast<double>(tempvar);
      // if we haven't thrown to the catch, we have something numeric.
      // we could also try to check if it's integral and return a Python Long instead of Float
      if(tempnum==std::llround(tempnum)){
        long long tempnuml = std::llround(tempnum);
        return Py_BuildValue("i", tempnuml);
      } else {
        return Py_BuildValue("d", tempnum);
      }
    }
    catch(boost::bad_lexical_cast &e){
      // not a recognisable number: just return as string
      return Py_BuildValue("s", tempvar.c_str());
    }
  } else if(strcmp(storename,"CStore")==0){
    activestore= &(gstore->CStore);
  } else if(strcmp(storename,"vars")==0){
    activestore= &(gstore->vars);
  } else {
    if(gstore->Stores.count(storename)==0){
      std::cerr<<"PythonAPI::GetStoreVariable failed to find requested Store "<<storename<<std::endl;
      return NULL;
    }
    activestore = gstore->Stores.at(storename);
  }
  
  // get the data type of the requested variable
  // ===========================================
  std::string thetypename="";
  std::string storetype = activestore->StoreType();
  if(variabletype==nullptr){
    if(strcmp(storetype.c_str(),"ascii")==0){
      std::string errmsg = "PythonAPI::GetStoreVariable request to get a variable from store '"
               +std::string(storename)+"', which is an ASCII store. For ASCII stores you must provide"
               +" an additional argument specifying the variable datatype. Valid types are: {";
      for(auto&& atype : typename_to_python_type){ errmsg += atype.first+", "; }
      errmsg=errmsg.substr(0,errmsg.size()-2);
      errmsg+= "}";
      std::cerr<<errmsg<<std::endl;
      return NULL;
    }
    thetypename = activestore->Type(variablename);
    if(strcmp(thetypename.c_str(),"?")==0){
      std::cerr<<"PythonAPI::GetStoreVariable failed to get object type from store!"
               <<" Enable BoostStore type checking, or use a specialised Getter"<<std::endl;
      return NULL;
    } else if(strcmp(thetypename.c_str(),"Not in Store")==0){
      std::cerr<<"PythonAPI::GetStoreVariable failed to get object type from store!"
               <<" Variable not found in store!"<<std::endl;
      return NULL;
    }
    thetypename = boost::core::demangle(thetypename.c_str());
  } else {
    if(strcmp(storetype.c_str(),"ascii")==0){
      if(typename_to_python_type.count(variabletype)==0){
        std::cerr<<"PythonAPI::GetStoreVariable failed, unrecognised data type '"
                 <<variabletype<<"'"<<std::endl;
        return NULL;
      } else {
        thetypename=variabletype;
      }
    } else {
      // BoostStores may not hold a type map if type checking was disabled,
      // so we let the user specify, but since BoostStores can hold all manner
      // of things we cannot validate it. You have been warned!
      thetypename=variabletype;
    }
  }
  
  // check if it's a pointer
  // =======================
  bool isptr = ( thetypename.back()=='*');
  if(isptr){ thetypename.pop_back(); }
  
  // check if it's a vector
  // ======================
  bool isvector = (thetypename.find("vector")!= std::string::npos);
  if(isvector){
    // a demangled vector<int> looks like: 'std::vector<int, std::allocator<int> >'
    // use regexp to try to match this pattern, and pull out the type of contained elements
    std::string regex_expression="std::vector<([^,]+), std::allocator<([^>]+)> >";
    //std::cout<<"building regex from string "<<regex_expression<<std::endl;
    
    // std::regex version
    /*
    std::regex theexpression(regex_expression);
    std::match_results<std::string::const_iterator> submatches;
    //std::cout<<"matching regex expression "<<regex_expression<<std::endl;
    std::regex_match (thetypename, submatches, theexpression);
    if((std::string)submatches[0]==""){
      std::cerr<<"PythonAPI::GetStoreVariable: unrecognised type: "<<thetypename<<std::endl;
      return NULL;
    }
    //std::cout<<"extracted element type is "<<(std::string)submatches[1]<<std::endl;
    ///std::cout<<"extracted allocator type "<<(std::string)submatches[2]<<std::endl;
    // consistency check:
    if((std::string)submatches[1]!=(std::string)submatches[2]){
          std::cerr<<"PythonAPI::GetStoreVariable: Vector element type "<<(std::string)submatches[1]
                   <<" doesn't match allocator type "<<(std::string)submatches[1] 
                   <<std::endl;
          return NULL;
    }
    thetypename = (std::string)submatches[1];  // match 0 is 'whole match' or smthg
    */
    
    // boost edition because std::regex is broken in g++4.9
    try{
        boost::regex theexpression;
        theexpression.assign(regex_expression,boost::regex::perl);
        //std::cout<<"declaring match results to hold matches"<<std::endl;
        boost::smatch submatches;
        //std::cout<<"matching regex expression "<<regex_expression<<std::endl;
        boost::regex_match (thetypename, submatches, theexpression);
        //std::cout<<"match done"<<std::endl;
        if((std::string)submatches[0]==""){
          std::cerr<<"PythonAPI::GetStoreVariable: unrecognised type: "<<thetypename<<std::endl;
          return NULL;
        }
        //std::cout<<"extracted element type is "<<(std::string)submatches[1]<<std::endl;
        //std::cout<<"extracted allocator type "<<(std::string)submatches[2]<<std::endl;
        // consistency check:
        if((std::string)submatches[1]!=(std::string)submatches[2]){
              std::cerr<<"PythonAPI::GetStoreVariable: Vector element type "<<(std::string)submatches[1]
                       <<" doesn't match allocator type "<<(std::string)submatches[1] 
                       <<std::endl;
              return NULL;
        }
        thetypename = (std::string)submatches[1];  // match 0 is 'whole match' or smthg
        //std::cout<<"element type is "<<thetypename<<std::endl;
    } catch (boost::regex_error& e){
        // if(bregex_err_strings.count(e.code())){
        //     std::cerr<<bregex_err_strings.at(e.code())<<std::endl;
        // } else {
        //     std::cerr<<"unknown boost::refex_error?? code: "<<e.code()<<std::endl;
        // }
        return NULL;
    } catch(std::exception& e){
        std::cerr<<"Warning: regex matching threw: "<<e.what()<<std::endl;
        return NULL;
    } catch(...){
        std::cerr<<"Warning: unhandled regex matching exception!"<<std::endl;
        return NULL;
    }
  }
  
  // Call the appropriate subtype of GetXXX() based on  the variable type
  // ====================================================================
  // Basic types first
  // ~~~~~~~~~~~~~~~~~
  if(thetypename==boost::core::demangle(typeid(bool).name())
  ){
    bool tempvar=false;
    return GetStoreVariable(variablename, tempvar, isptr, isvector);        // bool
  }
  if(thetypename==boost::core::demangle(typeid(int).name())           ||
     thetypename==boost::core::demangle(typeid(short).name())         ||
     thetypename==boost::core::demangle(typeid(unsigned int).name())  ||
     thetypename==boost::core::demangle(typeid(unsigned short).name())
  ){
    int tempvar=0;       // python only has int...
    return GetStoreVariable(variablename, tempvar, isptr, isvector);        // int
  }
  else if(thetypename==boost::core::demangle(typeid(long).name())           ||
          thetypename==boost::core::demangle(typeid(unsigned long).name())  ||
          thetypename==boost::core::demangle(typeid(long long).name())      ||
          thetypename==boost::core::demangle(typeid(unsigned long long).name())
  ){
    long long tempvar=0; // python only has long...
    return GetStoreVariable(variablename, tempvar, isptr, isvector);       // long
  }
  else if(thetypename==boost::core::demangle(typeid(double).name()) ||
          thetypename==boost::core::demangle(typeid(float).name())
  ){
    double tempvar=0;    // python only has float...
    return GetStoreVariable(variablename, tempvar, isptr, isvector);       // float
  }
  else if(thetypename==boost::core::demangle(typeid(char).name())
  ){
    char tempvar=' ';
    return GetStoreVariable(variablename, tempvar, isptr, isvector);       // char
  }
  else if(/*thetypename==boost::core::demangle(typeid(char*).name()) || */ // XXX how do we handle char arrays?
          thetypename==boost::core::demangle(typeid(std::string).name())
  ){
    std::string tempvar="";
    return GetStoreVariable(variablename, tempvar, isptr, isvector);      // string
  }
  // Simpler ToolAnalysis classes
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // else if(thetypename==boost::core::demangle(typeid(Position).name())){
  //   Position tempvar;
  //   return GetStoreVariable(variablename, tempvar, isptr, isvector);      // Position
  // }
  // else if(thetypename==boost::core::demangle(typeid(Direction).name())){
  //   Direction tempvar;
  //   return GetStoreVariable(variablename, tempvar, isptr, isvector);      // Direction
  // }
  // else if(thetypename==boost::core::demangle(typeid(TimeClass).name())){
  //   TimeClass tempvar;
  //   return GetStoreVariable(variablename, tempvar, isptr, isvector);      // TimeClass
  // }
  // More complex ToolAnalysis classes
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // TODO: add your own class here (and below too, if you want to support vectors of that class
  // TODO std::map might also be useful
  else {
    std::cerr<<"PythonAPI::GetStoreVariable Unrecognised data type in Store "<<thetypename<<std::endl;
    return NULL;
  }
}

// ===========================================
// ===========================================
// Setters

template<typename T> 
PyObject* SetStoreVariable(std::string variablename, PyObject* variableasobj, T tempvar, bool isiterable){
  // need to get the python type format string
  // fortunately these are the same whether parsing args or building a pyobject
  std::string thetypename = boost::core::demangle(typeid(tempvar).name());
  // confirm we recognise the type and have a suitable python conversion
  if(typename_to_python_type.count(thetypename)==0){
    std::cerr<<"PythonAPI::SetStoreVariable variable type "<<thetypename
             <<" not in type conversion map"<<std::endl;
             return NULL;
  }
  /// get the format string describing the python type
  const char* python_type = typename_to_python_type.at(thetypename).c_str();
  
  if(isiterable){
    // construct a vector with the container elements
    PyObject* iterator = PyObject_GetIter(variableasobj);
    PyObject* item;
    std::vector<T> tempvec;
    while((item = PyIter_Next(iterator))){
      PyTypeObject* pythontype = item->ob_type;
      if (!PyArg_ParseTuple(item, python_type, &tempvar)) return NULL;
      tempvec.push_back(tempvar);
      Py_DECREF(item);  // release the reference obtained by PyIter_Next ...
    }
    Py_DECREF(iterator);
    activestore->Set(variablename,tempvec);
    return variableasobj;
  } else {
    if (!PyArg_ParseTuple(variableasobj, python_type, &tempvar)) return NULL;
    activestore->Set(variablename,tempvar);
    return variableasobj;
  }
  return NULL; // not sure how we got here
}

// specialization for String objects; they need to be retrieved via a char* but converted to std::string before storage
template<>
inline PyObject* SetStoreVariable<std::string>(std::string variablename, PyObject* variableasobj, std::string tempvar, bool isiterable){
  char* tempchars=nullptr;
  if(isiterable){
    // construct a vector with the container elements
    PyObject* iterator = PyObject_GetIter(variableasobj);
    PyObject* item;
    std::vector<std::string> tempvec;
    while((item = PyIter_Next(iterator))){
      PyTypeObject* pythontype = item->ob_type;
      if (!PyArg_ParseTuple(item, "s", &tempchars)) return NULL;
      tempvar = std::string(tempchars);
      tempvec.push_back(tempvar);
      Py_DECREF(item);  // release the reference obtained by PyIter_Next ...
    }
    Py_DECREF(iterator);
    activestore->Set(variablename,tempvec);
    return variableasobj;
  } else {
    if (!PyArg_ParseTuple(variableasobj, "s", &tempchars)) return NULL;
    tempvar = std::string(tempchars);
    activestore->Set(variablename,tempvar);
    return variableasobj;
  }
  return NULL; // not sure how we got here
}

// TODO although we can retrieve Position, Direction and TimeClass objects
// they are returned to python as lists / long ints. We would need some additional
// flag from the python call to be able to determine if the user wishes to put a long or list
// of ints into a Position, Direction or TimeClass object in the Store.
// Suggested: Look into using SWIG to generate the necessary wrappers

// ===================================
// Generic wrapper

static PyObject* SetStoreVariable(PyObject* self, PyObject* args){
  
  // 'args' will be a tuple in which the first element should be a string (name of store)
  // the second element also a string (the name of the variable)
  // and the third element will be the value, whose type we would ideally like to determine automatically
  // Let's try to do this
  if(PyTuple_Size(args)!=3){   // PyTuple_Size returns a Py_ssize_t... do we need to convert it?
    std::cerr<<"PythonAPI::SetStoreVariable usage: SetStoreVariable('storename','variablename',variableval)"
             <<std::endl;
    return NULL;
  }
  PyObject* storenameasobj = PyTuple_GetSlice(args,0,0);
  if(storenameasobj){
    std::cerr<<"PythonAPI::SetStoreVariable failed to retrieve store name from args"<<std::endl;
    return NULL;
  }
  PyObject* variablenameasobj = PyTuple_GetSlice(args,1,1);
  if(variablenameasobj){
    std::cerr<<"PythonAPI::SetStoreVariable failed to retrieve variable name from args"<<std::endl;
    return NULL;
  }
  PyObject* variableasobj = PyTuple_GetSlice(args,2,2);
  // PyTuple_GetItem(args,0) would return the object, but then we can't use PyArg_ParseTuple
  // to convert it to a c-type?
  if(variableasobj){
    std::cerr<<"PythonAPI::SetStoreVariable failed to retrieve variable from args"<<std::endl;
    return NULL;
  }
  
  // so we want to deduce the type of the thing being passed to us.
  // first check if it's a list, tuple, set or numpy array
  //bool islist = PyList_Check(variableasobj);
  //bool iscontainer = PyObject_IsInstance(variableasobj, (tuple, list, set, numpy.ndarray));
  // probably the most generically suitable check may be to see if we can iterate over the object.
  // But note: strings in python are also iterable, so check for that explicitly.
  int isiterable = ((PyIter_Check(variableasobj)) && (!PyObject_TypeCheck(&variableasobj, &PyUnicode_Type)));
  
  // if it is, scan through the elements and get their type(s)
  std::string pythontypestring="first";
  if(isiterable){
    // we'll only support vectors for now
    PyObject* iterator = PyObject_GetIter(variableasobj);
    PyObject* item;
    // scan the whole list to check for a commmon datatype: for now we're only going to support
    // std::vector not std::tuple, but in either case we need to know which to use (not tuple by default)
    // TODO add support for std::tuple
    while((item = PyIter_Next(iterator))){
      // determine the type of this element
      PyTypeObject* pythontype = item->ob_type;
      std::string element_type = std::string(pythontype->tp_name);
      if(pythontypestring=="first") pythontypestring = element_type;
      Py_DECREF(item);  // release the reference obtained by PyIter_Next ...
      if(element_type!=pythontypestring){
        std::cerr<<"PythonAPI::SetStoreVariable lists of non-uniform datatypes are not supported"<<std::endl;
        return NULL;
      }
    }
    Py_DECREF(iterator);
  } else {
    // if it's not iterable, just get the object type
    PyTypeObject* pythontype = variableasobj->ob_type;
    pythontypestring = std::string(pythontype->tp_name);
  }
  
//  if (PyErr_Occurred()) {
//    /* propagate error */
//  } else {
//    /* continue doing useful work */
//  }
  
  //std::cout << "PythonAPI::SetStoreVariable received object of type " << pythontypestring << std::endl;
  
  // set the active store:
  const char *storename=nullptr;
  if (!PyArg_ParseTuple(storenameasobj, "s", &storename)) return NULL;
  if(strcmp(storename,"CStore")==0){
    activestore= &(gstore->CStore);
  } else if(strcmp(storename,"vars")==0){
    activestore= &(gstore->vars);
  } else {
    if(gstore->Stores.count(storename)==0){
      std::cerr<<"PythonAPI::GetStoreVariable failed to find requested Store "<<storename<<std::endl;
      return NULL;
    }
    activestore = gstore->Stores.at(storename);
  }
  
  // convert the variable name from Python object
  const char *variablenamechar=nullptr;
  if (!PyArg_ParseTuple(variablenameasobj, "s", &variablenamechar)) return NULL;
  std::string variablename(variablenamechar);
  
  // convert the element object to the appropriate data type and put it in the store
  if(pythontypestring=="int"){
    int tempvar=0;
    return SetStoreVariable(variablename, variableasobj, tempvar, isiterable);
  } else if(pythontypestring=="float"){
    double tempvar=0.;
    return SetStoreVariable(variablename, variableasobj, tempvar, isiterable);
  } else if(pythontypestring=="long"){
    long long tempvar=0;
    return SetStoreVariable(variablename, variableasobj, tempvar, isiterable);
  } else if(pythontypestring=="str"){
    std::string tempvar="";
    return SetStoreVariable(variablename, variableasobj, tempvar, isiterable);
  } else if(pythontypestring=="bool"){
    bool tempvar=false;
    return SetStoreVariable(variablename, variableasobj, tempvar, isiterable);
  } else {
    std::cerr<<"PythonAPI::SetStoreVariable unrecognised variable type "<<pythontypestring<<std::endl;
  }
  return NULL; // shouldn't get here
}

static PyObject* SetStoreInt(PyObject *self, PyObject *args){
  const char *command=nullptr;
  int a=0;
  if (!PyArg_ParseTuple(args, "si", &command, &a)) return NULL;
  activestore->Set(command,a);
  return Py_BuildValue("i", a);

}

static PyObject* SetStoreDouble(PyObject *self, PyObject *args){
  const char *command=nullptr;
  double b=0;
  if (!PyArg_ParseTuple(args, "sd", &command, &b)) return NULL;  
  activestore->Set(command,b);
  return Py_BuildValue("d", b);

}

static PyObject* SetStoreString(PyObject *self, PyObject *args){
  const char *command=nullptr;
  const char *s=nullptr;
  if (!PyArg_ParseTuple(args, "ss", &command, &s)) return NULL;
  std::string a(s);
  activestore->Set(command,a);
  return Py_BuildValue("s", s);

}


static PyMethodDef StoreMethods[] = {
  {"GetInt", GetStoreInt, METH_VARARGS,
   "Return the value of an int in the store"},
  {"GetDouble", GetStoreDouble, METH_VARARGS,
   "Return the value of an int in the store"},
  {"GetString", GetStoreString, METH_VARARGS,
   "Return the value of an int in the store"},
  {"SetInt", SetStoreInt, METH_VARARGS,
   "Set the value of an int in the store"},
  {"SetDouble", SetStoreDouble, METH_VARARGS,
   "Set the value of an int in the store"},
  {"SetString", SetStoreString, METH_VARARGS,
   "Set the value of an int in the store"},
  {"GetStoreVariable", GetStoreVariable, METH_VARARGS,
   "Return the value of a variable in the requested store"},
  {"SetStoreVariable", SetStoreVariable, METH_VARARGS,
   "Set the value of a variable in the requested store"},
  {NULL, NULL, 0, NULL}
};

static struct PyModuleDef StoreModule = {
  PyModuleDef_HEAD_INIT,
  "Store",   /* name of module */
  "Module for accessing BoostStores from python tools", /* module documentation, may be NULL */
  -1,       /* size of per-interpreter state of the module,
	       or -1 if the module keeps state in global variables. */
    StoreMethods
};


#endif
