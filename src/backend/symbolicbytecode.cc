#include <backend/codegenerator.h>
#include <backend/symbolicbytecode.h>

using namespace std;

namespace venom {
namespace backend {

void SInstLabel::printDebug(ostream& o) {
  o << Instruction::stringify(opcode) << " label_" << value->getIndex()
    << endl;
}

}
}
