error=0
for file in *.exp
do
    expect $file || error=1
done

exit $error