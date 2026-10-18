FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends g++ libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
RUN mkdir -p /var/data
COPY include/ ./include/
COPY src/web_server.cpp src/hospital_system.cpp ./src/
COPY web/ ./web/

RUN g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude \
    src/web_server.cpp src/hospital_system.cpp -o hospital_web -lsqlite3

ENV HOST=0.0.0.0
ENV PORT=10000
ENV HOSPITAL_DB=/var/data/hospital.db
EXPOSE 10000
CMD ["/app/hospital_web"]
