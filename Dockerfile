# syntax=docker/dockerfile:1

FROM debian:trixie-slim AS builder

# Install build-time and test dependencies
# ruby is only needed for testing
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    meson \
    ruby && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy dependency files first to utilize layer caching for subprojects
COPY subprojects ./subprojects/
COPY meson.build ./
RUN meson subprojects download

COPY . .

RUN meson setup build --buildtype=release --strip && \
    meson compile -C build airspace && \
    meson test -C build --print-errorlogs && \
    meson install -C build --destdir /app/install --no-rebuild --skip-subprojects


FROM debian:trixie-slim AS runtime

RUN useradd -m appuser
USER appuser

COPY --from=builder /app/install/usr/local/bin/airspace /usr/local/bin/airspace

ENTRYPOINT ["/usr/local/bin/airspace"]
CMD ["--help"]
