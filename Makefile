ENGINE ?= podman
REGISTRY ?= docker.io
NAMESPACE ?= yassuodocker
IMAGE ?= t5-tuyaos
TAG ?= VERSION-$(shell date +%Y%m%d%H%M%S) 
PLATFORM ?= linux/amd64
APP_NAME ?= SuperT
APP_VERSION ?= 1.0.0
CONTAINER_NAME ?= $(IMAGE)-dev
PROJECT_DIR ?= $(CURDIR)
WORKSPACE_DIR ?= /workspace

IMAGE_REPO := $(REGISTRY)/$(NAMESPACE)/$(IMAGE)
IMAGE_REF := $(IMAGE_REPO):$(TAG)

.PHONY: help check-engine check-namespace login build push publish pull run shell build-firmware dev-up dev-stop dev-shell dev-build-firmware

help:
	@echo "Available targets:"
	@echo "  make build            Build the container image"
	@echo "  make push             Push the image to the registry"
	@echo "  make publish          Build and push the image"
	@echo "  make pull             Pull the image from the registry"
	@echo "  make run              Run the container with the default command"
	@echo "  make shell            Open a shell in the container"
	@echo "  make build-firmware   Run the default firmware build in the container"
	@echo "  make dev-up           Start a long-lived dev container with the project bind-mounted"
	@echo "  make dev-stop         Stop and remove the dev container"
	@echo "  make dev-shell        Open a shell in the running dev container"
	@echo "  make dev-build-firmware Build firmware inside the running dev container"
	@echo "  make login            Log in to the registry"
	@echo ""
	@echo "Variables you can override:"
	@echo "  ENGINE=$(ENGINE)"
	@echo "  REGISTRY=$(REGISTRY)"
	@echo "  NAMESPACE=$(NAMESPACE)"
	@echo "  IMAGE=$(IMAGE)"
	@echo "  TAG=$(TAG)"
	@echo "  PLATFORM=$(PLATFORM)"
	@echo "  APP_NAME=$(APP_NAME)"
	@echo "  APP_VERSION=$(APP_VERSION)"
	@echo "  CONTAINER_NAME=$(CONTAINER_NAME)"
	@echo "  PROJECT_DIR=$(PROJECT_DIR)"
	@echo "  WORKSPACE_DIR=$(WORKSPACE_DIR)"
	@echo ""
	@echo "Example:"
	@echo "  make publish NAMESPACE=myname TAG=v1"

check-engine:
	@$(ENGINE) --version > /dev/null

check-namespace:
	@if [ "$(NAMESPACE)" = "your-dockerhub-user" ]; then \
		echo "Please set NAMESPACE, for example: make build NAMESPACE=myname"; \
		exit 1; \
	fi

login: check-engine check-namespace
	$(ENGINE) login $(REGISTRY)

build: check-engine check-namespace
	$(ENGINE) build --platform $(PLATFORM) -t $(IMAGE_REF) .

push: check-engine check-namespace
	$(ENGINE) push $(IMAGE_REF)

publish: build push

pull: check-engine check-namespace
	$(ENGINE) pull $(IMAGE_REF)

run: check-engine check-namespace
	$(ENGINE) run --rm -it --platform $(PLATFORM) $(IMAGE_REF)

shell: check-engine check-namespace
	$(ENGINE) run --rm -it --platform $(PLATFORM) --entrypoint /bin/bash $(IMAGE_REF)

build-firmware: check-engine check-namespace
	$(ENGINE) run --rm -it --platform $(PLATFORM) \
		-e TUYA_APP_NAME=$(APP_NAME) \
		-e TUYA_APP_VERSION=$(APP_VERSION) \
		$(IMAGE_REF) build

dev-up: check-engine check-namespace
	$(ENGINE) run -d --name $(CONTAINER_NAME) --platform $(PLATFORM) \
		-v $(PROJECT_DIR):$(WORKSPACE_DIR) \
		-w $(WORKSPACE_DIR)/software/TuyaOS \
		-e TUYA_ROOT=$(WORKSPACE_DIR)/software/TuyaOS \
		-e TUYA_APP_NAME=$(APP_NAME) \
		-e TUYA_APP_VERSION=$(APP_VERSION) \
		--entrypoint /bin/bash \
		$(IMAGE_REF) -lc "sleep infinity"

dev-stop: check-engine
	-$(ENGINE) rm -f $(CONTAINER_NAME)

dev-shell: check-engine
	$(ENGINE) exec -it $(CONTAINER_NAME) /bin/bash

dev-build-firmware: check-engine
	$(ENGINE) exec -it $(CONTAINER_NAME) /bin/bash -lc './build_app.sh "." "$${TUYA_APP_NAME}" "$${TUYA_APP_VERSION}"'
