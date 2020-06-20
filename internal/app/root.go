package app

import (
	"encoding/hex"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"math/rand"
	"net"
	"net/http"
	"os"
	"os/user"
	"path/filepath"
	"time"

	"github.com/moutend/AudioServer/internal/core"
	"github.com/moutend/AudioServer/internal/util"

	"github.com/go-chi/chi"
	"github.com/moutend/AudioServer/internal/api"
	"github.com/spf13/cobra"
)

var RootCommand = &cobra.Command{
	Use:  "todoapp",
	RunE: rootRunE,
}

func rootRunE(cmd *cobra.Command, args []string) error {
	rand.Seed(time.Now().Unix())
	p := make([]byte, 16)

	if _, err := rand.Read(p); err != nil {
		return err
	}

	myself, err := user.Current()

	if err != nil {
		return err
	}

	fileName := fmt.Sprintf("AudioServer-%s.txt", hex.EncodeToString(p))
	outputPath := filepath.Join(myself.HomeDir, "AppData", "Roaming", "ScreenReaderX", "Logs", "SystemLog", fileName)
	output := util.NewBackgroundWriter(outputPath)

	defer output.Close()

	log.SetFlags(log.Ldate | log.Ltime | log.LUTC | log.Llongfile)
	log.SetOutput(output)

	var logServerConfig struct {
		Addr string `json:"addr"`
	}

	logServerConfigPath := filepath.Join(myself.HomeDir, "AppData", "Roaming", "ScreenReaderX", "Server", "LogServer.json")

	logServerConfigData, err := ioutil.ReadFile(logServerConfigPath)

	if err != nil {
		return err
	}
	if err := json.Unmarshal(logServerConfigData, &logServerConfig); err != nil {
		return err
	}
	if err := core.Setup(logServerConfig.Addr); err != nil {
		return err
	}

	defer core.Teardown()

	router := chi.NewRouter()
	api.Setup(router)

	listener, err := net.Listen("tcp", "127.0.0.1:0")

	if err != nil {
		return err
	}

	serverAddr := listener.Addr().(*net.TCPAddr).String()

	serverConfig, err := json.Marshal(struct {
		Addr string `json:"addr"`
	}{
		Addr: serverAddr,
	})

	if err != nil {
		return err
	}

	serverConfigPath := filepath.Join(myself.HomeDir, "AppData", "Roaming", "ScreenReaderX", "Server", "AudioServer.json")
	os.MkdirAll(filepath.Dir(serverConfigPath), 0755)

	if err := ioutil.WriteFile(serverConfigPath, serverConfig, 0644); err != nil {
		return err
	}

	log.Printf("Listening on %s\n", serverAddr)

	return http.Serve(listener, router)
}
