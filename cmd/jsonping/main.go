package main

import (
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"os/user"
	"path"
	"path/filepath"
)

func main() {
	log.SetFlags(0)
	log.SetPrefix("error: ")

	if err := run(); err != nil {
		log.Fatal(err)
	}
}

func run() (err error) {
	var (
		pathFlag   string
		methodFlag string
	)

	flag.StringVar(&pathFlag, "path", "", "Path to the JSON file.")
	flag.StringVar(&methodFlag, "method", "", "HTTP method e.g. POST")
	flag.Parse()

	if len(flag.Args()) < 1 {
		return fmt.Errorf("specify address")
	}

	address := flag.Args()[0]
	client := &http.Client{}

	myself, err := user.Current()

	if err != nil {
		return err
	}

	audioServerConfigPath := filepath.Join(myself.HomeDir, "AppData", "Roaming", "ScreenReaderX", "Server", "AudioServer.json")

	audioServerConfigData, err := ioutil.ReadFile(audioServerConfigPath)

	if err != nil {
		return err
	}

	var audioServerConfig struct {
		Addr string `json:"addr"`
	}

	if err := json.Unmarshal(audioServerConfigData, &audioServerConfig); err != nil {
		return err
	}

	url := path.Join(audioServerConfig.Addr, address)

	fmt.Println("Sending request to:", url)

	var req *http.Request

	if pathFlag == "" {
		req, err = http.NewRequest(methodFlag, address, nil)
	} else {
		data, err := ioutil.ReadFile(pathFlag)

		if err != nil {
			return err
		}

		req, err = http.NewRequest(methodFlag, url, bytes.NewBuffer(data))
	}
	if err != nil {
		return err
	}

	res, err := client.Do(req)

	if err != nil {
		return err
	}

	defer res.Body.Close()

	body := &bytes.Buffer{}

	if _, err := io.Copy(body, res.Body); err != nil {
		return err
	}

	fmt.Printf("%s %s\n", res.Status, body.Bytes())

	return nil
}
