package main

import (
	"os"

	"github.com/moutend/AudioServer/internal/app"
)

func main() {
	app.RootCommand.SetOutput(os.Stdout)

	if err := app.RootCommand.Execute(); err != nil {
		app.RootCommand.SetOutput(os.Stderr)

		os.Exit(-1)
	}
}
