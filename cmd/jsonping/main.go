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
		PayloadFlag string
		methodFlag  string
	)

	flag.StringVar(&PayloadFlag, "path", "", "Path to the JSON file.")
	flag.StringVar(&methodFlag, "method", "", "HTTP method e.g. POST")
	flag.Parse()

	if len(flag.Args()) < 1 {
		return fmt.Errorf("specify address")
	}

	path := flag.Args()[0]
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

	address := fmt.Sprintf("http://%s%s", audioServerConfig.Addr, path)

	fmt.Println("Sending request to:", address)

	var req *http.Request

	if PayloadFlag == "" {
		req, err = http.NewRequest(methodFlag, address, nil)
	} else {
		data, err := ioutil.ReadFile(PayloadFlag)

		if err != nil {
			return err
		}

		req, err = http.NewRequest(methodFlag, address, bytes.NewBuffer(data))
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
