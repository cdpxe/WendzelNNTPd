error=0
results=()

for file in *.exp
do
    echo "Run test $file"
    expect $file
    result=$?
    if [ $result -ne 0 ]
    then
        error=1
    fi
    results+=("$file: $result")
    echo ""
done

echo "Test results in the format <filename>: <returncode>"
printf '%s\n' "${results[@]}"

exit $error
