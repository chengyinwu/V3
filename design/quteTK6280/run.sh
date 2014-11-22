echo "Running aiger.dofile ..."
if [ "$1" == "-stdout" ]; then
   ../../v3 -f aiger.dofile
else
   ../../v3 -f aiger.dofile > aiger.result
fi

echo "Running btor.dofile ..."
if [ "$1" == "-stdout" ]; then
   ../../v3 -f btor.dofile
else
   ../../v3 -f btor.dofile > btor.result
fi
