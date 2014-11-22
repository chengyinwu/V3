echo "Running aiger.dofile ..."
if [ "$1" == "-stdout" ]; then
   ../../bin/v3 -f aiger.dofile
else
   ../../bin/v3 -f aiger.dofile > aiger.result
fi

echo "Running btor.dofile ..."
if [ "$1" == "-stdout" ]; then
   ../../bin/v3 -f btor.dofile
else
   ../../bin/v3 -f btor.dofile > btor.result
fi
