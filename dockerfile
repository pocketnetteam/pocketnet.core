FROM debian:stable-slim

ENV POCKETCOIN_DATA=/home/pocketcoin/.pocketcoin
EXPOSE 37070 37071 38081 8087

RUN useradd -r pocketcoin \
  && apt-get update -y

RUN mkdir -p "$POCKETCOIN_DATA"
RUN chmod 700 "$POCKETCOIN_DATA"
RUN chown -R pocketcoin "$POCKETCOIN_DATA"
USER pocketcoin

WORKDIR "$POCKETCOIN_DATA"
COPY ./src/release/usr/local/* /usr/local/

ENTRYPOINT ["pocketcoind"]