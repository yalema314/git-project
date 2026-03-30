# t5-e1

## 容器镜像构建与上传

仓库根目录已经提供了 `Dockerfile`、`.dockerignore` 和 `Makefile`，默认使用 `podman` 来构建、上传、下载和运行镜像。

### 需要你自己填写的变量

以下变量定义在 `Makefile` 中，可以在执行 `make` 时覆盖。

- `NAMESPACE`
  这个通常填写你自己的 Docker Hub 用户名或组织名，不是随便取的本地名字。
  例如你的 Docker Hub 用户名是 `myname`，那就写 `NAMESPACE=myname`。
  注意：镜像仓库路径必须全小写，所以这里不能写 `yassuoDocker`，要写成 `yassuodocker`。
- `IMAGE`
  镜像名，默认是 `t5-tuyaos`，你也可以改成自己想要的名字。
  注意：`IMAGE` 也必须全小写。
- `TAG`
  镜像标签，默认是 `latest`，也可以改成 `v1`、`2026-03-30` 这类版本号。
- `REGISTRY`
  镜像仓库地址，默认是 `docker.io`。如果你使用 Docker Hub，一般不用改。
- `ENGINE`
  容器引擎，默认是 `podman`。如果你本机就是用 Podman，不需要改。
- `PLATFORM`
  默认是 `linux/amd64`，通常也不需要改。
- `APP_NAME`
  这个只在 `make build-firmware` 时需要关心，默认是 `SuperT`。
- `APP_VERSION`
  这个也主要用于 `make build-firmware`，默认是 `1.0.0`。

### 镜像地址是怎么组成的

最终镜像地址格式如下：

```text
$(REGISTRY)/$(NAMESPACE)/$(IMAGE):$(TAG)
```

例如：

- `REGISTRY=docker.io`
- `NAMESPACE=myname`
- `IMAGE=t5-tuyaos`
- `TAG=v1`

最终镜像地址就是：

```text
docker.io/myname/t5-tuyaos:v1
```

## 常用命令

### 1. 登录镜像仓库

```bash
make login NAMESPACE=myname
```

### 2. 构建镜像

```bash
make build NAMESPACE=myname
```

### 3. 推送镜像

```bash
make push NAMESPACE=myname
```

### 4. 一次性构建并上传

```bash
make publish NAMESPACE=myname TAG=v1
```

### 5. 拉取镜像

```bash
make pull NAMESPACE=myname TAG=v1
```

### 6. 运行容器

```bash
make run NAMESPACE=myname TAG=v1
```

### 7. 进入容器 shell

```bash
make shell NAMESPACE=myname TAG=v1
```

### 8. 在容器中编译固件

```bash
make build-firmware NAMESPACE=myname TAG=v1 APP_NAME=SuperT APP_VERSION=1.0.0
```

### 9. 启动带源码挂载的开发容器

这个目标适合日常开发。它会启动一个常驻容器，并把当前项目目录挂载到容器里的 `/workspace`，这样你改完代码后，容器里马上就是最新内容，不需要重新构建镜像。

```bash
make dev-up NAMESPACE=myname TAG=v1
```

进入这个开发容器：

```bash
make dev-shell
```

在这个开发容器里编译固件：

```bash
make dev-build-firmware
```

停止并删除这个开发容器：

```bash
make dev-stop
```

## Windows 端直接使用 Podman

上传后，你也可以在 Windows 端直接执行：

```bash
podman pull docker.io/myname/t5-tuyaos:v1
podman run --rm -it docker.io/myname/t5-tuyaos:v1
```

如果要直接触发默认固件编译：

```bash
podman run --rm -it \
  -e TUYA_APP_NAME=SuperT \
  -e TUYA_APP_VERSION=1.0.0 \
  docker.io/myname/t5-tuyaos:v1 build
```

## 最常见的填写方式

如果你只是想先跑通一次，通常只需要自己填写这几个：

```bash
make publish NAMESPACE=myname
```

或者：

```bash
make publish NAMESPACE=myname IMAGE=t5-tuyaos TAG=v1
```

其中：

- `myname` 换成你的 Docker Hub 用户名，并且必须全小写
- `t5-tuyaos` 可以保留默认值，也可以换成你自己的镜像名，但也必须全小写
- `v1` 换成你希望发布的版本标签

## Windows 通过 Samba 打开 Linux 目录时，怎样让 VS Code 拥有容器编译环境

如果你现在是在 Windows 上用 Samba 打开这个 Linux 目录，那么当前这个 VS Code 窗口本质上还是“Windows 本地窗口”，它只是编辑远端文件，不等于已经进入 Linux 或容器环境。

最推荐的方式是：

1. 在 Windows 端 VS Code 安装 `Remote - SSH` 和 `Dev Containers`
2. 用 `Remote - SSH` 连接到这台 Linux 主机
3. 在远端窗口里打开这个项目目录
4. 在 Linux 主机上先启动开发容器：

```bash
make dev-up NAMESPACE=myname TAG=v1
```

5. 在 VS Code 命令面板执行 `Dev Containers: Attach to Running Container...`
6. 选择 `$(IMAGE)-dev` 对应的容器

这样之后：

- 代码还是你这份项目目录
- 编译环境来自你上传并拉取下来的镜像
- 容器里看到的是实时源码，不是打包时的旧副本
- VS Code 终端、任务、插件运行环境都会切到容器里

如果你继续只用 Samba 打开目录，而不走 `Remote - SSH`，那 VS Code 本身不会自动继承 Linux 容器环境，这种方式更适合“编辑文件”，不适合做完整编译环境集成。
