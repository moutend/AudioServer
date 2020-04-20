package app

import (
	"encoding/hex"
	"fmt"
	"log"
	"math/rand"
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

	address := "localhost:7902"

	if a := viper.GetString("server.address"); a != "" {
		address = a
	}

	log.Printf("Listening on %s\n", address)

	return http.ListenAndServe(address, router)
}

func init() {
	RootCommand.PersistentFlags().StringP("config", "c", "", "Path to configuration file")
}