package socketpath

import (
	"fmt"
	"os"
	"path/filepath"
)

const EnvironmentVariable = "NEXUSD_SOCKET"

// Default returns the configured socket path or a per-user path in the cache
// directory. The socket itself is created with owner-only access.
func Default() (string, error) {
	if path := os.Getenv(EnvironmentVariable); path != "" {
		return filepath.Clean(path), nil
	}

	user, err := os.UserCacheDir()
	if err != nil {
		return "", fmt.Errorf("resolve user cache directory: %w", err)
	}

	dir := filepath.Join(user, "nexusd")
	return filepath.Join(dir, "nexusd.sock"), nil
}
