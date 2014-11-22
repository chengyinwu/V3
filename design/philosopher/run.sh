echo "Running aiger.dofile ..."
if [ "$1" == "-stdout" ]; then
   ../../v3 -f aiger.dofile
else
   ../../v3 -f aiger.dofile > aiger.result
fi

echo "Running rtl.dofile ..."
if [ "$1" == "-stdout" ]; then
   ../../v3 -f rtl.dofile
else
   ../../v3 -f rtl.dofile > rtl.result
fi
