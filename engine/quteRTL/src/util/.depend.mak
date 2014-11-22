bddManager.o: bddManager.cpp bddNode.h bddManager.h bddHash.h bv4.h bv2.h \
  myString.h myTruthT.h
bddNode.o: bddNode.cpp bddNode.h bddManager.h bddHash.h bv4.h bv2.h \
  myString.h myTruthT.h
bv2.o: bv2.cpp bv4.h bv2.h myString.h
myCout.o: myCout.cpp myCout.h myString.h bv2.h bv4.h
myGetChar.o: myGetChar.cpp
myString.o: myString.cpp myString.h
myTruthT.o: myTruthT.cpp myTruthT.h myString.h
nameMapBstNode.o: nameMapBstNode.cpp nameMapBstNode.h nameMapNode.h
nameMap.o: nameMap.cpp nameMap.h nameMapBstNode.h nameMapNode.h
nameMapNode.o: nameMapNode.cpp nameMapNode.h
utilString.o: utilString.cpp
