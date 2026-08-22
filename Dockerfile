FROM debian:bookworm-slim AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends gcc libc6-dev libmicrohttpd-dev libcjson-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY backend/inventory-api.c backend/inventory-api.c
RUN gcc -O2 -Wall -Wextra -o nexstock-backend backend/inventory-api.c \
    -lmicrohttpd -lcjson

FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends libmicrohttpd12 libcjson1 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /app/nexstock-backend ./nexstock-backend
COPY frontend ./frontend
RUN mkdir -p ./backend
COPY backend/inventory.json ./backend/inventory.json

ENV PORT=8040
EXPOSE 8040
CMD ["./nexstock-backend"]
