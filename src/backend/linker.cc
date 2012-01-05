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
      ret.push_back(
        exec_const_pool->create(obj->getConstantPool()[i]));
    }
    return ret;
  }
  util::container_pool<string>* exec_const_pool;
};

struct func_table_functor {
  func_table_functor(const Linker::FuncDescMap& func_desc_map)
    : func_desc_map(func_desc_map) {}
  inline FuncDescVec operator()(ObjectCode* obj) {
    FuncDescVec ret;
    ret.reserve(obj->getFuncRefTable().size());
    for (size_t i = 0; i < obj->getFuncRefTable().size(); i++) {
      SymbolReference& fref = obj->getFuncRefTable()[i];
      if (!fref.isLocal()) {
        Linker::FuncDescMap::iterator it =
          func_desc_map.find(fref.getFullName());
        if (it == func_desc_map.end()) {
          throw LinkerException(
              "No external function symbol: " + fref.getFullName());
        }
        ret.push_back(it->second);
      } else {
        // TODO: implement me
        VENOM_UNIMPLEMENTED;
      }
    }
    return ret;
  }
  Linker::FuncDescMap func_desc_map;
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
  // merge constant pools, and create mapping tables
  // for each objcode
  util::container_pool<string> exec_const_pool;
  vector<MapTbl> const_map_tables(objs.size());
  transform(objs.begin(), objs.end(), const_map_tables.begin(),
            constant_table_functor(&exec_const_pool));

  vector<FuncDescVec> func_map_tables(objs.size());
  // TODO: non-builtin functions
  transform(objs.begin(), objs.end(), func_map_tables.begin(),
            func_table_functor(builtin_function_map));

  // calculate total # of instructions
  size_t n_insts = util::foldl(objs.begin(), objs.end(),
                               0, inst_sum_functor());
  // TODO: allocate the entire stream as a contiguous memory array
  // (not just have the pointers being contiguous)
  Executable::IStream::array_type execInsts = new Instruction* [n_insts];

  // resolve instructions into one single stream
  size_t acc = 0;
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
