FROM wendzelnntpd-base:latest

COPY .. /my-app
WORKDIR /my-app

# disable IPv6 as default because not enabled by default
RUN sed -i 's/listen	::1/;listen	::1/g' wendzelnntpd.conf
# listen to all IP addresses for reaching from host system
RUN sed -i 's/listen	127.0.0.1/;listen	127.0.0.1/g' wendzelnntpd.conf
RUN sed -i 's/;listen	0.0.0.0/listen	0.0.0.0/g' wendzelnntpd.conf
# default activate verbose mode for development
RUN sed -i 's/;verbose-mode/verbose-mode/g' wendzelnntpd.conf
RUN touch /var/log/wendzelnntpd

RUN ./configure
RUN make
RUN make install

ENTRYPOINT /usr/local/sbin/wendzelnntpd