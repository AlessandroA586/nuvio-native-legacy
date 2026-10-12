# PC-side lab in a container (Linux, or macOS with OrbStack/Docker Desktop).
#   docker build -t nuvio-dts-lab tools/dts-tizen-lab
#   docker run --rm -p 8000:8000 -v "$PWD/movies:/media" nuvio-dts-lab /media/Movie.mkv
#   docker run --rm --entrypoint python3 nuvio-dts-lab host_tests.py
FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
      build-essential git ca-certificates curl xz-utils python3 ffmpeg \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /lab
COPY build.sh tvconv.c nvlab_server.py make_fixtures.sh host_tests.py tv_runner.py ./
RUN sh build.sh
EXPOSE 8000
ENTRYPOINT ["python3", "/lab/nvlab_server.py", "--port", "8000"]
