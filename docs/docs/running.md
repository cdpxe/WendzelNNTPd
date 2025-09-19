# Starting and Running WendzelNNTPd

## Starting the Service

Once your WendzelNNTPd installation has been configured, you can run the
server (in the default case you need superuser access to do that since
this is required to bind WendzelNNTPd to the default NNTP port 119) by
starting **/usr/local/sbin/wendzelnntpd**.

    $ /usr/local/sbin/wendzelnntpd 
    WendzelNNTPd: version 2.0.7 'Berlin'  - (Oct
    26 2015 14:10:20 #2544) is ready.

**Note (Daemon Mode):** If you want to run WendzelNNTPd as a background
daemon process on \*nix-like operating systems, you should use the
parameter **-d**.

## Stopping and Restarting the Service

The server can be stopped by terminating its process:

    $ pkill wendzelnntpd

The server has a handler for the termination signal that allows to
safely shutdown using **pkill** or **kill**.

To restart the service, terminate and start the service.

### Automating Start, Stop, and Restart

The script **init.d_script** in the directory *scripts/startup* of the
tarball can be used to start, restart, and stop WendzelNNTPd. It is a
standard *init.d* script for Linux operating systems that can usually be
copied to */etc/init.d* (it must be executable).

    $ cp scripts/startup/init.d_script /etc/init.d/wendzelnntpd
    $ chmod +x /etc/init.d/wendzelnntpd

**Note:** Please note that some operating systems use different
directories than */etc/init.d* or other startup script formats. In such
cases, the script works nevertheless but should simply be installed to
*/usr/local/sbin* instead.

To start, stop, and restart WendzelNNTPd, the following commands can be
used afterwards:

    $ /etc/init.d/wendzelnntpd start
    Starting WendzelNNTPd ... done.
    WendzelNNTPd: version 2.0.7 'Berlin'  - (Oct
    26 2015 14:10:20 #2544) is ready.

    $ /etc/init.d/wendzelnntpd restart
    Stopping WendzelNNTPd ... done.
    Starting WendzelNNTPd ... done.
    WendzelNNTPd: version 2.0.7 'Berlin'  - (Oct
    26 2015 14:10:20 #2544) is ready.

    $ /etc/init.d/wendzelnntpd stop
    Stopping WendzelNNTPd ... done.

## Administration Tool 'wendzelnntpadm'

Use the command line tool **wendzelnntpadm** to configure users, roles
and newsgroups of your WendzelNNTPd installation. To get an overview of
supported commands, run "wendzelnntpadm help":

    $ wendzelnntpadm help
    usage: wendzelnntpd <command> [parameters]
    *** Newsgroup Administration:
     <listgroups>
     <addgroup | modgroup> <newsgroup> <posting-allowed-flag (y/n)>
     <delgroup> <newsgroup>
    *** User Administration:
     <listusers>
     <adduser> <username> [<password>]
     <deluser> <username>
    *** ACL (Access Control List) Administration:
     <listacl>
     <addacluser | delacluser> <username> <newsgroup>
     <addaclrole | delaclrole> <role>
     <rolegroupconnect | rolegroupdisconnect> <role> <newsgroup>
     <roleuserconnect | roleuserdisconnect> <role> <username>

## Creating/Listing/Deleting Newsgroups

You can either list, create or delete newsgroups using
**wendzelnntpadm**.

### Listing existing newsgroups

    $ wendzelnntpadm listgroups
    Newsgroup, Low-, High-Value, Posting-Flag
    -----------------------------------------
    alt.test 10 1 y
    mgmt.talk 1 1 y
    secret.project-x 20 1 y
    done.

### Creating a new newsgroup

To create a new newsgroup run the following command:

    $ wendzelnntpadm addgroup my.cool.group y
    Newsgroup my.cool.group does not exist. Creating new group.
    done.

You can also change the "posting allowed" flag of a newsgroup:

    $ wendzelnntpadm modgroup my.cool.group y
    Newsgroup my.cool.group exists: okay.
    done.
    $ wendzelnntpadm modgroup my.cool.group n
    Newsgroup my.cool.group exists: okay.
    done.

### Deleting a newsgroup

    $ wendzelnntpadm delgroup my.cool.group
    Newsgroup my.cool.group exists: okay.
    Clearing association class ... done
    Clearing ACL associations of newsgroup my.cool.group... done
    Clearing ACL role associations of newsgroup my.cool.group... done
    Deleting newsgroup my.cool.group itself ... done
    Cleanup: Deleting postings that do not belong to an existing newsgroup ... done
    done.

## User Accounts Administration

The easiest way to give only some people access to your server is to
create user accounts (please make sure you activated authentication in
your configuration file). You can add, delete and list all users.

### Listing Users (and Passwords)

This command always prints the (hashed) password of the users:

    $ wendzelnntpadm listusers
    Username, Password
    ------------------
    developer1, wegerhgrhtrthjtzj
    developer2, erghnrehhnht
    manager1, wegergergrhth
    manager2, thnthnrothnht
    swendzel, lalalegergreg
    swendzel2, 94j5z5jh5th
    swendzel3, lalalalala
    swendzel4, wegwegwegwegweg
    done.

### Creating a new user

You can either enter the password as additional parameter (useful for
scripts that create users automatically) \...

    $ wendzelnntpadm adduser UserName HisPassWord
    User UserName does currently not exist: okay.
    done.

\... or you can type it using the prompt (in this case the input is
shadowed):

    $ wendzelnntpadm adduser UserName2
    Enter new password for this user (max. 100 chars):
    User UserName2 does currently not exist: okay.
    done.

**Please Note:** A password must include at least 8 characters and may
not include more than 100 characters.

### Deleting an existing user

    $ wendzelnntpadm deluser UserName2
    User UserName2 exists: okay.
    Clearing ACL associations of user UserName2... done
    Clearing ACL role associations of user UserName2... done
    Deleting user UserName2 from database ... done
    done.

## Access Control List Administration (in case the standard NNTP authentication is not enough)

Welcome to the advanced part of WendzelNNTPd. WendzelNNTPd includes a
powerful role based access control system. You can either only use
normal access control lists where you can configure which user will have
access to which newsgroup. Or you can use the advanced role system: You
can add users to roles (e.g., the user "boss99" to the role
"management") and give a role access to a group (e.g., role "management"
shall have access to "discuss.management").

**Note:** Please note that you must activate the ACL feature in your
configuration file to use it.

**Note:** To see *all* data related to the ACL subsystem of your
WendzelNNTPd installation, simply use "wendzelnntpadm listacl".

### Invisible Newsgroups

WendzelNNTPd includes a feature called "Invisible Newsgroups" which
means that a user without access to a newsgroup will neither see the
newsgroup in the list of newsgroups, nor will he be able to post to such
a newsgroup or will be able to read it.

### Simple Access Control

We start with the simple access control component where you can define
which user will have access to which newsgroup.

#### Giving a user access to a newsgroup

    $ wendzelnntpadm addacluser swendzel alt.test
    User swendzel exists: okay.
    Newsgroup alt.test exists: okay.
    done.
    $ wendzelnntpadm listacl
    List of roles in database:
    Roles
    -----

    Connections between users and roles:
    Role, User
    ----------

    Username, Has access to group
    -----------------------------
    swendzel, alt.test

    Role, Has access to group
    -------------------------
    done.

#### Removing a user's access to a newsgroup

    $ wendzelnntpadm delacluser swendzel alt.test
    User swendzel exists: okay.
    Newsgroup alt.test exists: okay.
    done.

### Adding and Removing ACL Roles

If you have many users, some of them should have access to the same
newsgroup (e.g., the developers of a new system should have access to
the development newsgroup of the system). With roles you do not have to
give every user explicit access to such a group. Instead you add the
users to a role and give the role access to the group. (One advantage is
that you can easily give the complete role access to another group with
only one command instead of adding each of its users to the list of
people who have access to the new group).

In the following examples, we give the users "developer1", "developer2",
and "developer3" access to the development role of "project-x" and
connect their role to the newsgroups "project-x.discussion" and
"project-x.support". To do so, we create the three users and the two
newsgroups first:

    $ wendzelnntpadm adduser developer1
    Enter new password for this user (max. 100 chars):
    User developer1 does currently not exist: okay.
    done.
    $ wendzelnntpadm adduser developer2
    Enter new password for this user (max. 100 chars):
    User developer2 does currently not exist: okay.
    done.
    $ wendzelnntpadm adduser developer3
    Enter new password for this user (max. 100 chars):
    User developer3 does currently not exist: okay.
    done.

    $ wendzelnntpadm addgroup project-x.discussion y
    Newsgroup project-x.discussion does not exist. Creating new group.
    done.
    $ wendzelnntpadm addgroup project-x.support y
    Newsgroup project-x.support does not exist. Creating new group.
    done.

#### Creating an ACL Role

Before you can add users to a role and before you can connect a role to
a newsgroup, you have to create an ACL *role* (you have to choose an
ASCII name for it). In this example, the group is called "project-x".

    $ wendzelnntpadm addaclrole project-x
    Role project-x does not exists: okay.
    done.

#### Deleting an ACL Role

You can delete an ACL role by using "delaclrole" instead of "addaclrole"
like in the example above.

### Connecting and Disconnecting Users with/from Roles

To add (connect) or remove (disconnect) a user to/from a role, you need
to use the admin tool too.

#### Connecting a User with a Role

The second parameter ("project-x") is the role name and the third
parameter ("developer1") is the username. Here we add our three
developer users from the example above to the group project-x:

    $ wendzelnntpadm roleuserconnect project-x developer1
    Role project-x exists: okay.
    User developer1 exists: okay.
    Connecting role project-x with user developer1 ... done
    done.
    $ wendzelnntpadm roleuserconnect project-x developer2
    Role project-x exists: okay.
    User developer2 exists: okay.
    Connecting role project-x with user developer2 ... done
    done.
    $ wendzelnntpadm roleuserconnect project-x developer3
    Role project-x exists: okay.
    User developer3 exists: okay.
    Connecting role project-x with user developer3 ... done
    done.

#### Disconnecting a User from a Role

    $ wendzelnntpadm roleuserdisconnect project-x developer1
    Role project-x exists: okay.
    User developer1 exists: okay.
    Dis-Connecting role project-x from user developer1 ... done
    done.

### Connecting and Disconnecting Roles with/from Newsgroups

Even if a role is connected with a set of users, it is still useless
until you connect the role with a newsgroup.

#### Connecting a Role with a Newsgroup

To connect a role with a newsgroup, we have to use the command line tool
for a last time (the 2nd parameter is the role, and the 3rd parameter is
the name of the newsgroup). Here we connect our "project-x" role to the
two newsgroups "project-x.discussion" and "project-x.support":

    $ wendzelnntpadm rolegroupconnect project-x project-x.discussion
    Role project-x exists: okay.
    Newsgroup project-x.discussion exists: okay.
    Connecting role project-x with newsgroup project-x.discussion ... done
    done.
    $ wendzelnntpadm rolegroupconnect project-x project-x.support
    Role project-x exists: okay.
    Newsgroup project-x.support exists: okay.
    Connecting role project-x with newsgroup project-x.support ... done
    done.

#### Disconnecting a Role From a Newsgroup

Disconnecting is done like in the example above but you have to use the
command "rolegroup**dis**connect" instead of "rolegroupconnect".

### Listing Your Whole ACL Configuration Again

Like mentioned before, you can use the command "listacl" to list your
whole ACL configuration (except the list of users that will be listed by
the command "listusers").

    $ wendzelnntpadm listacl
    List of roles in database:
    Roles
    -----
    project-x

    Connections between users and roles:
    Role, User
    ----------
    project-x, developer1
    project-x, developer2
    project-x, developer3

    Username, Has access to group
    -----------------------------
    swendzel, alt.test

    Role, Has access to group
    -------------------------
    project-x, project-x.discussion
    project-x, project-x.support
    done.

#### Saving time

As mentioned above, you can safe time by using roles. For instance, if
you add a new developer to the system, and the developer should have
access to the two groups "project-x.discussion" and "project-x.support",
you do not have to assign the user to both (or even more) groups by
hand. Instead, you just add the user to the role "project-x" that is
already connected to both groups.

If you want to give all developers access to the group "project-x.news",
you also do not have to connect every developer with the project.
Instead, you just connect the role with the newsgroup, what is one
command instead of $n$ commands. Of course, this time-saving concept
also works if you want to delete a user.

## Hardening

Besides the already mentioned authentication, ACL and RBAC features, the
security of the server can be improved by putting WendzelNNTPd in a
*chroot* environment or letting it run under an unprivileged user
account (the user then needs write access to
*/var/spool/news/wendzelnntpd* and read access to
(*/usr/local)/etc/wendzelnntpd.conf*). An unprivileged user under
Unix-like systems is also not able to create a listen socket on the
default NNTP port (119) since all ports up to 1023 are usually[^1]
reserved. This means that the server should use a port
$\geq$`<!-- -->`{=html}1024 if it is started by a non-root user.

Please also note that WendzelNNTPd can be easily identified due to its
welcoming 'banner' (desired code '200' message of NNTP). Tools such as
`nmap` provide rules to identify WendzelNNTPd and its version this way.
Theoretically, this could be changed by a slight code modification
(welcome message, HELP output and other components that make the server
identifiable). However, I do not recommend this as it is just a form of
'security by obscurity'.

[^1]: Some \*nix-like systems may have a different range of privileged
    ports.
