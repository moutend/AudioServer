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
	"os/user"
	"path/filepath"
	"strings"
	"time"

	"github.com/moutend/AudioServer/internal/core"
	"github.com/moutend/AudioServer/internal/util"

	"github.com/go-chi/chi"
	"github.com/moutend/AudioServer/internal/api"
	"github.com/spf13/cobra"
	"github.com/spf13/viper"
)

var RootCommand = &cobra.Command{
	Use:  "todoapp",
	RunE: rootRunE,
}

func rootRunE(cmd *cobra.Command, args []string) error {
	if path, _ := cmd.Flags().GetString("config"); path != "" {
		viper.SetConfigFile(path)
	}

	viper.SetEnvKeyReplacer(strings.NewReplacer(".", "_", "-", "_"))
	viper.AutomaticEnv()

	if err := viper.ReadInConfig(); err != nil {
		return err
	}

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

	if err := core.Setup(); err != nil {
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

	if err := ioutil.WriteFile(serverConfigPath, serverConfig, 0644); err != nil {
		return err
	}

	log.Printf("Listening on %s\n", serverAddr)

	return http.Serve(listener, router)
}

func init() {
	RootCommand.PersistentFlags().StringP("config", "c", "", "Path to configuration file")
}
