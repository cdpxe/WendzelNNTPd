FROM debian:stable-slim

RUN apt -y update && apt -y install wget gcc make telnet

RUN wget https://core.tcl-lang.org/tcl/tarball/release/tcl.tar.gz
RUN tar -xvf tcl.tar.gz
RUN cd tcl/unix && ./configure && make && make install

RUN cd .. && wget https://sourceforge.net/projects/expect/files/Expect/5.45.4/expect5.45.4.tar.gz
RUN tar -xvf expect5.45.4.tar.gz
RUN cd expect5.45.4 && ./configure && make && make install