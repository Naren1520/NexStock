FROM debian:bookworm-slim AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends gcc libc6-dev pkg-config ca-certificates libmicrohttpd-dev libcjson-dev libmongoc-dev libbson-dev libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY backend/inventory-api.c backend/inventory-api.c
RUN gcc -O2 -Wall -Wextra -o nexstock-backend backend/inventory-api.c \
    $(pkg-config --cflags --libs libmongoc-1.0 libbson-1.0) -lmicrohttpd -lcjson -lcrypto -lpthread

FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates libmicrohttpd12 libcjson1 libmongoc-1.0-0 libbson-1.0-0 libssl3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /app/nexstock-backend ./nexstock-backend
COPY frontend ./frontend
RUN mkdir -p ./backend

ENV PORT=8040
EXPOSE 8040
CMD ["./nexstock-backend"]
