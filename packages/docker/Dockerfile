FROM debian:stable-slim as build-env

# install build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    bison \
    flex \
    gcc \
    libmariadb-dev-compat \
    libmariadb-dev \
    libmhash-dev \
    libsqlite3-dev \
    libssl-dev \
    make \
    openssl \
    sqlite3

COPY . /wendzelnntpd
WORKDIR /wendzelnntpd

RUN ./configure && make && make install CREATE_CERTIFICATES=NO


FROM debian:stable-slim

# install runtime dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    libmariadb3 \
    libmhash2 \
    libsqlite3-0 \
    libssl3 \
    openssl \
    tini \
	&& apt-get clean \
	&& rm -rf /var/lib/apt/lists/*

# copy the binaries and other required data from the build environment
COPY --from=build-env /usr/local/sbin/wendzelnntpd /usr/local/sbin/
COPY --from=build-env /usr/local/sbin/wendzelnntpadm /usr/local/sbin/
COPY --from=build-env /usr/local/sbin/create_certificate /usr/local/sbin/
COPY --from=build-env /usr/local/share/wendzelnntpd/openssl.cnf /usr/local/share/wendzelnntpd/
COPY --from=build-env /usr/local/etc/wendzelnntpd/wendzelnntpd.conf /usr/local/etc/wendzelnntpd/
COPY --from=build-env /var/spool/news/wendzelnntpd/usenet.db /var/spool/news/wendzelnntpd/

COPY packages/docker/wendzelnntpd.conf /usr/local/etc/wendzelnntpd/
COPY packages/docker/docker-entrypoint.sh /

RUN touch /var/log/wendzelnntpd

VOLUME /var/spool/news/wendzelnntpd /usr/local/etc/wendzelnntpd

EXPOSE 119
EXPOSE 563

ENTRYPOINT [ "tini", "--", "/docker-entrypoint.sh" ]
CMD [ "wendzelnntpd" ]
