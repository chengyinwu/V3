echo "Running aiger.dofile ..."
if [ "$1" == "-stdout" ]; then
   ../../bin/v3 -f aiger.dofile
else
   ../../bin/v3 -f aiger.dofile > aiger.result
fi

echo "Running rtl.dofile ..."
if [ "$1" == "-stdout" ]; then
   ../../bin/v3 -f rtl.dofile
else
   ../../bin/v3 -f rtl.dofile > rtl.result
fi
