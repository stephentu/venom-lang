/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

Executable::~Executable() {
  util::delete_pointers(instructions.begin(), instructions.end());
  util::delete_pointers(user_func_descs.begin(), user_func_descs.end());
  util::delete_pointers(user_class_objs.begin(), user_class_objs.end());
}

struct constant_table_functor {
  constant_table_functor(util::container_pool<ExecConstant>* exec_const_pool)
    : exec_const_pool(exec_const_pool) {}
  inline Linker::MapTbl operator()(vector<ExecConstant>& consts) {
    Linker::MapTbl ret;
    ret.reserve(consts.size());
    for (size_t i = 0; i < consts.size(); i++) {
      bool create;
      ret.push_back(
        exec_const_pool->create(consts[i], create));
    }
    return ret;
  }
  util::container_pool<ExecConstant>* exec_const_pool;
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

Executable* Linker::link(const ObjCodeVec& objs, size_t mainIdx) {
  assert(!objs.empty());
  VENOM_CHECK_RANGE(mainIdx, objs.size());

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
    objFuncDescVec.reserve(obj->getFuncPool().size());
    for (vector<FunctionSignature>::iterator it = obj->getFuncPool().begin();
         it != obj->getFuncPool().end(); ++it) {
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
        FuncDescMap::iterator it =
          funcDescMap.find(fref.getFullName());
        if (it == funcDescMap.end()) {
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
  vector<Executable::ClassObjVec> localClassObjs(objs.size());
  ClassObjMap classObjMap; // for external class usage
  for (size_t i = 0; i < objs.size(); i++) {
    ObjectCode* obj = objs[i];
    Executable::ClassObjVec& classObjVec = localClassObjs[i];
    FuncDescVec& refTableVec = func_map_tables[i];
    classObjVec.reserve(obj->getClassPool().size());
    for (vector<ClassSignature>::iterator it = obj->getClassPool().begin();
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
  vector<Executable::ClassObjVec> class_map_tables(objs.size());
  for (size_t i = 0; i < objs.size(); i++) {
    ObjectCode* obj = objs[i];
    Executable::ClassObjVec& classObjVec = localClassObjs[i];
    Executable::ClassObjVec& refTableVec = class_map_tables[i];
    refTableVec.reserve(obj->getClassRefTable().size());
    for (size_t i = 0; i < obj->getClassRefTable().size(); i++) {
      SymbolReference& fref = obj->getClassRefTable()[i];
      if (!fref.isLocal()) {
        ClassObjMap::iterator it =
          classObjMap.find(fref.getFullName());
        if (it == classObjMap.end()) {
          throw LinkerException(
              "No external class symbol: " + fref.getFullName());
        }
        refTableVec.push_back(it->second);
      } else {
        VENOM_CHECK_RANGE(fref.getLocalIndex(), classObjVec.size());
        refTableVec.push_back(classObjVec[fref.getLocalIndex()]);
      }
    }
  }

  // translate each obj's constant pool's indices into ExecConstants
  vector<Executable::ConstPool> localConstVec(objs.size());
  for (size_t i = 0; i < objs.size(); i++) {
    ObjectCode* obj = objs[i];
    Executable::ConstPool& execConstVec = localConstVec[i];
    execConstVec.reserve(obj->getConstantPool().size());
    Executable::ClassObjVec& refTableVec = class_map_tables[i];
    for (size_t i = 0; i < obj->getConstantPool().size(); i++) {
      Constant& konst = obj->getConstantPool()[i];
      if (konst.isString()) {
        execConstVec.push_back(ExecConstant(konst.getData()));
      } else {
        VENOM_CHECK_RANGE(konst.getClassIdx(), refTableVec.size());
        execConstVec.push_back(
              ExecConstant(refTableVec[konst.getClassIdx()]));
      }
    }
  }

  // merge constant pools, and create mapping tables
  // for each objcode
  util::container_pool<ExecConstant> exec_const_pool;
  vector<MapTbl> const_map_tables(objs.size());
  transform(localConstVec.begin(), localConstVec.end(),
            const_map_tables.begin(),
            constant_table_functor(&exec_const_pool));

  // calculate total # of instructions
  size_t n_insts = util::foldl(objs.begin(), objs.end(),
                               0, inst_sum_functor());
  // TODO: allocate the entire stream as a contiguous memory array
  // (not just have the pointers being contiguous)
  vector<Instruction*> execInsts(n_insts);

  // resolve instructions into one single stream
  acc = 0;
  for (size_t i = 0; i < objs.size(); i++) {
    ResolutionTable resTbl(&const_map_tables[i],
                           &class_map_tables[i],
                           &func_map_tables[i]);
    ObjectCode::IStream& insts = objs[i]->getInstructions();
    transform(insts.begin(), insts.end(), execInsts.begin() + acc,
              inst_resolver_functor(&resTbl));
    acc += insts.size();
  }

  // grab main address out
  ObjectCode::NameOffsetMap::iterator it =
    objs[mainIdx]->getNameOffsetMap().find("<main>");
  assert(it != objs[mainIdx]->getNameOffsetMap().end());

  return new Executable(
        exec_const_pool.vec,
        Executable::IStream::BuildFrom(execInsts),
        it->second,
        util::flatten_vec(localFuncDescriptors),
        util::flatten_vec(localClassObjs));
}

}
}
