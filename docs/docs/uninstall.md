# Uninstallation

WendzelNNTPd can be uninstalled with `make uninstall` if you installed it from source.
`./configure` needs to be called with the same parameters as during installation.
You need superuser access to uninstall WendzelNNTPd.
It is recommended to use the same version of the sources of WendzelNNTPd as the one installed.
The configuration files, SQLite database, postings and log files are left in place to prevent data loss.
You need to remove them manually if desired.

```console
$ ./configure
$ sudo make uninstall
```
