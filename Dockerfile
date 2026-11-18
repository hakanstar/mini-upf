# Build stage
FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends gcc make libc6-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY Makefile ./
COPY include ./include
COPY src ./src
RUN make

# Runtime stage
FROM debian:bookworm-slim
COPY --from=build /src/build/mini-upf /usr/local/bin/mini-upf
EXPOSE 2152/udp
ENTRYPOINT ["/usr/local/bin/mini-upf"]
