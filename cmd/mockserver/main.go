package main

import (
	"fmt"
	"log"
	"net/http"
	"os/user"
	"path/filepath"
	"time"
)

func handler(w http.ResponseWriter, r *http.Request) {
	log.Println(time.Now().UTC().Format(time.RFC3339Nano), r.Method, r.URL.Path)

	fmt.Fprintf(w, `{}`)
}

func main() {
	myself, err := user.Current()

	if err != nil {
		log.Fatal(err)
	}

	fileName := "mockserver.txt"
	outputPath := filepath.Join(myself.HomeDir, "AppData", "Roaming", "ScreenReaderX", "Logs", "SystemLog", fileName)
	output := NewBackgroundWriter(outputPath)

	defer output.Close()

	log.SetFlags(0)
	log.SetOutput(output)
	log.SetPrefix("")

	http.HandleFunc("/", handler)
	log.Fatal(http.ListenAndServe(":7902", nil))
}
