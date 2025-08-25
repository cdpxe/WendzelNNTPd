check_returncode() {
    actual=$1
    expected=$2
    if [ "$actual" -ne "$expected" ]
    then
        echo "Failure"
        echo "Expected returncode $expected but was $actual"
        echo "Output:"
        echo "$3"
        exit 1
    fi
}

check_output() {
    actual=$1
    expected=$2
    if ! echo "$actual" | grep -q -- "$expected"
    then
        echo "Failure"
        echo
        echo "Expected:"
        echo "$expected"
        echo
        echo "Actual:"
        echo "$actual"
        exit 1
    fi
}

echo "== Test help output without command =="
output=$(wendzelnntpadm 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "usage: wendzelnntpd <command> \[parameters\]"

echo "== Test output for invalid command =="
output=$(wendzelnntpadm invalid invalid 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "Invalid mode: invalid'."

echo "== Test listgroups command =="
output=$(wendzelnntpadm listgroups 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "Newsgroup, Low-, High-Value, Posting-Flag"
check_output "$output" "-----------------------------------------"
check_output "$output" "alt.wendzelnntpd.test 2 1 y"
check_output "$output" "alt.wendzelnntpd.test.empty 0 0 y"
check_output "$output" "alt.wendzelnntpd.test.post 0 0 y"
check_output "$output" "done."

echo "== Test addgroup command =="
output=$(wendzelnntpadm addgroup newgroup n 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "Newsgroup newgroup does not exist. Creating new group."
check_output "$output" "done."

output=$(wendzelnntpadm listgroups 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "Newsgroup, Low-, High-Value, Posting-Flag"
check_output "$output" "-----------------------------------------"
check_output "$output" "alt.wendzelnntpd.test 2 1 y"
check_output "$output" "alt.wendzelnntpd.test.empty 0 0 y"
check_output "$output" "alt.wendzelnntpd.test.post 0 0 y"
check_output "$output" "newgroup 0 0 n"
check_output "$output" "done."

echo "== Test addgroup command but group already exists =="
output=$(wendzelnntpadm addgroup newgroup n 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "Newsgroup newgroup already exists."

echo "== Test modgroup command for existing group =="
output=$(wendzelnntpadm modgroup newgroup y 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "Newsgroup newgroup exists: okay."
check_output "$output" "done."

output=$(wendzelnntpadm listgroups 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "Newsgroup, Low-, High-Value, Posting-Flag"
check_output "$output" "-----------------------------------------"
check_output "$output" "alt.wendzelnntpd.test 2 1 y"
check_output "$output" "alt.wendzelnntpd.test.empty 0 0 y"
check_output "$output" "alt.wendzelnntpd.test.post 0 0 y"
check_output "$output" "newgroup 0 0 y"
check_output "$output" "done."

echo "== Test modgroup command for non existing group =="
output=$(wendzelnntpadm modgroup invalid y 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "Newsgroup invalid does not exists."

output=$(wendzelnntpadm listgroups 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "Newsgroup, Low-, High-Value, Posting-Flag"
check_output "$output" "-----------------------------------------"
check_output "$output" "alt.wendzelnntpd.test 2 1 y"
check_output "$output" "alt.wendzelnntpd.test.empty 0 0 y"
check_output "$output" "alt.wendzelnntpd.test.post 0 0 y"
check_output "$output" "done."

echo "== Test delgroup command for existing group =="
output=$(wendzelnntpadm delgroup newgroup y 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "Newsgroup newgroup exists: okay."
check_output "$output" "Clearing association class ... done"
check_output "$output" "Clearing ACL associations of newsgroup newgroup... done"
check_output "$output" "Clearing ACL role associations of newsgroup newgroup... done"
check_output "$output" "Deleting newsgroup newgroup itself ... done"
check_output "$output" "Cleanup: Deleting postings that do not belong to an existing newsgroup ... done"
check_output "$output" "done."

echo "== Test delgroup command for non existing group =="
output=$(wendzelnntpadm delgroup invalid y 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "Newsgroup invalid does not exists."

echo "== Test listusers command =="
output=$(wendzelnntpadm listusers 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "Username, Password"
check_output "$output" "------------------"
check_output "$output" "testuser, 1c039cb73df5bad8a0fe7711e2447c2b72c8d9ff8dc53d75d201aa9491fc1756"
check_output "$output" "testuser2, b6e30ea30cc1f9e3742e9dc98c7c6b5fb10c102dc325791535306821115c877a"
check_output "$output" "done."

echo "== Test adduser command =="
output=$(wendzelnntpadm adduser newuser password 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "User newuser does currently not exist: okay."
check_output "$output" "done."

output=$(wendzelnntpadm listusers 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "Username, Password"
check_output "$output" "------------------"
check_output "$output" "testuser, 1c039cb73df5bad8a0fe7711e2447c2b72c8d9ff8dc53d75d201aa9491fc1756"
check_output "$output" "testuser2, b6e30ea30cc1f9e3742e9dc98c7c6b5fb10c102dc325791535306821115c877a"
check_output "$output" "newuser, dd45e54135eea3309d8b63369e2bfac468a33ff12d40015adf1ba44c50807760"
check_output "$output" "done."

echo "== Test adduser command but user already exists =="
output=$(wendzelnntpadm adduser newuser password 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "User newuser does already exists."

echo "== Test deluser command for existing user =="
output=$(wendzelnntpadm deluser newuser y 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "User newuser exists: okay."
check_output "$output" "Clearing ACL associations of user newuser... done"
check_output "$output" "Clearing ACL role associations of user newuser... done"
check_output "$output" "Deleting user newuser from database ... done"
check_output "$output" "done."

output=$(wendzelnntpadm listusers 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "Username, Password"
check_output "$output" "------------------"
check_output "$output" "testuser, 1c039cb73df5bad8a0fe7711e2447c2b72c8d9ff8dc53d75d201aa9491fc1756"
check_output "$output" "testuser2, b6e30ea30cc1f9e3742e9dc98c7c6b5fb10c102dc325791535306821115c877a"
check_output "$output" "done."

echo "== Test deluser command for non existing user =="
output=$(wendzelnntpadm deluser invalid y 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "User invalid does not exists."

echo "== Test listacl command =="
output=$(wendzelnntpadm listacl 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "List of roles in database:"
check_output "$output" "Roles"
check_output "$output" "-----"
check_output "$output" "testrole"
check_output "$output" "Connections between users and roles:"
check_output "$output" "Role, User"
check_output "$output" "----------"
check_output "$output" "testrole, testuser"
check_output "$output" "Username, Has access to group"
check_output "$output" "-----------------------------"
check_output "$output" "testuser, alt.wendzelnntpd.test"
check_output "$output" "testuser2, alt.wendzelnntpd.test.empty"
check_output "$output" "Role, Has access to group"
check_output "$output" "-------------------------"
check_output "$output" "testrole, alt.wendzelnntpd.test.post"
check_output "$output" "done."

echo "== Test addacluser command =="
output=$(wendzelnntpadm addacluser testuser alt.wendzelnntpd.test.post 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "User testuser exists: okay"
check_output "$output" "Newsgroup alt.wendzelnntpd.test.post exists: okay"
check_output "$output" "done."

output=$(wendzelnntpadm listacl 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "List of roles in database:"
check_output "$output" "Roles"
check_output "$output" "-----"
check_output "$output" "testrole"
check_output "$output" "Connections between users and roles:"
check_output "$output" "Role, User"
check_output "$output" "----------"
check_output "$output" "testrole, testuser"
check_output "$output" "Username, Has access to group"
check_output "$output" "-----------------------------"
check_output "$output" "testuser, alt.wendzelnntpd.test"
check_output "$output" "testuser2, alt.wendzelnntpd.test.empty"
check_output "$output" "testuser, alt.wendzelnntpd.test.post"
check_output "$output" "Role, Has access to group"
check_output "$output" "-------------------------"
check_output "$output" "testrole, alt.wendzelnntpd.test.post"
check_output "$output" "done."

echo "== Test addacluser command but newsgroup does not exist =="
output=$(wendzelnntpadm addacluser testuser invalid 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "User testuser exists: okay"
check_output "$output" "Newsgroup invalid does not exists."

echo "== Test addacluser command but user does not exist =="
output=$(wendzelnntpadm addacluser invalid alt.wendzelnntpd.test.post 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "User invalid does not exists."

echo "== Test delacluser command for existing user and newsgroup =="
output=$(wendzelnntpadm delacluser testuser alt.wendzelnntpd.test.post 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "User testuser exists: okay"
check_output "$output" "Newsgroup alt.wendzelnntpd.test.post exists: okay."
check_output "$output" "done."

output=$(wendzelnntpadm listacl 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "List of roles in database:"
check_output "$output" "Roles"
check_output "$output" "-----"
check_output "$output" "testrole"
check_output "$output" "Connections between users and roles:"
check_output "$output" "Role, User"
check_output "$output" "----------"
check_output "$output" "testrole, testuser"
check_output "$output" "Username, Has access to group"
check_output "$output" "-----------------------------"
check_output "$output" "testuser, alt.wendzelnntpd.test"
check_output "$output" "testuser2, alt.wendzelnntpd.test.empty"
check_output "$output" "Role, Has access to group"
check_output "$output" "-------------------------"
check_output "$output" "testrole, alt.wendzelnntpd.test.post"
check_output "$output" "done."

echo "== Test delacluser command but newsgroup does not exist =="
output=$(wendzelnntpadm delacluser testuser invalid 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "User testuser exists: okay"
check_output "$output" "Newsgroup invalid does not exists."

echo "== Test delacluser command but user does not exist =="
output=$(wendzelnntpadm delacluser invalid alt.wendzelnntpd.test.post 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "User invalid does not exists."

echo "== Test addaclrole command =="
output=$(wendzelnntpadm addaclrole newrole 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "Role newrole does not exists: okay"
check_output "$output" "done."

output=$(wendzelnntpadm listacl 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "List of roles in database:"
check_output "$output" "Roles"
check_output "$output" "-----"
check_output "$output" "testrole"
check_output "$output" "newrole"
check_output "$output" "Connections between users and roles:"
check_output "$output" "Role, User"
check_output "$output" "----------"
check_output "$output" "testrole, testuser"
check_output "$output" "Username, Has access to group"
check_output "$output" "-----------------------------"
check_output "$output" "testuser, alt.wendzelnntpd.test"
check_output "$output" "testuser2, alt.wendzelnntpd.test.empty"
check_output "$output" "Role, Has access to group"
check_output "$output" "-------------------------"
check_output "$output" "testrole, alt.wendzelnntpd.test.post"
check_output "$output" "done."

echo "== Test addaclrole command but role already exists =="
output=$(wendzelnntpadm addaclrole newrole 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "Role newrole already exists."

echo "== Test delaclrole command for existing role =="
output=$(wendzelnntpadm delaclrole newrole 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "Role newrole exists: okay."
check_output "$output" "Removing associations of role newrole with their users ... don"
check_output "$output" "Removing associations of role newrole with their newsgroups ... done"
check_output "$output" "Removing role newrole ... done"
check_output "$output" "done."

output=$(wendzelnntpadm listacl 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "List of roles in database:"
check_output "$output" "Roles"
check_output "$output" "-----"
check_output "$output" "testrole"
check_output "$output" "Connections between users and roles:"
check_output "$output" "Role, User"
check_output "$output" "----------"
check_output "$output" "testrole, testuser"
check_output "$output" "Username, Has access to group"
check_output "$output" "-----------------------------"
check_output "$output" "testuser, alt.wendzelnntpd.test"
check_output "$output" "testuser2, alt.wendzelnntpd.test.empty"
check_output "$output" "Role, Has access to group"
check_output "$output" "-------------------------"
check_output "$output" "testrole, alt.wendzelnntpd.test.post"
check_output "$output" "done."

echo "== Test delaclrole command for non existing role =="
output=$(wendzelnntpadm delaclrole invalid 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "Role invalid does not exists."

echo "== Test rolegroupconnect command =="
output=$(wendzelnntpadm rolegroupconnect testrole alt.wendzelnntpd.test.empty 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "Role testrole exists: okay"
check_output "$output" "Newsgroup alt.wendzelnntpd.test.empty exists: okay."
check_output "$output" "Connecting role testrole with newsgroup alt.wendzelnntpd.test.empty ... done"
check_output "$output" "done."

output=$(wendzelnntpadm listacl 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "List of roles in database:"
check_output "$output" "Roles"
check_output "$output" "-----"
check_output "$output" "testrole"
check_output "$output" "Connections between users and roles:"
check_output "$output" "Role, User"
check_output "$output" "----------"
check_output "$output" "testrole, testuser"
check_output "$output" "Username, Has access to group"
check_output "$output" "-----------------------------"
check_output "$output" "testuser, alt.wendzelnntpd.test"
check_output "$output" "testuser2, alt.wendzelnntpd.test.empty"
check_output "$output" "Role, Has access to group"
check_output "$output" "-------------------------"
check_output "$output" "testrole, alt.wendzelnntpd.test.post"
check_output "$output" "testrole, alt.wendzelnntpd.test.empty"
check_output "$output" "done."

echo "== Test rolegroupconnect command but newsgroup does not exist =="
output=$(wendzelnntpadm rolegroupconnect testrole invalid 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "Role testrole exists: okay"
check_output "$output" "Newsgroup invalid does not exists."

echo "== Test rolegroupconnect command but role does not exist =="
output=$(wendzelnntpadm rolegroupconnect invalid alt.wendzelnntpd.test.empty 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "Role invalid does not exists."

echo "== Test rolegroupdisconnect command for existing role and newsgroup =="
output=$(wendzelnntpadm rolegroupdisconnect testrole alt.wendzelnntpd.test.empty 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "Role testrole exists: okay"
check_output "$output" "Newsgroup alt.wendzelnntpd.test.empty exists: okay."
check_output "$output" "Dis-Connecting role testrole from newsgroup alt.wendzelnntpd.test.empty ... done"
check_output "$output" "done."

output=$(wendzelnntpadm listacl 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "List of roles in database:"
check_output "$output" "Roles"
check_output "$output" "-----"
check_output "$output" "testrole"
check_output "$output" "Connections between users and roles:"
check_output "$output" "Role, User"
check_output "$output" "----------"
check_output "$output" "testrole, testuser"
check_output "$output" "Username, Has access to group"
check_output "$output" "-----------------------------"
check_output "$output" "testuser, alt.wendzelnntpd.test"
check_output "$output" "testuser2, alt.wendzelnntpd.test.empty"
check_output "$output" "Role, Has access to group"
check_output "$output" "-------------------------"
check_output "$output" "testrole, alt.wendzelnntpd.test.post"
check_output "$output" "done."

echo "== Test rolegroupdisconnect command but newsgroup does not exist =="
output=$(wendzelnntpadm rolegroupdisconnect testrole invalid 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "Role testrole exists: okay"
check_output "$output" "Newsgroup invalid does not exists."

echo "== Test rolegroupdisconnect command but role does not exist =="
output=$(wendzelnntpadm rolegroupdisconnect invalid alt.wendzelnntpd.test.empty 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "Role invalid does not exists."

echo "== Test roleuserconnect command =="
output=$(wendzelnntpadm roleuserconnect testrole testuser2 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "Role testrole exists: okay"
check_output "$output" "User testuser2 exists: okay."
check_output "$output" "Connecting role testrole with user testuser2 ... done"
check_output "$output" "done."

output=$(wendzelnntpadm listacl 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "List of roles in database:"
check_output "$output" "Roles"
check_output "$output" "-----"
check_output "$output" "testrole"
check_output "$output" "Connections between users and roles:"
check_output "$output" "Role, User"
check_output "$output" "----------"
check_output "$output" "testrole, testuser"
check_output "$output" "testrole, testuser2"
check_output "$output" "Username, Has access to group"
check_output "$output" "-----------------------------"
check_output "$output" "testuser, alt.wendzelnntpd.test"
check_output "$output" "testuser2, alt.wendzelnntpd.test.empty"
check_output "$output" "Role, Has access to group"
check_output "$output" "-------------------------"
check_output "$output" "testrole, alt.wendzelnntpd.test.post"
check_output "$output" "done."

echo "== Test roleuserconnect command but user does not exist =="
output=$(wendzelnntpadm roleuserconnect testrole invalid 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "Role testrole exists: okay"
check_output "$output" "User invalid does not exists."

echo "== Test roleuserconnect command but role does not exist =="
output=$(wendzelnntpadm roleuserconnect invalid testuser2 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "Role invalid does not exists."

echo "== Test roleuserdisconnect command for existing role and iser =="
output=$(wendzelnntpadm roleuserdisconnect testrole testuser2 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "Role testrole exists: okay"
check_output "$output" "User testuser2 exists: okay."
check_output "$output" "Dis-Connecting role testrole from user testuser2 ... done"
check_output "$output" "done."

output=$(wendzelnntpadm listacl 2>&1)
returncode=$?

check_returncode $returncode 0 "$output"
check_output "$output" "List of roles in database:"
check_output "$output" "Roles"
check_output "$output" "-----"
check_output "$output" "testrole"
check_output "$output" "Connections between users and roles:"
check_output "$output" "Role, User"
check_output "$output" "----------"
check_output "$output" "testrole, testuser"
check_output "$output" "Username, Has access to group"
check_output "$output" "-----------------------------"
check_output "$output" "testuser, alt.wendzelnntpd.test"
check_output "$output" "testuser2, alt.wendzelnntpd.test.empty"
check_output "$output" "Role, Has access to group"
check_output "$output" "-------------------------"
check_output "$output" "testrole, alt.wendzelnntpd.test.post"
check_output "$output" "done."

echo "== Test roleuserdisconnect command but user does not exist =="
output=$(wendzelnntpadm roleuserdisconnect testrole invalid 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "Role testrole exists: okay"
check_output "$output" "User invalid does not exists."

echo "== Test roleuserdisconnect command but role does not exist =="
output=$(wendzelnntpadm roleuserdisconnect invalid testuser2 2>&1)
returncode=$?

check_returncode $returncode 1 "$output"
check_output "$output" "Role invalid does not exists."
