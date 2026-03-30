FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8
ENV TZ=Etc/UTC

RUN apt-get update && apt-get install -y --no-install-recommends \
    bash \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    file \
    git \
    libncurses-dev \
    locales \
    make \
    ninja-build \
    pkg-config \
    python3 \
    python3-pip \
    python3-venv \
    rsync \
    unzip \
    wget \
    xz-utils \
    zip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY . /workspace

RUN chmod +x /workspace/software/TuyaOS/build_app.sh \
    /workspace/software/TuyaOS/prepare_app.sh \
    /workspace/software/TuyaOS/vendor/T5/t5_os/build.sh \
    /workspace/docker/entrypoint.sh

ENV TUYA_ROOT=/workspace/software/TuyaOS
ENV TUYA_APP_NAME=SuperT
ENV TUYA_APP_VERSION=1.0.0
ENV PATH=/workspace/software/TuyaOS/vendor/T5/toolchain/gcc-arm-none-eabi-10.3-2021.10/bin:${PATH}

WORKDIR /workspace/software/TuyaOS

ENTRYPOINT ["/workspace/docker/entrypoint.sh"]
CMD ["bash"]