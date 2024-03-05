FROM ubuntu:20.04

ENV POCKETCORE_HOME=/home/pocketcore
ENV POCKETCOIN_DATA=${POCKETCORE_HOME}/.pocketcoin

EXPOSE 37070 37071 38081 38881 8087 8887 36060 36061 39091 39991 39092 6067 6667

RUN groupadd --gid 1000 pocketcore && useradd --uid 1000 --gid pocketcore --shell /bin/bash --create-home pocketcore
RUN apt-get update -y && apt-get install -y sqlite3

RUN mkdir -p ${POCKETCOIN_DATA}
RUN chmod 700 ${POCKETCOIN_DATA}
RUN chown -R pocketcore:pocketcore ${POCKETCOIN_DATA}
RUN ls -la ${POCKETCOIN_DATA}
USER pocketcore

COPY ./release/usr/local/ /usr/local/

WORKDIR ${POCKETCORE_HOME}
ENTRYPOINT ["pocketcoind"]