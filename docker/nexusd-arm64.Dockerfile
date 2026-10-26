FROM golang:1.27-bookworm

WORKDIR /workspace/apps/nexusd
ENV GOTOOLCHAIN=local
CMD ["bash", "-c", "mkdir -p bin /workspace/build/docker/arm64/bin /workspace/build/docker/arm64/go-mod-cache && CGO_ENABLED=0 GOOS=linux GOARCH=arm64 go build -buildvcs=false -trimpath -o /workspace/build/docker/arm64/bin/nexusd ./cmd/nexusd && CGO_ENABLED=0 GOOS=linux GOARCH=arm64 go build -buildvcs=false -trimpath -o /workspace/build/docker/arm64/bin/nexusctl ./cmd/nexusctl && install -m 0755 /workspace/build/docker/arm64/bin/nexusd bin/nexusd && install -m 0755 /workspace/build/docker/arm64/bin/nexusctl bin/nexusctl"]
