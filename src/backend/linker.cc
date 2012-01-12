#include <algorithm>

#include <backend/linker.h>
#include <backend/vm.h>

#include <util/container.h>
#include <util/macros.h>
#include <util/stl.h>

using namespace std;
using namespace venom::runtime;

namespace venom {
namespace backend {

struct constant_table_functor {
  constant_table_functor(util::container_pool<string>* exec_const_pool)
    : exec_const_pool(exec_const_pool) {}
  inline Linker::MapTbl operator()(ObjectCode* obj) {
    Linker::MapTbl ret;
    ret.reserve(obj->getConstantPool().size());
    for (size_t i = 0; i < obj->getConstantPool().size(); i++) {
      bool create;
      ret.push_back(
        exec_const_pool->create(obj->getConstantPool()[i], create));
    }
    return ret;
  }
  util::container_pool<string>* exec_const_pool;
};

struct inst_resolver_functor {
  inst_resolver_functor(ResolutionTable* resTable)
    : resTable(resTable), pos(0) {}
  inline Instruction* operator()(SymbolicInstruction* i) {
    return i->resolve(pos++, *resTable);
  }
  ResolutionTable* resTable;
  size_t pos;
};

struct inst_sum_functor {
  inline size_t operator()(size_t accum, ObjectCode* obj) {
    return accum + obj->getInstructions().size();
  }
};

Executable* Linker::link(const ObjCodeVec& objs) {
  // go through each local function in each obj (in order),
  // and create FunctionDescriptors, which point to the
  // global offset in the executable instruction stream

  // local func descs, for each obj
  vector<FuncDescVec> localFuncDescriptors(objs.size());
  FuncDescMap funcDescMap; // for external user symbols
  size_t acc = 0;
  for (size_t i = 0; i < objs.size(); i++) {
    ObjectCode* obj = objs[i];
    ObjectCode::IStream& insts = obj->getInstructions();
    FuncDescVec& objFuncDescVec = localFuncDescriptors[i];
    objFuncDescVec.reserve(obj->getFuncPool().begin());
    for (vector<FunctionSignature>::iterator it = obj->getFuncPool().begin();
         it != obj->getFuncPool()->end(); ++it) {
      FunctionDescriptor *desc = it->createFuncDescriptor(acc);
      objFuncDescVec.push_back(desc);
      // TODO: assert that this is a *new* entry
      funcDescMap[it->getFullName(obj->getModuleName())] = desc;
    }
    acc += insts.size();
  }

  // merge the builtin symbols into the user symbols
  // TODO: assert all new entries
  funcDescMap.insert(builtin_function_map.begin(),
                     builtin_function_map.end());

  // create the func ref table for each object
  vector<FuncDescVec> func_map_tables(objs.size());
  for (size_t i = 0; i < objs.size(); i++) {
    ObjectCode* obj = objs[i];
    FuncDescVec& localFuncDesc = localFuncDescriptors[i];
    FuncDescVec& refTableVec = func_map_tables[i];
    refTableVec.reserve(obj->getFuncRefTable().size());
    for (size_t i = 0; i < obj->getFuncRefTable().size(); i++) {
      SymbolReference& fref = obj->getFuncRefTable()[i];
      if (!fref.isLocal()) {
        Linker::FuncDescMap::iterator it =
          func_desc_map.find(fref.getFullName());
        if (it == func_desc_map.end()) {
          throw LinkerException(
              "No external function symbol: " + fref.getFullName());
        }
        refTableVec.push_back(it->second);
      } else {
        VENOM_CHECK_RANGE(fref.getLocalIndex(), localFuncDesc.size());
        refTableVec.push_back(localFuncDesc[fref.getLocalIndex()]);
      }
    }
  }

  // go through each local class in each obj,
  // and create class objs
  typedef vector<venom_class_object*> ClassObjVec;
  typedef map<string, venom_class_object*> ClassObjMap;
  vector<ClassObjVec> localClassObjs(objs.size());
  ClassObjMap classObjMap; // for external class usage
  for (size_t i = 0; i < objs.size(); i++) {
    ObjectCode* obj = objs[i];
    ClassObjVec& classObjVec = localClassObjs[i];
    FuncDescVec& refTableVec = func_map_tables[i];
    classObjVec.reserve(obj->getClassPool().size());
    for (vector<ClassSignature>::iterator it = obj->getClassPool().size();
         it != obj->getClassPool().end(); ++it) {
      venom_class_object* classObj = it->createClassObject(refTableVec);
      classObjVec.push_back(classObj);
      classObjMap[it->getFullName(obj->getModuleName())] = classObj;
    }
  }

  // merge builtin classes into classObjMap
  classObjMap.insert(builtin_class_map.begin(), builtin_class_map.end());
  // TODO: assert all new entries

  // create the class ref table for each object

  // merge constant pools, and create mapping tables
  // for each objcode
  util::container_pool<Constant> exec_const_pool;
  vector<MapTbl> const_map_tables(objs.size());

  // TODO: we want to map all class idxs in consts to the global
  // class index number, before doing the transformation

  transform(objs.begin(), objs.end(), const_map_tables.begin(),
            constant_table_functor(&exec_const_pool));



  // calculate total # of instructions
  size_t n_insts = util::foldl(objs.begin(), objs.end(),
                               0, inst_sum_functor());
  // TODO: allocate the entire stream as a contiguous memory array
  // (not just have the pointers being contiguous)
  Executable::IStream::array_type execInsts = new Instruction* [n_insts];

  // resolve instructions into one single stream
  acc = 0;
  for (size_t i = 0; i < objs.size(); i++) {
    ResolutionTable resTbl(&const_map_tables[i], NULL, &func_map_tables[i]);
    ObjectCode::IStream& insts = objs[i]->getInstructions();
    transform(insts.begin(), insts.end(), execInsts + acc,
              inst_resolver_functor(&resTbl));
    acc += insts.size();
  }

  return new Executable(exec_const_pool.vec,
                        Executable::ClassObjPool(NULL, 0),
                        Executable::IStream(execInsts, n_insts));
}

}
}
