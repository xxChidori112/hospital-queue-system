FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY include/ ./include/
COPY src/web_server.cpp src/hospital_system.cpp ./src/
COPY web/ ./web/

RUN g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude \
    src/web_server.cpp src/hospital_system.cpp -o hospital_web

ENV HOST=0.0.0.0
ENV PORT=10000
EXPOSE 10000
CMD ["/app/hospital_web"]
