error=0
results=()

nntp_address="${nntp_address:-localhost}"

for file in tests/*.exp
do
    echo "Run test $file"
    nntp_address=$nntp_address expect $file
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
