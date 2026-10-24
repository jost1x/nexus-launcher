GOOS ?= $(shell go env GOOS)
GOARCH ?= $(shell go env GOARCH)
BIN_DIR ?= bin

.PHONY: generate lint-proto build build-arm64 fmt vet clean

generate:
	cd ../../packages/proto && buf generate

lint-proto:
	cd ../../packages/proto && buf lint

build:
	GOOS=$(GOOS) GOARCH=$(GOARCH) go build -o $(BIN_DIR)/nexusd ./cmd/nexusd
	GOOS=$(GOOS) GOARCH=$(GOARCH) go build -o $(BIN_DIR)/nexusctl ./cmd/nexusctl

build-arm64:
	$(MAKE) build GOOS=linux GOARCH=arm64

fmt:
	gofmt -w $$(find cmd internal gen -name '*.go' -type f)

vet:
	go vet ./...

clean:
	rm -rf $(BIN_DIR)
