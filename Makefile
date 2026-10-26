DOCKER_COMPOSE ?= docker compose
ANSIBLE_PLAYBOOK ?= ansible-playbook
BUILD_JOBS ?= 2
LOCAL_UID ?= $(shell id -u)
LOCAL_GID ?= $(shell id -g)
NEXUS_UI_BUNDLE_DIR ?= $(CURDIR)/build/docker/arm64/nexus-r36s-bundle
ANSIBLE_BECOME_FLAGS ?= -K
ANSIBLE_ARGS ?=

.PHONY: help build-images build-ui build-ui-desktop build-daemon build-arm64 \
	deploy-ui deploy-ui-fast deploy-daemon deploy-all

help:
	@printf '%s\n' \
	  'NEXUS build and deployment targets:' \
	  '  make build-images      Build the ARM64 Docker builder images' \
	  '  make build-ui          Build the framebuffer UI for R36S' \
	  '  make build-ui-desktop  Build the SDL desktop UI locally' \
	  '  make build-daemon      Build nexusd and nexusctl for ARM64' \
	  '  make build-arm64       Build the UI and daemon for R36S' \
	  '  make deploy-ui         Build and fully prepare/deploy the UI' \
	  '  make deploy-ui-fast    Build and deploy UI without dependency checks' \
	  '  make deploy-daemon     Build and deploy nexusd and nexusctl' \
	  '  make deploy-all        Build and deploy the daemon, then the UI' \
	  '' \
	  'Set BUILD_JOBS=4 to change Docker build parallelism.' \
	  'Set ANSIBLE_BECOME_FLAGS= to avoid the sudo password prompt when permitted.'

build-images:
	$(DOCKER_COMPOSE) build --builder default build-ui build-daemon

build-ui:
	LOCAL_UID=$(LOCAL_UID) LOCAL_GID=$(LOCAL_GID) BUILD_JOBS=$(BUILD_JOBS) $(DOCKER_COMPOSE) run --rm build-ui

build-ui-desktop:
	cmake -S apps/nexus-ui -B build/nexus-ui -DCMAKE_BUILD_TYPE=Release
	cmake --build build/nexus-ui --parallel $(BUILD_JOBS)

build-daemon:
	LOCAL_UID=$(LOCAL_UID) LOCAL_GID=$(LOCAL_GID) BUILD_JOBS=$(BUILD_JOBS) $(DOCKER_COMPOSE) run --rm build-daemon

build-arm64:
	$(MAKE) build-ui BUILD_JOBS=$(BUILD_JOBS)
	$(MAKE) build-daemon BUILD_JOBS=$(BUILD_JOBS)

deploy-ui: build-ui
	cd deploy/ansible && $(ANSIBLE_PLAYBOOK) playbooks/deploy-ui.yml $(ANSIBLE_BECOME_FLAGS) -e 'nexus_ui_bundle_dir=$(NEXUS_UI_BUNDLE_DIR)' $(ANSIBLE_ARGS)

deploy-ui-fast: build-ui
	cd deploy/ansible && $(ANSIBLE_PLAYBOOK) playbooks/deploy-ui.yml $(ANSIBLE_BECOME_FLAGS) --skip-tags ui_dependencies -e 'nexus_ui_bundle_dir=$(NEXUS_UI_BUNDLE_DIR)' $(ANSIBLE_ARGS)

deploy-daemon: build-daemon
	cd deploy/ansible && $(ANSIBLE_PLAYBOOK) playbooks/deploy.yml $(ANSIBLE_BECOME_FLAGS) $(ANSIBLE_ARGS)

deploy-all: build-arm64
	cd deploy/ansible && $(ANSIBLE_PLAYBOOK) playbooks/deploy.yml $(ANSIBLE_BECOME_FLAGS) $(ANSIBLE_ARGS)
	cd deploy/ansible && $(ANSIBLE_PLAYBOOK) playbooks/deploy-ui.yml $(ANSIBLE_BECOME_FLAGS) -e 'nexus_ui_bundle_dir=$(NEXUS_UI_BUNDLE_DIR)' $(ANSIBLE_ARGS)
