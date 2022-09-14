/****************************************************************************
  FileName     [ v3BtorParser.cpp ]
  PackageName  [ v3/src/io ]
  Synopsis     [ BTOR to V3 Network Parser. ]
  Author       [ Cheng-Yin Wu ]
  Copyright    [ Copyright(c) 2012-2014 DVLab, GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef V3_BTOR_PARSER_C
#define V3_BTOR_PARSER_C

#include "v3Msg.h"
#include "v3NtkUtil.h"
#include "v3StrUtil.h"
#include "v3NtkParser.h"

#include <fstream>
#include <string>

// As far as I know there is a discrepancy between sim and solver values over V3_MODULE gates. 
// However, I cannot assure all implementations (simulation, sat-solving, bit-blast) are definitely correct.
// Please disable this macro after a thorough regression and debugging is developed. 
//#define V3_BTOR_DISABLE_DIV_MODULE_UNTIL_STABLE

// Define
typedef V3Vec<string>::Vec    V3BtorLexVec;

// Parser Helper Functions
bool lexLineBTOR(ifstream& input, V3BtorLexVec& tokens) {
   assert (input.is_open()); tokens.clear();
   string buffer = "";
   size_t i, j;
   while (!input.eof()) {
      getline(input, buffer); if (input.eof()) break;
      for (i = j = 0; i < buffer.size(); ++i) {
         if (buffer[i] == ' ' || buffer[i] == '\t') {
            if (i > j) tokens.push_back(buffer.substr(j, i - j));
            j = i + 1;
         }
         else if (buffer[i] == '\n' || buffer[i] == ';') {  // ";" is Comment Symbol in BTOR
            if (i > j) tokens.push_back(buffer.substr(j, i - j));
            j = i + 1; break;
         }
      }
      if (i > j) tokens.push_back(buffer.substr(j));
      if (tokens.size()) return true;
   }
   return false;
}

inline const V3NetId getBtorInput(const V3HashMap<uint32_t, V3NetId>::Hash& id2IdHash, const int& id) {
   assert (id2IdHash.end() != id2IdHash.find((uint32_t)((id < 0) ? -id : id)));
   return (id > 0) ? id2IdHash.find(id)->second : ~(id2IdHash.find(-id)->second);
}

// Main BTOR Parsing Function
V3NtkInput* const V3BtorParser(const char* fileName, const bool& symbol) {
   // Open Input BTOR File
   assert (fileName); ifstream input; input.open(fileName);
   if (!input.is_open()) { Msg(MSG_ERR) << "BTOR Input File \"" << fileName << "\" Not Found !!" << endl; return 0; }
   // Create Network Handler
   V3NtkInput* btorHandler = new V3NtkInput(false, fileName);
   if (!btorHandler) { Msg(MSG_ERR) << "Create BTOR Parser Failed !!" << endl; return 0; }
   if (!btorHandler->getNtk()) { Msg(MSG_ERR) << "Create BTOR Network Failed !!" << endl; return 0; }
   V3BvNtk* const ntk = dynamic_cast<V3BvNtk*>(btorHandler->getNtk()); assert (ntk);
   // Start Parsing
   V3HashMap<uint32_t, V3NetId>::Hash id2IdHash;
   V3NetId outNet, inNet1, inNet2, inNet3;
   V3BtorLexVec tokens; tokens.clear();
   int id, width, op1, op2, op3;
   string op = "";
   bool processing = true;
   
   while (1) {
      processing = lexLineBTOR(input, tokens); if (!processing) break; assert (tokens.size());
      // id
      if (!v3Str2Int(tokens[0], id) || (id <= 0)) {
         Msg(MSG_ERR) << "Line start with non-positive-integer id \"" << tokens[0] << "\" !" << endl; break;
      }
      // op
      if (tokens.size() == 1) {
         Msg(MSG_ERR) << "Missing operator type for variable " << id << " !" << endl; break;
      }
      op = tokens[1];
      // width
      if (tokens.size() == 2) {
         Msg(MSG_ERR) << "Missing width specification for variable " << id << " !" << endl; break;
      }
      if ((!v3Str2Int(tokens[2], width)) || (!(width > 0))) {
         Msg(MSG_ERR) << "Variable " << id << " has non-positive width \"" << tokens[2] << "\" !" << endl; break;
      }
      // Create Output Net (Excluding FF and Root)
      if (op != "next" && op != "root") {
         outNet = btorHandler->createNet(symbol ? "" : tokens[0], width);
         if (V3NetUD == outNet) break; id2IdHash.insert(make_pair((uint32_t)id, outNet));
      }
      // Create Corresponding Gates
      if (op == "var") {
         if (symbol && tokens.size() > 3) 
            if (!btorHandler->resetNetName(outNet.id, tokens[3])) { delete btorHandler; return 0; }
      }
      else if ( // 1 operand operators
            op == "constd" || op == "consth" || op == "const" || op == "root" || op == "one" || 
            op == "ones" || op == "zero" || op == "not" || op == "neg" || op == "inc" || 
            op == "dec" || op == "redand" || op == "redor" || op == "redxor" 
            )
      {
         if (tokens.size() < 4) {
            Msg(MSG_ERR) << "Missing operand 1 for variable " << id << " (" << op << ") !" << endl; break;
         }
         if (op == "constd") {
            if (!createBvConstGate(ntk, outNet, v3Int2Str(width) + "'d" + tokens[3])) break;
            else continue;
         }
         else if (op == "consth") {
            if (!createBvConstGate(ntk, outNet, v3Int2Str(width) + "'h" + tokens[3])) break;
            else continue;
         }
         else if (op == "const") {
            if (!createBvConstGate(ntk, outNet, v3Int2Str(width) + "'b" + tokens[3])) break;
            else continue;
         }
         else if (op == "one") {  // constant 1 : id op width
            if (!createBvConstGate(ntk, outNet, v3Int2Str(width) + "'b1")) break;
         }
         else if (op == "zero") {  // constant 0 : id op width
            if (!createBvConstGate(ntk, outNet, v3Int2Str(width) + "'b0")) break;
         }
         else if (op == "ones") {  // constant -1 : id op width
            if (!createBvConstGate(ntk, outNet, v3Int2Str(width) + "'b" + string(width, '1'))) break;
         }
         else if (!v3Str2Int(tokens[3], op1)) {
            Msg(MSG_ERR) << "Variable " << id << " has non-integer operand \"" << tokens[3] << "\" !" << endl; break;
         }
         // Create Input Net for One Operand Gate
         inNet1 = getBtorInput(id2IdHash, op1); if (V3NetUD == inNet1) break;
         if (op == "root") { // assertion : id root width op1
            //if (width != 1) { Msg(MSG_ERR) << "Bit-width of root should be 1 !" << endl; break; }
            if (!createOutput(ntk, inNet1)) break;
            btorHandler->resetOutName(ntk->getOutputSize() - 1, 
                                       (symbol && tokens.size() > 4) ? tokens[4] : tokens[0]);
         }
         else if (op == "not") {  // inverter : id not width op1
            if (!createV3BufGate(ntk, outNet, getV3InvertNet(inNet1))) break;
         }
         else if (op == "neg") {  // negative : id neg width op1
            inNet2 = btorHandler->createNet("", width); if (V3NetUD == inNet2) break;
            if (!createBvConstGate(ntk, inNet2, v3Int2Str(width) + "'b1")) break;
            if (!createBvPairGate(ntk, BV_ADD, outNet, inNet2, getV3InvertNet(inNet1))) break;
         }
         else if (op == "inc") {  // increment : id inc width op1
            inNet2 = btorHandler->createNet("", width); if (V3NetUD == inNet2) break;
            if (!createBvConstGate(ntk, inNet2, v3Int2Str(width) + "'b1")) break;
            if (!createBvPairGate(ntk, BV_ADD, outNet, inNet2, inNet1)) break;
         }
         else if (op == "dec") {  // decrement : id dec width op1
            inNet2 = btorHandler->createNet("", width); if (V3NetUD == inNet2) break;
            if (!createBvConstGate(ntk, inNet2, v3Int2Str(width) + "'b1")) break;
            if (!createBvPairGate(ntk, BV_SUB, outNet, inNet1, inNet2)) break;
         }
         else if (op == "redand" || op == "redor" || op == "redxor") {
            if (width != 1) {
               Msg(MSG_ERR) << "Reduced variable " << id << " has width more than 1 !" << endl; break;
            }
            if (op == "redand") {  // reduced AND : id redand 1 op1
               if (!createBvReducedGate(ntk, BV_RED_AND, outNet, inNet1)) break;
            }
            else if (op == "redor") {  // reduced OR : id redor 1 op1
               if (!createBvReducedGate(ntk, BV_RED_OR, outNet, inNet1)) break;
            }
            else if (op == "redxor") {  // reduced XOR : id redxor 1 op1
               if (!createBvReducedGate(ntk, BV_RED_XOR, outNet, inNet1)) break;
            }
            else assert (0);
         }
         else assert (0);
      }
      else if ( // 2 operand operators
            op == "next" || 
            op == "and" || op == "or" || op == "xor" || op == "nand" || op == "nor" || 
            op == "xnor" || op == "add" || op == "sub" || op == "mul" ||
#ifndef V3_BTOR_DISABLE_DIV_MODULE_UNTIL_STABLE
            op == "udiv" || op == "urem" || op == "sdiv" || op == "srem" ||
#endif
            op == "eq" || op == "ne" || op == "ult" || op == "ulte" ||
            op == "ugt" || op == "ugte" || op == "sll" || op == "srl" || op == "sra" || op == "concat"
            )
      {
         if (tokens.size() < 4) {
            Msg(MSG_ERR) << "Missing operand 1 for variable " << id << " (" << op << ") !" << endl; break;
         }
         else if (!v3Str2Int(tokens[3], op1)) {
            Msg(MSG_ERR) << "Variable " << id << " has non-integer operand \"" << tokens[3] << "\" !" << endl; break;
         }
         if (tokens.size() < 5) {
            Msg(MSG_ERR) << "Missing operand 2 for variable " << id << " (" << op << ") !" << endl; break;
         }
         else if (!v3Str2Int(tokens[4], op2)) {
            Msg(MSG_ERR) << "Variable " << id << " has non-integer operand \"" << tokens[4] << "\" !" << endl; break;
         }
         // Create Input Nets for Two Operand Gates
         inNet1 = getBtorInput(id2IdHash, op1); if (V3NetUD == inNet1) break;
         inNet2 = getBtorInput(id2IdHash, op2); if (V3NetUD == inNet2) break;
         // Special Operations to FF
         if (op == "next") {
            inNet3 = btorHandler->createNet("", width); if (V3NetUD == inNet3) break;
            if (!createBvConstGate(ntk, inNet3, v3Int2Str(width) + "'d0")) break;
            if (!createV3FFGate(ntk, inNet1, inNet2, inNet3)) break;
         }
         else if (op == "and") { if (!createBvPairGate(ntk, BV_AND, outNet, inNet1, inNet2)) break; }
         else if (op == "or") { if (!createBvPairGate(ntk, BV_OR, outNet, inNet1, inNet2)) break; }
         else if (op == "xor") { if (!createBvPairGate(ntk, BV_XOR, outNet, inNet1, inNet2)) break; }
         else if (op == "nand") { if (!createBvPairGate(ntk, BV_NAND, outNet, inNet1, inNet2)) break; }
         else if (op == "nor") { if (!createBvPairGate(ntk, BV_NOR, outNet, inNet1, inNet2)) break; }
         else if (op == "xnor") { if (!createBvPairGate(ntk, BV_XNOR, outNet, inNet1, inNet2)) break; }
         else if (op == "add") { if (!createBvPairGate(ntk, BV_ADD, outNet, inNet1, inNet2)) break; }
         else if (op == "sub") { if (!createBvPairGate(ntk, BV_SUB, outNet, inNet1, inNet2)) break; }
         else if (op == "mul") { if (!createBvPairGate(ntk, BV_MULT, outNet, inNet1, inNet2)) break; }
         else if (op == "udiv") { if (!createBvPairGate(ntk, BV_DIV, outNet, inNet1, inNet2)) break; }
         else if (op == "urem") { if (!createBvPairGate(ntk, BV_MODULO, outNet, inNet1, inNet2)) break; }
         else if (op == "sll") { if (!createBvPairGate(ntk, BV_SHL, outNet, inNet1, inNet2)) break; }
         else if (op == "srl") { if (!createBvPairGate(ntk, BV_SHR, outNet, inNet1, inNet2)) break; }
         else if (op == "concat") { if (!createBvPairGate(ntk, BV_MERGE, outNet, inNet1, inNet2)) break; }
         else if (op == "eq" || op == "ne" || op == "ult" || op == "ulte" || op == "ugt" || op == "ugte") {
            if (width != 1) {
               Msg(MSG_ERR) << "Comparator variable " << id << " has width more than 1 !" << endl; break;
            }
            if (op == "eq") { if (!createBvPairGate(ntk, BV_EQUALITY, outNet, inNet1, inNet2)) break; }
            else if (op == "ne") { if (!createBvPairGate(ntk, BV_NEQ, outNet, inNet1, inNet2)) break; }
            else if (op == "ult") { if (!createBvPairGate(ntk, BV_LESS, outNet, inNet1, inNet2)) break; }
            else if (op == "ulte") { if (!createBvPairGate(ntk, BV_LEQ, outNet, inNet1, inNet2)) break; }
            else if (op == "ugt") { if (!createBvPairGate(ntk, BV_GREATER, outNet, inNet1, inNet2)) break; }
            else if (op == "ugte") { if (!createBvPairGate(ntk, BV_GEQ, outNet, inNet1, inNet2)) break; }
            else assert (0);
         }
         else if (op == "sra") {  // Ari(a >> b) = MSB(a) ? ~((~a) >> b) : (a >> b);
            const V3NetId falseNet = btorHandler->createNet("", width); if (V3NetUD == falseNet) break;
            if (!createBvPairGate(ntk, BV_SHR, falseNet, inNet1, inNet2)) break;
            const V3NetId trueNet = btorHandler->createNet("", width); if (V3NetUD == trueNet) break;
            if (!createBvPairGate(ntk, BV_SHR, trueNet, ~inNet1, inNet2)) break;
            inNet3 = btorHandler->createNet("", 1); if (V3NetUD == inNet3) break;  // MSB of inNet1
            if (!createBvSliceGate(ntk, inNet3, inNet1, width - 1, width - 1)) break;
            if (!createBvMuxGate(ntk, outNet, falseNet, ~trueNet, inNet3)) break;
         }
         else if (op == "sdiv") {
            if (1 == width) { if (!createBvPairGate(ntk, BV_OR, outNet, inNet1, ~inNet2)) break; }
            else {
               // Sign Bits
               V3NetId sign1 = btorHandler->createNet("", 1); if (V3NetUD == sign1) break;
               V3NetId sign2 = btorHandler->createNet("", 1); if (V3NetUD == sign2) break;
               if (!createBvSliceGate(ntk, sign1, inNet1, width - 1, width - 1)) break;
               if (!createBvSliceGate(ntk, sign2, inNet2, width - 1, width - 1)) break;
               V3NetId sign_xor = btorHandler->createNet("", 1); if (V3NetUD == sign_xor) break;
               if (!createBvPairGate(ntk, BV_XOR, sign_xor, sign1, sign2)) break;
               // Division
               V3NetId const1 = btorHandler->createNet("", width); if (V3NetUD == const1) break;
               if (!createBvConstGate(ntk, const1, v3Int2Str(width) + "'d1")) break;
               V3NetId negNet1 = btorHandler->createNet("", width); if (V3NetUD == negNet1) break;
               V3NetId negNet2 = btorHandler->createNet("", width); if (V3NetUD == negNet2) break;
               if (!createBvPairGate(ntk, BV_ADD, negNet1, ~inNet1, const1)) break;
               if (!createBvPairGate(ntk, BV_ADD, negNet2, ~inNet2, const1)) break;
               V3NetId inNetCond1 = btorHandler->createNet("", width); if (V3NetUD == inNetCond1) break;
               V3NetId inNetCond2 = btorHandler->createNet("", width); if (V3NetUD == inNetCond2) break;
               if (!createBvMuxGate(ntk, inNetCond1, inNet1, negNet1, sign1)) break;
               if (!createBvMuxGate(ntk, inNetCond2, inNet2, negNet2, sign2)) break;
               V3NetId udivNet = btorHandler->createNet("", width); if (V3NetUD == udivNet) break;
               if (!createBvPairGate(ntk, BV_DIV, udivNet, inNetCond1, inNetCond2)) break;
               V3NetId neg_udivNet = btorHandler->createNet("", width); if (V3NetUD == neg_udivNet) break;
               if (!createBvPairGate(ntk, BV_ADD, neg_udivNet, ~udivNet, const1)) break;
               if (!createBvMuxGate(ntk, outNet, udivNet, neg_udivNet, sign_xor)) break;
            }
         }
         else if (op == "srem") {
            if (1 == width) { if (!createBvPairGate(ntk, BV_OR, outNet, ~inNet1, inNet2)) break; }
            else {
               // Sign Bits
               V3NetId sign1 = btorHandler->createNet("", 1); if (V3NetUD == sign1) break;
               V3NetId sign2 = btorHandler->createNet("", 1); if (V3NetUD == sign2) break;
               if (!createBvSliceGate(ntk, sign1, inNet1, width - 1, width - 1)) break;
               if (!createBvSliceGate(ntk, sign2, inNet2, width - 1, width - 1)) break;
               // Modulo
               V3NetId const1 = btorHandler->createNet("", width); if (V3NetUD == const1) break;
               if (!createBvConstGate(ntk, const1, v3Int2Str(width) + "'d1")) break;
               V3NetId negNet1 = btorHandler->createNet("", width); if (V3NetUD == negNet1) break;
               V3NetId negNet2 = btorHandler->createNet("", width); if (V3NetUD == negNet2) break;
               if (!createBvPairGate(ntk, BV_ADD, negNet1, ~inNet1, const1)) break;
               if (!createBvPairGate(ntk, BV_ADD, negNet2, ~inNet2, const1)) break;
               V3NetId inNetCond1 = btorHandler->createNet("", width); if (V3NetUD == inNetCond1) break;
               V3NetId inNetCond2 = btorHandler->createNet("", width); if (V3NetUD == inNetCond2) break;
               if (!createBvMuxGate(ntk, inNetCond1, inNet1, negNet1, sign1)) break;
               if (!createBvMuxGate(ntk, inNetCond2, inNet2, negNet2, sign2)) break;
               V3NetId uremNet = btorHandler->createNet("", width); if (V3NetUD == uremNet) break;
               if (!createBvPairGate(ntk, BV_MODULO, uremNet, inNetCond1, inNetCond2)) break;
               V3NetId neg_uremNet = btorHandler->createNet("", width); if (V3NetUD == neg_uremNet) break;
               if (!createBvPairGate(ntk, BV_ADD, neg_uremNet, ~uremNet, const1)) break;
               if (!createBvMuxGate(ntk, outNet, uremNet, neg_uremNet, sign1)) break;
            }
         }
         else assert (0);
      }
      else if ( // More than 2 operands
            op == "cond" || op == "slice"
            )
      {
         if (tokens.size() < 4) {
            Msg(MSG_ERR) << "Missing operand 1 for variable " << id << " (" << op << ") !" << endl; break;
         }
         else if (!v3Str2Int(tokens[3], op1)) {
            Msg(MSG_ERR) << "Variable " << id << " has non-integer operand \"" << tokens[3] << "\" !" << endl; break;
         }
         if (tokens.size() < 5) {
            Msg(MSG_ERR) << "Missing operand 2 for variable " << id << " (" << op << ") !" << endl; break;
         }
         else if (!v3Str2Int(tokens[4], op2)) {
            Msg(MSG_ERR) << "Variable " << id << " has non-integer operand \"" << tokens[4] << "\" !" << endl; break;
         }
         if (tokens.size() < 6) {
            Msg(MSG_ERR) << "Missing operand 3 for variable " << id << " (" << op << ") !" << endl; break;
         }
         else if (!v3Str2Int(tokens[5], op3)) {
            Msg(MSG_ERR) << "Variable " << id << " has non-integer operand \"" << tokens[5] << "\" !" << endl; break;
         }
         if (op == "cond") {  // multiplexer : id cond width op_select op_true op_false
            // Create Input Nets for BV_MUX  Gates
            inNet1 = getBtorInput(id2IdHash, op1); if (V3NetUD == inNet1) break;
            inNet2 = getBtorInput(id2IdHash, op2); if (V3NetUD == inNet2) break;
            inNet3 = getBtorInput(id2IdHash, op3); if (V3NetUD == inNet3) break;
            if (!createBvMuxGate(ntk, outNet, inNet3, inNet2, inNet1)) break;
         }
         else if (op == "slice") {  // split : id slice width op1 op_msb op_lsb
            // Create Input Nets for BV_MUX  Gates
            inNet1 = getBtorInput(id2IdHash, op1); if (V3NetUD == inNet1) break;
            if (!createBvSliceGate(ntk, outNet, inNet1, op2, op3)) break;
         }
         else assert (0);
      }
      else {
         Msg(MSG_ERR) << "Unknown or unhandled operator \"" << op << "\" !" << endl; break;
      }
   }
   if (processing) { delete btorHandler; btorHandler = 0; }
   else btorHandler->renderFreeNetAsInput(); return btorHandler;
}

#endif

