error=0
results=()

nntpd_address="${nntpd_address:-localhost}"

for file in tests/*.exp
do
    echo "Run test $file"
    nntpd_address=$nntpd_address expect $file
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
