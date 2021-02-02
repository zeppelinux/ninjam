FROM ubuntu:latest

ENV PATH /usr/local/ninjam:$PATH
ADD https://github.com/justinfrankel/ninjam/archive/master.zip /var/tmp/

WORKDIR /var/tmp/ninjam/server
RUN make
RUN mkdir -p /usr/local/ninjam
RUN mv /var/tmp/ninjam/server/ninjamsrv /usr/local/ninjam
RUN mv /var/tmp/ninjam/server/example.cfg /usr/local/ninjam/config.cfg
RUN mv /var/tmp/ninjam/server/cclicense.txt /usr/local/ninjam/cclicense.txt

# forward request and error logs to docker log collector
#RUN ln -sf /dev/stdout ./ninjamserver.log

EXPOSE 2049
ENTRYPOINT ["ninjamsrv", "config.cfg"]